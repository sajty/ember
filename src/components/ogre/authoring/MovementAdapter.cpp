//
// C++ Implementation: MovementAdapter
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

#include "MovementAdapter.h"
#include "IMovementBridge.h"
#include "EntityMoveManager.h"
#include "components/ogre/camera/MainCamera.h"
#include "../AvatarTerrainCursor.h"
#include "../Convert.h"
#include <OgreRoot.h>

using namespace WFMath;
using namespace Ember;

namespace Ember
{
namespace OgreView
{
namespace Authoring
{

MovementAdapterWorkerBase::MovementAdapterWorkerBase(MovementAdapter& adapter) :
	mAdapter(adapter)
{
}

MovementAdapterWorkerBase::~MovementAdapterWorkerBase()
{
}

IMovementBridge* MovementAdapterWorkerBase::getBridge()
{
	return mAdapter.mBridge;
}

const Camera::MainCamera& MovementAdapterWorkerBase::getCamera() const
{
	return mAdapter.mCamera;
}


MovementAdapterWorkerDiscrete::MovementAdapterWorkerDiscrete(MovementAdapter& adapter) :
	MovementAdapterWorkerBase(adapter), mMovementSpeed(10)
{
}

bool MovementAdapterWorkerDiscrete::injectMouseMove(const MouseMotion& motion, bool& freezeMouse)
{
	//this will move the entity instead of the mouse

	Vector<3> direction;
	direction.zero();
	direction.x() = -motion.xRelativeMovement;
	direction.y() = motion.yRelativeMovement;
	direction = direction * mMovementSpeed;
	//hard coded to allow the shift button to increase the speed
	// 	if (Input::getSingleton().isKeyDown(SDLK_RSHIFT) || Input::getSingleton().isKeyDown(SDLK_LSHIFT)) {
	// 		direction = direction * 5;
	// 	}

	Quaternion orientation = Convert::toWF(getCamera().getOrientation());

	//We need to constraint the orientation to only around the z axis.
	WFMath::Vector<3> rotator(1.0, 0.0, 0.0);
	rotator.rotate(orientation);
	WFMath::Quaternion adjustedOrientation;
	adjustedOrientation.fromRotMatrix(WFMath::RotMatrix<3>().rotationZ(atan2(rotator.y(), rotator.x())));

	orientation = adjustedOrientation;

	//move it relative to the camera
	direction = direction.rotate(orientation);

	getBridge()->move(direction);//move the entity a fixed distance for each mouse movement.

	//we don't want to move the cursor
	freezeMouse = true;

	return false;

}

MovementAdapterWorkerTerrainCursor::MovementAdapterWorkerTerrainCursor(MovementAdapter& adapter) :
	MovementAdapterWorkerBase(adapter)
{
	// Register this as a frame listener
	Ogre::Root::getSingleton().addFrameListener(this);
}

MovementAdapterWorkerTerrainCursor::~MovementAdapterWorkerTerrainCursor()
{
	Ogre::Root::getSingleton().removeFrameListener(this);
}

void MovementAdapterWorkerTerrainCursor::update()
{
	updatePosition(true);
}

bool MovementAdapterWorkerTerrainCursor::frameStarted(const Ogre::FrameEvent& event)
{
	updatePosition();
	return true;
}

void MovementAdapterWorkerTerrainCursor::updatePosition(bool forceUpdate)
{
	const Ogre::Vector3* position(0);
	if (getCamera().getTerrainCursor().getTerrainCursorPosition(&position) || forceUpdate) {
		getBridge()->setPosition(Convert::toWF<WFMath::Point<3>>(*position));
	}
}

MovementAdapter::MovementAdapter(const Camera::MainCamera& camera) :
	mCamera(camera), mBridge(0), mWorker(0)
{
}

MovementAdapter::~MovementAdapter()
{
	//detach(); //A call to this will delete both the bridge and the adapter.
}

void MovementAdapter::finalizeMovement()
{
	removeAdapter();
	//We need to do it this way since there's a chance that the call to IMovementBridge::finalizeMovement will delete this instance, and then we can't reference mBridge anymore
	IMovementBridge* bridge = mBridge;
	mBridge = 0;
	bridge->finalizeMovement();
	delete bridge;
}

void MovementAdapter::cancelMovement()
{
	removeAdapter();
	//We need to do it this way since there's a chance that the call to IMovementBridge::cancelMovement will delete this instance, and then we can't reference mBridge anymore
	IMovementBridge* bridge = mBridge;
	mBridge = 0;
	bridge->cancelMovement();
	delete bridge;
}

bool MovementAdapter::injectMouseMove(const MouseMotion& motion, bool& freezeMouse)
{
	if (mWorker) {
		return mWorker->injectMouseMove(motion, freezeMouse);
	}
	return true;
}

bool MovementAdapter::injectMouseButtonUp(const Input::MouseButton& button)
{
	if (button == Input::MouseButtonLeft) {
		finalizeMovement();
		//After we've finalized we've done here, so we should let other IInputAdapters handle the mouse button up (we've had an issue where cegui didn't receive the mouse up event, which made it think that icons that were dragged were still being dragged (as the end-drag event is emitted on mouse up))
		return true;
	} else if (button == Input::MouseButtonRight) {
	} else {
		return false;
	}

	return false;
}

bool MovementAdapter::injectMouseButtonDown(const Input::MouseButton& button)
{
	if (button == Input::MouseButtonLeft) {
	} else if (button == Input::MouseButtonRight) {

	} else if (button == Input::MouseButtonMiddle) {

	} else if (button == Input::MouseWheelUp) {
		int movementDegrees = 10;
		if (Input::getSingleton().isKeyDown(SDLK_LSHIFT) || Input::getSingleton().isKeyDown(SDLK_RSHIFT)) {
			movementDegrees = 1;
		}
		mBridge->yaw(movementDegrees);
	} else if (button == Input::MouseWheelDown) {
		int movementDegrees = 10;
		if (Input::getSingleton().isKeyDown(SDLK_LSHIFT) || Input::getSingleton().isKeyDown(SDLK_RSHIFT)) {
			movementDegrees = 1;
		}
		mBridge->yaw(-movementDegrees);
	}

	return false;
}

bool MovementAdapter::injectChar(char character)
{
	return true;
}

bool MovementAdapter::injectKeyDown(const SDLKey& key)
{
	if (mWorker) {
		//by pressing and holding shift we'll allow the user to position it with more precision. We do this by switching the worker instances.
		if (key == SDLK_LSHIFT || key == SDLK_RSHIFT) {
			delete mWorker;
			mWorker = new MovementAdapterWorkerDiscrete(*this);
		}
	}
	return true;
}

bool MovementAdapter::injectKeyUp(const SDLKey& key)
{
	if (key == SDLK_ESCAPE) {
		cancelMovement();
		return false;
	} else if (key == SDLK_LSHIFT || key == SDLK_RSHIFT) {
		if (mWorker) {
			delete mWorker;
			mWorker = new MovementAdapterWorkerTerrainCursor(*this);
		}
	}

	return true;
}

void MovementAdapter::attachToBridge(IMovementBridge* bridge)
{
	if (mBridge != bridge) {
		delete mBridge;
	}
	mBridge = bridge;
	addAdapter();
}

void MovementAdapter::detach()
{
	delete mBridge;
	mBridge = 0;
	removeAdapter();
}

void MovementAdapter::removeAdapter()
{
	Input::getSingleton().removeAdapter(this);
	delete mWorker;
	mWorker = 0;
}

void MovementAdapter::addAdapter()
{
	Input::getSingleton().addAdapter(this);
	//default to the terrain cursor positioning mode
	mWorker = new MovementAdapterWorkerTerrainCursor(*this);
}

void MovementAdapter::update()
{
	if (mWorker) {
		mWorker->update();
	}
}

}

}
}
