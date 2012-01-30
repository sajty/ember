//
// C++ Implementation: Input
//
// Description: 
//
//
// Author: Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2005
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.//
//
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Input.h"
#include "IWindowProvider.h"
#include "InputCommandMapper.h"

#include "IInputAdapter.h"

#include "services/config/ConfigListenerContainer.h"

#include "framework/Tokeniser.h"
#include "framework/ConsoleBackend.h"
#include "framework/LoggingInstance.h"
#include "framework/MainLoopController.h"

#include <OIS.h>
#include <boost/thread/thread.hpp>

template<> Ember::Input* Ember::Singleton<Ember::Input>::ms_Singleton = 0;

namespace Ember
{

const std::string Input::BINDCOMMAND("bind");
const std::string Input::UNBINDCOMMAND("unbind");

Input::Input() :
	mCurrentInputMode(IM_GUI),
	mSuppressForCurrentEvent(false), mMovementModeEnabled(false),
	mMouseGrab(false), mWindowProvider(NULL),
	mInputManager(NULL), mKeyboard(NULL), mMouse(NULL),
	mConfigListenerContainer(new ConfigListenerContainer())
{
	mMousePosition.xPixelPosition = 0;
	mMousePosition.yPixelPosition = 0;
	mMousePosition.xRelativePosition = 0.0f;
	mMousePosition.yRelativePosition = 0.0f;
}

Input::~Input()
{
	delete mConfigListenerContainer;
}

void Input::attach(IWindowProvider* windowProvider, OIS::ParamList& params)
{
	//mInputManager should not exist. You should call Input::detach first.
	assert(!mInputManager);

	//The windowProvider should not be NULL.
	assert(windowProvider);
	mWindowProvider = windowProvider;

	params.insert(std::make_pair("WINDOW", mWindowProvider->getWindowHandle()));

	mInputManager = OIS::InputManager::createInputSystem(params);

	// If possible create a buffered keyboard
	if (mInputManager->getNumberOfDevices(OIS::OISKeyboard) > 0) {
			mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject (OIS::OISKeyboard, true));
			mKeyboard->setEventCallback (this);
		}

	// If possible create a buffered mouse
	if (mInputManager->getNumberOfDevices(OIS::OISMouse) > 0) {
		mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject (OIS::OISMouse, true));
		mMouse->setEventCallback (this);

		//update the window size for the mouse
		unsigned int width, height;
		mWindowProvider->getWindowSize(width, height);
		setGeometry(width, height);
		
		mMouse->grab(false);
	}

	ConsoleBackend& console = ConsoleBackend::getSingleton();
	console.registerCommand(BINDCOMMAND, this);
	console.registerCommand(UNBINDCOMMAND, this);

}

void Input::setGeometry(int width, int height)
{
	if (mMouse) {
		const OIS::MouseState& mouseState = mMouse->getMouseState();
		mouseState.width  = width;
		mouseState.height = height;
	}
}

void Input::runCommand(const std::string &command, const std::string &args)
{
	if (command == BINDCOMMAND) {
		Tokeniser tokeniser;
		tokeniser.initTokens(args);
		std::string state("general");
		std::string key = tokeniser.nextToken();
		if (key != "") {
			std::string command(tokeniser.nextToken());
			if (command != "") {
				if (tokeniser.nextToken() != "") {
					state = tokeniser.nextToken();
				}
				InputCommandMapperStore::iterator I = mInputCommandMappers.find(state);
				if (I != mInputCommandMappers.end()) {
					I->second->bindCommand(key, command);
				}
			}
		}
	} else if (command == UNBINDCOMMAND) {
		Tokeniser tokeniser;
		tokeniser.initTokens(args);
		std::string state("general");
		std::string key(tokeniser.nextToken());
		if (key != "") {
			if (tokeniser.nextToken() != "") {
				state = tokeniser.nextToken();
			}
			InputCommandMapperStore::iterator I = mInputCommandMappers.find(state);
			if (I != mInputCommandMappers.end()) {
				I->second->unbindCommand(key);
			}
		}
	}
}

void Input::suppressFurtherHandlingOfCurrentEvent()
{
	mSuppressForCurrentEvent = true;
}

bool Input::getMovementModeEnabled() const
{
	return mMovementModeEnabled;
}

void Input::setMovementModeEnabled(bool value)
{
	mMovementModeEnabled = value;
}

void Input::registerCommandMapper(InputCommandMapper* mapper)
{
	mInputCommandMappers[mapper->getState()] = mapper;
}

void Input::deregisterCommandMapper(InputCommandMapper* mapper)
{
	mInputCommandMappers.erase(mInputCommandMappers.find(mapper->getState()));
}

bool Input::isApplicationVisible()
{
	return mWindowProvider->isWindowVisible();
}

void Input::startInteraction()
{
	mConfigListenerContainer->registerConfigListenerWithDefaults("input", "catchmouse", sigc::mem_fun(*this, &Input::Config_CatchMouse), true);
}

void Input::processInput()
{
	if (mMouse) {
		mMouse->capture();
	}

	if (mKeyboard) {
		mKeyboard->capture();
	}
}


bool Input::mouseMoved(const OIS::MouseEvent& e)
{

	//handle mouse scrolling
	int scroll = e.state.Z.rel;
	if(scroll != 0){
		if(scroll > 0){
			EventMouseButtonPressed.emit(MouseWheelUp, mCurrentInputMode);
			for (IInputAdapterStore::const_iterator I = mAdapters.begin(); I != mAdapters.end();I++) {
				IInputAdapter* adapter = *I;
				if (!adapter->injectMouseButtonDown(Input::MouseWheelUp)){
					break;
				}
			}
		}else{
			EventMouseButtonPressed.emit(MouseWheelDown, mCurrentInputMode);
			for (IInputAdapterStore::const_iterator I = mAdapters.begin(); I != mAdapters.end();I++) {
				IInputAdapter* adapter = *I;
				if (!adapter->injectMouseButtonDown(Input::MouseWheelDown)){
					break;
				}
			}
		}
	}

	float width = e.state.width;
	float height = e.state.height;

	//TODO: Remove this and freezeMouse. Istead use new InputMode and grab input.
	MousePosition oldPos(mMousePosition);

	//We need to update position first, since we don't want to send old position.
	if (mCurrentInputMode == IM_GUI) {

		//update mouse position
		mMousePosition.xPixelPosition = e.state.X.abs;
		mMousePosition.yPixelPosition = e.state.Y.abs;
		mMousePosition.xRelativePosition = e.state.X.abs / width;
		mMousePosition.yRelativePosition = e.state.Y.abs / height;
	}

		MouseMotion motion;
		motion.xPosition = e.state.X.abs;
		motion.yPosition = e.state.Y.abs;
		motion.xRelativeMovementInPixels = e.state.X.rel;
		motion.yRelativeMovementInPixels = e.state.Y.rel;
		motion.xRelativeMovement = e.state.X.rel / width;
		motion.yRelativeMovement = e.state.Y.rel / height;
		
		EventMouseMoved.emit(motion, mCurrentInputMode);

	//if we're in gui mode, we'll just send the mouse movement on to CEGUI
	if (mCurrentInputMode == IM_GUI) {
		bool freezeMouse = false;
		for (IInputAdapterStore::const_iterator I = mAdapters.begin(); I != mAdapters.end();I++) {
			IInputAdapter* adapter = *I;
			if (!adapter->injectMouseMove(motion, freezeMouse)){
				break;
			}
		}
		if(freezeMouse){
			mMousePosition = oldPos;
			mMouse->setPosition(oldPos.xPixelPosition, oldPos.yPixelPosition);
		}
	}

	

	return true;
}
bool Input::mousePressed(const OIS::MouseEvent& e, OIS::MouseButtonID id)
{

	switch (id){
	case OIS::MB_Right:
		//if the right mouse button is pressed, switch from gui mode
		if (mMovementModeEnabled) {
			toggleInputMode();
		}
		EventMouseButtonPressed.emit(MouseButtonRight, mCurrentInputMode);
		break;

	case OIS::MB_Middle:
		//middle mouse button pressed
		for (IInputAdapterStore::const_iterator I = mAdapters.begin(); I != mAdapters.end();I++) {
			IInputAdapter* adapter = *I;
			if (!adapter->injectMouseButtonDown(Input::MouseButtonMiddle)){
				break;
			}
		}

		EventMouseButtonPressed.emit(MouseButtonMiddle, mCurrentInputMode);
		break;

	case OIS::MB_Left:
		//left mouse button pressed
		for (IInputAdapterStore::const_iterator I = mAdapters.begin(); I != mAdapters.end();I++) {
			IInputAdapter* adapter = *I;
			if (!adapter->injectMouseButtonDown(Input::MouseButtonLeft)){
				break;
			}
		}

		EventMouseButtonPressed.emit(MouseButtonLeft, mCurrentInputMode);
		break;
	}
	return true;
}

bool Input::mouseReleased(const OIS::MouseEvent& e, OIS::MouseButtonID id)
{
	switch (id){
	case OIS::MB_Right:
		EventMouseButtonReleased.emit(MouseButtonRight, mCurrentInputMode);
		break;

	case OIS::MB_Middle:
		//middle mouse button released
		for (IInputAdapterStore::const_iterator I = mAdapters.begin(); I != mAdapters.end();I++) {
			IInputAdapter* adapter = *I;
			if (!adapter->injectMouseButtonUp(Input::MouseButtonMiddle)){
				break;
			}
		}

		EventMouseButtonReleased.emit(MouseButtonMiddle, mCurrentInputMode);
		break;

	case OIS::MB_Left:
		//left mouse button released
		for (IInputAdapterStore::const_iterator I = mAdapters.begin(); I != mAdapters.end();I++) {
			IInputAdapter* adapter = *I;
			if (!adapter->injectMouseButtonUp(Input::MouseButtonLeft)){
				break;
			}
		}

		EventMouseButtonReleased.emit(MouseButtonLeft, mCurrentInputMode);
		break;
	}
	return true;
}

void Input::writeToClipboard(const std::string& text)
{
	//TODO: implement clipboard support.
}

void Input::pasteFromClipboard()
{
	//TODO: Implement clipboard support.
}

const bool Input::isKeyDown(const OIS::KeyCode &key) const
{
	if (mKeyboard) {
		return mKeyboard->isKeyDown(key);
	} else {
		return false;
	}
}
const bool Input::isModifierDown(const OIS::Keyboard::Modifier &modifier) const
{
	if (mKeyboard) {
		return mKeyboard->isModifierDown(modifier);
	} else {
		return false;
	}
}
bool Input::keyPressed(const OIS::KeyEvent& keyEvent)
{

	if (keyEvent.key == OIS::KC_V && (mKeyboard->isKeyDown(OIS::KC_RCONTROL) || mKeyboard->isKeyDown(OIS::KC_LCONTROL))) {
		pasteFromClipboard();
	} else {
		mSuppressForCurrentEvent = false;
		if (mCurrentInputMode == IM_GUI) {
			// do event injection
			// key down
			for (IInputAdapterStore::const_iterator I = mAdapters.begin(); I != mAdapters.end() && !mSuppressForCurrentEvent;I++) {
				IInputAdapter* adapter = *I;
				if (!adapter->injectKeyDown(keyEvent.key)){
					break;
				}
			}

			//if keyEvent.text is 0, means the key is a function key.
			//NOTE: Encoding of keyEvent.text may depend on OS and OS settings.
			if (keyEvent.text != 0) {
				for (IInputAdapterStore::const_iterator I = mAdapters.begin(); I != mAdapters.end() && !mSuppressForCurrentEvent;I++) {
					IInputAdapter* adapter = *I;
					if (!adapter->injectChar(keyEvent.text)){
						break;
					}
				}
			}
		}
		if (!mSuppressForCurrentEvent) {
			EventKeyPressed(keyEvent, mCurrentInputMode);
		}
		mSuppressForCurrentEvent = false;
	}
	return true;
}
bool Input::keyReleased(const OIS::KeyEvent& keyEvent)
{
	mSuppressForCurrentEvent = false;
	if (mCurrentInputMode == IM_GUI) {
		for (IInputAdapterStore::const_iterator I = mAdapters.begin(); I != mAdapters.end() && !mSuppressForCurrentEvent;I++) {
			IInputAdapter* adapter = *I;
			if (!adapter->injectKeyUp(keyEvent.key)){
				break;
			}
		}
	}
	if (!mSuppressForCurrentEvent) {
		EventKeyReleased(keyEvent, mCurrentInputMode);
	}
	mSuppressForCurrentEvent = false;
	return true;
}

void Input::detach()
{
	if (mInputManager) {
		if (mMouse) {
			mInputManager->destroyInputObject (mMouse);
			mMouse = 0;
		}

		if (mKeyboard) {
			mInputManager->destroyInputObject (mKeyboard);
			mKeyboard = 0;
		}

		mInputManager->destroyInputSystem (mInputManager);
		mInputManager = 0;

		ConsoleBackend& console = ConsoleBackend::getSingleton();
		console.deregisterCommand(BINDCOMMAND);
		console.deregisterCommand(UNBINDCOMMAND);
	}
}

void Input::setInputMode(InputMode mode)
{
	setMouseGrab(mode == IM_MOVEMENT);
	mCurrentInputMode = mode;
	EventChangedInputMode.emit(mode);
}

Input::InputMode Input::getInputMode() const
{
	return mCurrentInputMode;
}

Input::InputMode Input::toggleInputMode()
{
	if (mCurrentInputMode == IM_GUI) {
		setInputMode(IM_MOVEMENT);
		return IM_MOVEMENT;
	} else {
		setInputMode(IM_GUI);
		return IM_GUI;
	}
}

void Input::addAdapter(IInputAdapter* adapter)
{
	mAdapters.push_front(adapter);
}

void Input::removeAdapter(IInputAdapter* adapter)
{
	mAdapters.remove(adapter);
}

const MousePosition& Input::getMousePosition() const
{
	return mMousePosition;
}

void Input::sleep(unsigned int milliseconds)
{
	try {
		boost::this_thread::sleep(boost::posix_time::milliseconds(milliseconds));
	} catch (const boost::thread_interrupted& ex) {
	}
}


void Input::Config_CatchMouse(const std::string& section, const std::string& key, varconf::Variable& variable)
{
	if (variable.is_bool()) {
		bool enabled = static_cast<bool>(variable);
		// if (enabled) {
		// 	mMouseGrabbingRequested = true;
		// } else {
		// 	setMouseGrab(false);
		// }
	}
}

void Input::setMouseGrab(bool enabled)
{
	if(mMouseGrab != enabled){
		mMouseGrab = enabled;
		mMouse->grab(enabled);
		
		if (!enabled) {
			mMouse->setPosition(mMousePosition.xPixelPosition, mMousePosition.yPixelPosition);
		}
	}
}
}

