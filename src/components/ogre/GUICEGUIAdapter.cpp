//
// C++ Implementation: GUICEGUIAdapter
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
#include "GUICEGUIAdapter.h"

#include <CEGUIExceptions.h>
#include <CEGUIGlobalEventSet.h>
#include <elements/CEGUIEditbox.h>
#include <elements/CEGUIMultiLineEditbox.h>

namespace Ember {
namespace OgreView {

GUICEGUIAdapter::GUICEGUIAdapter(CEGUI::System *system, CEGUI::OgreRenderer *renderer):
mGuiSystem(system)
, mGuiRenderer(renderer)
, mSelectedText(0)
{

	//lookup table for OIS scancodes and CEGUI keys
	mKeyMap[OIS::KC_BACK] = CEGUI::Key::Backspace;
	mKeyMap[OIS::KC_TAB] = CEGUI::Key::Tab;
/*	mKeyMap[OIS::KC_CLEAR] = CEGUI::Key::Clear;*/
	mKeyMap[OIS::KC_RETURN] = CEGUI::Key::Return;
	mKeyMap[OIS::KC_PAUSE] = CEGUI::Key::Pause;
	mKeyMap[OIS::KC_ESCAPE] = CEGUI::Key::Escape;
	mKeyMap[OIS::KC_SPACE] = CEGUI::Key::Space;
/*	mKeyMap[OIS::KC_EXCLAIM] = CEGUI::Key::Exclaim;*/
/*	mKeyMap[OIS::KC_QUOTEDBL] = CEGUI::Key::;
	mKeyMap[OIS::KC_HASH] = CEGUI::Key::;
	mKeyMap[OIS::KC_DOLLAR] = CEGUI::Key::;
	mKeyMap[OIS::KC_AMPERSAND] = CEGUI::Key::;
	mKeyMap[OIS::KC_QUOTE] = CEGUI::Key::;
	mKeyMap[OIS::KC_LEFTPAREN] = CEGUI::Key::;
	mKeyMap[OIS::KC_RIGHTPAREN] = CEGUI::Key::;
	mKeyMap[OIS::KC_ASTERISK] = CEGUI::Key::;*/
	mKeyMap[OIS::KC_ADD] = CEGUI::Key::Add;
/*	mKeyMap[OIS::KC_COMMA] = CEGUI::Key::;*/
	mKeyMap[OIS::KC_MINUS] = CEGUI::Key::Minus;
	mKeyMap[OIS::KC_PERIOD] = CEGUI::Key::Period;
/*	mKeyMap[OIS::KC_SLASH] = CEGUI::Key::;*/
	mKeyMap[OIS::KC_0] = CEGUI::Key::One;
	mKeyMap[OIS::KC_1] = CEGUI::Key::Two;
	mKeyMap[OIS::KC_2] = CEGUI::Key::Two;
	mKeyMap[OIS::KC_3] = CEGUI::Key::Three;
	mKeyMap[OIS::KC_4] = CEGUI::Key::Four;
	mKeyMap[OIS::KC_5] = CEGUI::Key::Five;
	mKeyMap[OIS::KC_6] = CEGUI::Key::Six;
	mKeyMap[OIS::KC_7] = CEGUI::Key::Seven;
	mKeyMap[OIS::KC_8] = CEGUI::Key::Eight;
	mKeyMap[OIS::KC_9] = CEGUI::Key::Nine;
	mKeyMap[OIS::KC_COLON] = CEGUI::Key::Colon;
	mKeyMap[OIS::KC_SEMICOLON] = CEGUI::Key::Semicolon;
/*	mKeyMap[OIS::KC_LESS] = CEGUI::Key::;*/
/*	mKeyMap[OIS::KC_EQUALS] = CEGUI::Key::;
	mKeyMap[OIS::KC_GREATER] = CEGUI::Key::;
	mKeyMap[OIS::KC_QUESTION] = CEGUI::Key::;*/
/*	mKeyMap[OIS::KC_AT] = CEGUI::Key::;*/
/*	mKeyMap[OIS::KC_LEFTBRACKET] = CEGUI::Key::;*/
	mKeyMap[OIS::KC_BACKSLASH] = CEGUI::Key::Backslash;
/*	mKeyMap[OIS::KC_RIGHTBRACKET] = CEGUI::Key::;*/
/*	mKeyMap[OIS::KC_CARET] = CEGUI::Key::;
	mKeyMap[OIS::KC_UNDERSCORE] = CEGUI::Key::;
	mKeyMap[OIS::KC_BACKQUOTE] = CEGUI::Key::;*/
	mKeyMap[OIS::KC_A] = CEGUI::Key::A;
	mKeyMap[OIS::KC_B] = CEGUI::Key::B;
	mKeyMap[OIS::KC_C] = CEGUI::Key::C;
	mKeyMap[OIS::KC_D] = CEGUI::Key::D;
	mKeyMap[OIS::KC_E] = CEGUI::Key::E;
	mKeyMap[OIS::KC_F] = CEGUI::Key::F;
	mKeyMap[OIS::KC_G] = CEGUI::Key::G;
	mKeyMap[OIS::KC_H] = CEGUI::Key::H;
	mKeyMap[OIS::KC_I] = CEGUI::Key::I;
	mKeyMap[OIS::KC_J] = CEGUI::Key::J;
	mKeyMap[OIS::KC_K] = CEGUI::Key::K;
	mKeyMap[OIS::KC_L] = CEGUI::Key::L;
	mKeyMap[OIS::KC_M] = CEGUI::Key::M;
	mKeyMap[OIS::KC_N] = CEGUI::Key::N;
	mKeyMap[OIS::KC_O] = CEGUI::Key::O;
	mKeyMap[OIS::KC_P] = CEGUI::Key::P;
	mKeyMap[OIS::KC_Q] = CEGUI::Key::Q;
	mKeyMap[OIS::KC_R] = CEGUI::Key::R;
	mKeyMap[OIS::KC_S] = CEGUI::Key::S;
	mKeyMap[OIS::KC_T] = CEGUI::Key::T;
	mKeyMap[OIS::KC_U] = CEGUI::Key::U;
	mKeyMap[OIS::KC_V] = CEGUI::Key::V;
	mKeyMap[OIS::KC_W] = CEGUI::Key::W;
	mKeyMap[OIS::KC_X] = CEGUI::Key::X;
	mKeyMap[OIS::KC_Y] = CEGUI::Key::Y;
	mKeyMap[OIS::KC_Z] = CEGUI::Key::Z;
	mKeyMap[OIS::KC_DELETE] = CEGUI::Key::Delete;
	mKeyMap[OIS::KC_UP] = CEGUI::Key::ArrowUp;
	mKeyMap[OIS::KC_DOWN] = CEGUI::Key::ArrowDown;
	mKeyMap[OIS::KC_RIGHT] = CEGUI::Key::ArrowRight;
	mKeyMap[OIS::KC_LEFT] = CEGUI::Key::ArrowLeft;
	mKeyMap[OIS::KC_INSERT] = CEGUI::Key::Insert;
	mKeyMap[OIS::KC_HOME] = CEGUI::Key::Home;
	mKeyMap[OIS::KC_END] = CEGUI::Key::End;
	mKeyMap[OIS::KC_PGUP] = CEGUI::Key::PageUp;
	mKeyMap[OIS::KC_PGDOWN] = CEGUI::Key::PageDown;
	mKeyMap[OIS::KC_F1] = CEGUI::Key::F1;
	mKeyMap[OIS::KC_F2] = CEGUI::Key::F2;
	mKeyMap[OIS::KC_F3] = CEGUI::Key::F3;
	mKeyMap[OIS::KC_F4] = CEGUI::Key::F4;
	mKeyMap[OIS::KC_F5] = CEGUI::Key::F5;
	mKeyMap[OIS::KC_F6] = CEGUI::Key::F6;
	mKeyMap[OIS::KC_F7] = CEGUI::Key::F7;
	mKeyMap[OIS::KC_F8] = CEGUI::Key::F8;
	mKeyMap[OIS::KC_F9] = CEGUI::Key::F9;
	mKeyMap[OIS::KC_F10] = CEGUI::Key::F10;
	mKeyMap[OIS::KC_F11] = CEGUI::Key::F11;
	mKeyMap[OIS::KC_F12] = CEGUI::Key::F12;
	mKeyMap[OIS::KC_F13] = CEGUI::Key::F13;
	mKeyMap[OIS::KC_F14] = CEGUI::Key::F14;
	mKeyMap[OIS::KC_F15] = CEGUI::Key::F15;
	mKeyMap[OIS::KC_NUMLOCK] = CEGUI::Key::NumLock;
	mKeyMap[OIS::KC_SCROLL] = CEGUI::Key::ScrollLock;
	mKeyMap[OIS::KC_RSHIFT] = CEGUI::Key::RightShift;
	mKeyMap[OIS::KC_LSHIFT] = CEGUI::Key::LeftShift;
	mKeyMap[OIS::KC_RCONTROL] = CEGUI::Key::RightControl;
	mKeyMap[OIS::KC_LCONTROL] = CEGUI::Key::LeftControl;
	mKeyMap[OIS::KC_RMENU] = CEGUI::Key::RightAlt;
	mKeyMap[OIS::KC_LMENU] = CEGUI::Key::LeftAlt;


	//set up the capturing of text selected event for the copy-and-paste functionality

//window->subscribeEvent(event, CEGUI::Event::Subscriber(&method, this));

	CEGUI::GlobalEventSet::getSingleton().subscribeEvent("MultiLineEditbox/TextSelectionChanged", CEGUI::Event::Subscriber(&GUICEGUIAdapter::MultiLineEditbox_selectionChangedHandler, this));
	CEGUI::GlobalEventSet::getSingleton().subscribeEvent("Editbox/TextSelectionChanged", CEGUI::Event::Subscriber(&GUICEGUIAdapter::Editbox_selectionChangedHandler, this));
	//CEGUI::GlobalEventSet::getSingleton().subscribeEvent("Editbox/TextSelectionChanged", &GUICEGUIAdapter::selectionChangedHandler);



}


GUICEGUIAdapter::~GUICEGUIAdapter()
{
}

bool GUICEGUIAdapter::MultiLineEditbox_selectionChangedHandler(const CEGUI::EventArgs& args)
{
	CEGUI::MultiLineEditbox* editbox = static_cast<CEGUI::MultiLineEditbox*>(mGuiSystem->getGUISheet()->getActiveChild());
	mSelectedText = &editbox->getText();
	mSelectionStart = editbox->getSelectionStartIndex();
	mSelectionEnd = editbox->getSelectionEndIndex();
/*	const CEGUI::String& text = editbox->getText();
	if (editbox->getSelectionLength() > 0) {
		std::string selection = text.substr(editbox->getSelectionStartIndex(), editbox->getSelectionEndIndex()).c_str();
		S_LOG_VERBOSE("Selected text: " << selection);
	}*/
//	S_LOG_VERBOSE("Selected text.");

	return true;
}

bool GUICEGUIAdapter::Editbox_selectionChangedHandler(const CEGUI::EventArgs& args)
{
	CEGUI::Editbox* editbox = static_cast<CEGUI::Editbox*>(mGuiSystem->getGUISheet()->getActiveChild());
	mSelectedText = &editbox->getText();
	mSelectionStart = editbox->getSelectionStartIndex();
	mSelectionEnd = editbox->getSelectionEndIndex();
	S_LOG_VERBOSE("Selected text.");
	return true;
}


bool GUICEGUIAdapter::injectMouseMove(const MouseMotion& motion, bool& freezeMouse)
{
	try {
		mGuiSystem->injectMousePosition(motion.xPosition, motion.yPosition);
	} catch (const CEGUI::Exception& ex) {
		S_LOG_WARNING("Error in CEGUI." << ex);
	}
	return true;
}

bool GUICEGUIAdapter::injectMouseButtonUp(const Input::MouseButton& button)
{
	CEGUI::MouseButton ceguiButton;
	if (button == Input::MouseButtonLeft) {
		ceguiButton = CEGUI::LeftButton;
	} else if(button == Input::MouseButtonRight) {
		ceguiButton = CEGUI::RightButton;
	} else if(button == Input::MouseButtonMiddle) {
		ceguiButton = CEGUI::MiddleButton;
	} else {
		return true;
	}

	try {
		mGuiSystem->injectMouseButtonUp(ceguiButton);
		return false;
	} catch (const std::exception& e) {
		S_LOG_WARNING("Error in CEGUI." << e);
	} catch (...) {
		S_LOG_WARNING("Unknown error in CEGUI.");
	}
	return true;
}

bool GUICEGUIAdapter::injectMouseButtonDown(const Input::MouseButton& button)
{
	CEGUI::MouseButton ceguiButton(CEGUI::LeftButton);
	if (button == Input::MouseButtonLeft) {
		ceguiButton = CEGUI::LeftButton;
	} else if(button == Input::MouseButtonRight) {
		ceguiButton = CEGUI::RightButton;
	} else if(button == Input::MouseButtonMiddle) {
		ceguiButton = CEGUI::MiddleButton;
	} else if(button == Input::MouseWheelDown) {
		try {
			mGuiSystem->injectMouseWheelChange(-1.0);
		} catch (const CEGUI::Exception& ex) {
			S_LOG_WARNING("Error in CEGUI." << ex);
		}
		return false;
	} else if(button == Input::MouseWheelUp) {
		try {
			mGuiSystem->injectMouseWheelChange(1.0);
		} catch (const CEGUI::Exception& ex) {
			S_LOG_WARNING("Error in CEGUI." << ex);
		}
		return false;
	} else {
		return true;
	}

	try {
		mGuiSystem->injectMouseButtonDown(ceguiButton);
		return false;
	} catch (const CEGUI::Exception& ex) {
		S_LOG_WARNING("Error in CEGUI." << ex);
	}
	return true;
}

bool GUICEGUIAdapter::injectChar(int character)
{
	try {
		//cegui can't handle tabs, so we have to convert it to a couple of spaces
		if (character == '\t') {
			mGuiSystem->injectChar(' ');
			mGuiSystem->injectChar(' ');
			mGuiSystem->injectChar(' ');
			mGuiSystem->injectChar(' ');
		//can't handle CR either really, insert a line break (0x0a) instead
		} else if (character == '\r') {
 			//mGuiSystem->injectChar(0x0a);
 			mGuiSystem->injectKeyDown(CEGUI::Key::Return);
 			mGuiSystem->injectKeyUp(CEGUI::Key::Return);
		} else {
			mGuiSystem->injectChar(character);
		}
	} catch (const CEGUI::Exception& ex) {
		S_LOG_WARNING("Error in CEGUI." << ex);
	}
	return true;

}

bool GUICEGUIAdapter::injectKeyDown(const OIS::KeyCode& key)
{
	try {
		OISKeyMap::const_iterator I =  mKeyMap.find(key);
		if (I != mKeyMap.end())  {
			unsigned int scanCode = I->second;
			mGuiSystem->injectKeyDown(scanCode);
		}
	} catch (const CEGUI::Exception& ex) {
		S_LOG_WARNING("Error in CEGUI." << ex);
	}
	return true;

}

bool GUICEGUIAdapter::injectKeyUp(const OIS::KeyCode& key)
{
	try {
		OISKeyMap::const_iterator I =  mKeyMap.find(key);
		if (I != mKeyMap.end())  {
			unsigned int scanCode = I->second;
			mGuiSystem->injectKeyUp(scanCode);
		}
	} catch (const CEGUI::Exception& ex) {
		S_LOG_WARNING("Error in CEGUI." << ex);
	}
	return true;
}



}
}
