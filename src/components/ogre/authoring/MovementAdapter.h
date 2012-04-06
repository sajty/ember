//
// C++ Interface: MovementAdapter
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
#ifndef EMBEROGREMovementADAPTER_H
#define EMBEROGREMovementADAPTER_H

#include "../EmberOgrePrerequisites.h"
#include "services/input/IInputAdapter.h"
#include <OgreFrameListener.h>

namespace Ember
{
namespace OgreView
{
namespace Camera {
class MainCamera;
}
namespace Authoring
{
class IMovementBridge;
class EntityMoveManager;
class MovementAdapter;

/**
 @brief Base class for all movement adapter worker classes.

 Implementations of this are responsible for the actual checking of input data to determine whether the bridge should be told to update or not.
 @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 */
class MovementAdapterWorkerBase
{
public:
	MovementAdapterWorkerBase(MovementAdapter& adapter);
	virtual ~MovementAdapterWorkerBase();

	virtual bool injectMouseMove(const MouseMotion& motion, bool& freezeMouse)
	{
		return true;
	}

	/**
	 * @brief Forces an update.
	 * This could be called to force a position update, even if no input has been received (for example a mouse movement).
	 */
	virtual void update()
	{
	}
	;
protected:

	/**
	 *    Gets the movement bridge in use.
	 * @return
	 */
	IMovementBridge* getBridge();

	/**
	 * @brief Gets the main world camera.
	 * @returns The main world camera.
	 */
	const Camera::MainCamera& getCamera() const;

	MovementAdapter& mAdapter;

};

/**
 @brief An adapter worker implementation which will move the entity a fixed distance for each mouse movement.
 @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 */
class MovementAdapterWorkerDiscrete: public MovementAdapterWorkerBase
{
public:
	MovementAdapterWorkerDiscrete(MovementAdapter& adapter);

	virtual bool injectMouseMove(const MouseMotion& motion, bool& freezeMouse);

protected:

	float mMovementSpeed;

};

/**
 @brief An adapter worker implementation which will always position the entity where the mouse cursor intersects with the terrain.
 @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 */
class MovementAdapterWorkerTerrainCursor: public MovementAdapterWorkerBase, public Ogre::FrameListener
{
public:
	MovementAdapterWorkerTerrainCursor(MovementAdapter& adapter);
	virtual ~MovementAdapterWorkerTerrainCursor();

	/**
	 * Methods from Ogre::FrameListener
	 */
	bool frameStarted(const Ogre::FrameEvent& event);

	/**
	 * @brief Forces an update.
	 * This could be called to force a position update, even if no input has been received (for example a mouse movement).
	 */
	virtual void update();

protected:

	/**
	 * @brief Updates the position from the placement of the cursor on the terrain.
	 * @param forceUpdate True if the position should be updated even if the mouse hasn't been moved.
	 */
	void updatePosition(bool forceUpdate = false);

};

/**
 @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 @author Lennart Sauerbeck

 @brief Provides an adapter for moving objects in the world.

 When activates, an instance of this will receive input events and pass these on to the currently selected instance of MovementAdapterWorkerBase which in turn will translate those input operations into movement of any object movements, as defined in the bridge to which this is attached.
 */
class MovementAdapter: public IInputAdapter
{
	friend class MovementAdapterWorkerBase;
public:

	/**
	 * @brief Ctor.
	 * @param camera The main camera used in the world.
	 */
	MovementAdapter(const Camera::MainCamera& camera);
	virtual ~MovementAdapter();

	virtual bool injectMouseMove(const MouseMotion& motion, bool& freezeMouse);
	virtual bool injectMouseButtonUp(const Input::MouseButton& button);
	virtual bool injectMouseButtonDown(const Input::MouseButton& button);
	virtual bool injectChar(char character);
	virtual bool injectKeyDown(const OIS::KeyCode& key);
	virtual bool injectKeyUp(const OIS::KeyCode& key);

	/**
	 * @brief Attaches the adapter to the suppied IMovementBridge, allowing it to be moved. This will activate the adapter.
	 * Ownership of the bridge will be passed to this class.
	 * @param bridge The bridge through which the entity is moved. After calling this, ownership will be transferred to this class.
	 */
	virtual void attachToBridge(IMovementBridge* bridge);

	/**
	 * @brief Detaches the adapter from the current bridge. This will deactive the adapter.
	 * At detachment the bridge will also be deleted. Call attachToBridge to attach to a new bridge if you want to move another entity.
	 */
	virtual void detach();

	/**
	 * @brief Forces an update.
	 * This could be called to force a position update, even if no input has been received (for example a mouse movement).
	 */
	virtual void update();

protected:
	/**
	 * @brief Removes this instance from the input system, which means that it will no longer receive any calls to the inject* methods.
	 */
	virtual void removeAdapter();

	/**
	 * @brief Adds this instance as an adapter to the input system, which means that it will begin receiving calls to the inject* methods.
	 */
	virtual void addAdapter();

	/**
	 * @brief Cancels the current movement, returning the IMovementBridge to it's original place.
	 */
	virtual void cancelMovement();

	/**
	 * @brief Finalizes the current movement, sending updates for the IMovementBridge to the server.
	 */
	virtual void finalizeMovement();

	/**
	 * @brief The main camera used in the scene.
	 */
	const Camera::MainCamera& mCamera;

	/**
	 * @brief The bridge through which all movement happens.
	 * This is initially null, but is set through attachToBridge. Once a bridge has been attached the ownership is transferred to this class, and subsequently the reponsibility to delete it.
	 */
	IMovementBridge* mBridge;

	/**
	 * @brief The worker instance which will listen for inputs and tell the bridge to update accordingly.
	 */
	MovementAdapterWorkerBase* mWorker;

};

}

}

}

#endif
