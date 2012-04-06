//
// C++ Implementation: MovableObjectRenderer
//
// Description:
//
//
// Author: Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2006
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

#include "MovableObjectRenderer.h"

#include "EntityCEGUITexture.h"
#include "../SimpleRenderContext.h"

#include <elements/CEGUIGUISheet.h>
#include <CEGUIPropertyHelper.h>

#include "framework/Exception.h"

#include "Widget.h"
#include "../GUIManager.h"

#include <OgreRoot.h>
#include <OgreSceneNode.h>
#include <OgreEntity.h>
#include <OgreRenderTexture.h>
#include <OgreTexture.h>
#include <OgreRenderTargetListener.h>

using namespace Ember;
namespace Ember {
namespace OgreView {
namespace Gui {

/**
 * @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 * @brief Responsible for invalidating a CEGUI Window whenever something is drawn to a viewport which the window is showing.
 *
 * This class shouldn't be needed though as with CEGUI 0.7 there's a way to make CEGUI directly use an Ogre render texture.
 */
class CEGUIWindowUpdater : public Ogre::RenderTargetListener
{
protected:
	CEGUI::Window& mWindow;
public:
	CEGUIWindowUpdater(CEGUI::Window& window) : mWindow(window)
	{
	}

	virtual void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
	{
		mWindow.invalidate();
	}
};

MovableObjectRenderer::MovableObjectRenderer(CEGUI::Window* image)
: mTexture(0), mIsInputCatchingAllowed(true), mAutoShowFull(true), mImage(image), mActive(true), mAxisEntity(0), mAxesNode(0), mWindowUpdater(0)
{
	int width = static_cast<int>(image->getPixelSize().d_width);
	int height = static_cast<int>(image->getPixelSize().d_height);
	if (width != 0 && height != 0) {
		mTexture = new EntityCEGUITexture(image->getName().c_str(), width, height);
		//most models are rotated away from the camera, so as a convenience we'll rotate the node
		//mTexture->getSceneNode()->rotate(Ogre::Vector3::UNIT_Y,(Ogre::Degree)180);

		mImage->setProperty("Image", CEGUI::PropertyHelper::imageToString(mTexture->getImage()));
		//mImage->setImageColours(CEGUI::colour(1.0f, 1.0f, 1.0f));
		BIND_CEGUI_EVENT(mImage, CEGUI::Window::EventMouseButtonDown, MovableObjectRenderer::image_MouseButtonDown);
		BIND_CEGUI_EVENT(mImage, CEGUI::Window::EventMouseWheel, MovableObjectRenderer::image_MouseWheel);


		// Register this as a frame listener
		Ogre::Root::getSingleton().addFrameListener(this);
		mWindowUpdater = new CEGUIWindowUpdater(*mImage);
		mTexture->getRenderContext()->getRenderTexture()->addListener(mWindowUpdater);

		//Render a blank scene to start with, else we'll get uninitialized buffer garbage shown.
		mTexture->getRenderContext()->getRenderTexture()->update();
	} else {
		throw Exception("Image dimension cannot be 0.");
	}
}


MovableObjectRenderer::~MovableObjectRenderer()
{
	if (mImage) {
		mImage->setProperty("Image", "");
	}
	if (mTexture && mWindowUpdater) {
		mTexture->getRenderContext()->getRenderTexture()->removeListener(mWindowUpdater);
	}

	delete mTexture;
	delete mWindowUpdater;
	// Register this as a frame listener
	Ogre::Root::getSingleton().removeFrameListener(this);

}

bool MovableObjectRenderer::injectMouseMove(const MouseMotion& motion, bool& freezeMouse)
{
	//rotate the modelnode
	if (Input::getSingleton().isModifierDown(OIS::Keyboard::Ctrl)) {
		mTexture->getRenderContext()->roll(Ogre::Degree(motion.xRelativeMovement * 180));
	} else {
		mTexture->getRenderContext()->yaw(Ogre::Degree(motion.xRelativeMovement * 180));
		mTexture->getRenderContext()->pitch(Ogre::Degree(motion.yRelativeMovement * 180));
	}
	//we don't want to move the cursor
	freezeMouse = true;
	return false;
}

Ogre::Quaternion MovableObjectRenderer::getEntityRotation()
{
	return mTexture->getRenderContext()->getEntityRotation();
}

void MovableObjectRenderer::resetCameraOrientation()
{
	mTexture->getRenderContext()->resetCameraOrientation();
}

void MovableObjectRenderer::pitch(Ogre::Degree degrees)
{
	mTexture->getRenderContext()->pitch(degrees);
}

void MovableObjectRenderer::yaw(Ogre::Degree degrees)
{
	mTexture->getRenderContext()->yaw(degrees);
}

void MovableObjectRenderer::roll(Ogre::Degree degrees)
{
	mTexture->getRenderContext()->roll(degrees);
}


bool MovableObjectRenderer::injectMouseButtonUp(const Input::MouseButton& button)
{
	if (button == Input::MouseButtonLeft) {
		releaseInput();
		//we need to return false, because we have removed the current interface and we are iterating on it.
		return false;
	}
	return true;
}

bool MovableObjectRenderer::injectMouseButtonDown(const Input::MouseButton& button)
{
	return true;
}

bool MovableObjectRenderer::injectChar(int character)
{
	return true;
}

bool MovableObjectRenderer::injectKeyDown(const OIS::KeyCode& key)
{
	return true;
}

bool MovableObjectRenderer::injectKeyUp(const OIS::KeyCode& key)
{
	return true;
}

bool MovableObjectRenderer::getIsInputCatchingAllowed() const
{
	return mIsInputCatchingAllowed;
}

void MovableObjectRenderer::setIsInputCatchingAllowed(bool allowed)
{
	mIsInputCatchingAllowed = allowed;
}

bool MovableObjectRenderer::getAutoShowFull() const
{
	return mAutoShowFull;
}

void MovableObjectRenderer::setAutoShowFull(bool showFull)
{
	mAutoShowFull = showFull;
}

void MovableObjectRenderer::showFull()
{
//	if (mModel) {
		mTexture->getRenderContext()->showFull(getMovableObject());
//	}
}

void MovableObjectRenderer::setCameraDistance(float distance)
{

	mTexture->getRenderContext()->setCameraDistance(mTexture->getRenderContext()->getDefaultCameraDistance() * distance);
/*	Ogre::Vector3 position = mTexture->getDefaultCameraPosition();
	position.z *= distance;
	mTexture->getCamera()->setPosition(position);*/
}

float MovableObjectRenderer::getCameraDistance()
{
	return  mTexture->getRenderContext()->getCameraDistance();
}

float MovableObjectRenderer::getAbsoluteCameraDistance()
{
	return  mTexture->getRenderContext()->getAbsoluteCameraDistance();
}

void MovableObjectRenderer::catchInput()
{
	Input::getSingleton().addAdapter(this);
}

void MovableObjectRenderer::releaseInput()
{
	Input::getSingleton().removeAdapter(this);
}

bool MovableObjectRenderer::image_MouseWheel(const CEGUI::EventArgs& args)
{
	const CEGUI::MouseEventArgs& mouseArgs = static_cast<const CEGUI::MouseEventArgs&>(args);

	if (mTexture) {
		if (mouseArgs.wheelChange != 0.0f) {
			float distance = mTexture->getRenderContext()->getCameraDistance();
			distance += (mouseArgs.wheelChange * 0.1);
			setCameraDistance(distance);
		}
	}

	return true;
}


bool MovableObjectRenderer::image_MouseButtonDown(const CEGUI::EventArgs& args)
{
	const CEGUI::MouseEventArgs& mouseArgs = static_cast<const CEGUI::MouseEventArgs&>(args);
	if (mouseArgs.button == CEGUI::LeftButton) {
		//only catch input if it's allowed
		if (getIsInputCatchingAllowed()) {
			catchInput();
		}
	}
	return true;
}

bool MovableObjectRenderer::frameStarted(const Ogre::FrameEvent& event)
{
//	S_LOG_VERBOSE(mImage->getName().c_str() << " visible: " << (mActive && mImage->isVisible()));
	//if the window isn't shown, don't update the render texture
	mTexture->getRenderContext()->setActive(mActive && mImage->isVisible());
	if (mActive && mImage->isVisible()) {
		updateRender();
	}
	return true;
}

void MovableObjectRenderer::updateRender()
{
	try {
		if (mTexture->getRenderContext()->getRenderTexture()) {
			mTexture->getRenderContext()->getRenderTexture()->update();
		}
	} catch (const std::exception& ex) {
		S_LOG_FAILURE("Error when updating render for MovableObjectRenderer." << ex);
	}
}

void MovableObjectRenderer::setBackgroundColour(const Ogre::ColourValue& colour)
{
	mTexture->getRenderContext()->setBackgroundColour(colour);
}

void MovableObjectRenderer::setBackgroundColour(float red, float green, float blue, float alpha)
{
	mTexture->getRenderContext()->setBackgroundColour(red, green, blue, alpha);
}

void MovableObjectRenderer::showAxis()
{
	if (!mAxesNode) {
		mAxesNode = mTexture->getRenderContext()->getSceneManager()->getRootSceneNode()->createChildSceneNode();
	}
	if (!mAxisEntity) {
		std::string name(mImage->getName().c_str());
		try {
			mAxisEntity = mTexture->getRenderContext()->getSceneManager()->createEntity(name + "_axes", "axes.mesh");
			if (mAxisEntity) {
				try {
					mAxesNode->attachObject(mAxisEntity);
				} catch (const std::exception& ex) {
					S_LOG_WARNING("Error when attaching axes mesh."<< ex);
				}
			}
		} catch (const std::exception& ex) {
			S_LOG_WARNING("Error when loading axes mesh."<< ex);
		}
	}
	mAxesNode->setVisible(true);
}

void MovableObjectRenderer::hideAxis()
{
	if (mAxesNode) {
		mAxesNode->setVisible(false);
	}
}

SimpleRenderContext::CameraPositioningMode MovableObjectRenderer::getCameraPositionMode() const
{
	return mTexture->getRenderContext()->getCameraPositionMode();
}

void MovableObjectRenderer::setCameraPositionMode(SimpleRenderContext::CameraPositioningMode mode)
{
	mTexture->getRenderContext()->setCameraPositionMode(mode);
}

}
}
}
