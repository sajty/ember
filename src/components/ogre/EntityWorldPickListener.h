//
// C++ Interface: EntityWorldPickListener
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
#ifndef EMBEROGREENTITYWORLDPICKLISTENER_H
#define EMBEROGREENTITYWORLDPICKLISTENER_H

#include "EmberOgrePrerequisites.h"
#include "IWorldPickListener.h"
#include <sigc++/signal.h>
#include "framework/ConsoleObject.h"
#include <memory>
#include <vector>
#include <OgreVector3.h>

namespace Eris
{
class View;
}

namespace Ember
{
namespace OgreView
{

class EmberEntity;
class EntityWorldPickListener;
class EmberEntityFactory;
class Scene;
/**
 * @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 * @brief Struct used for returning the result of a mouse pick.
 */
struct EntityPickResult
{
	EmberEntity* entity;
	Ogre::Vector3 position;
	Ogre::Real distance;
	bool isTransparent;
};

/**
 * @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 * @brief Visualizes the picking operation by placing a large ball at the picked position.
 */
class EntityWorldPickListenerVisualizer: public virtual sigc::trackable
{
public:
	EntityWorldPickListenerVisualizer(EntityWorldPickListener& pickListener, Ogre::SceneManager& sceneManager);
	virtual ~EntityWorldPickListenerVisualizer();

private:
	Ogre::Entity* mEntity;
	Ogre::SceneNode* mDebugNode;
	void picker_EventPickedEntity(const std::vector<EntityPickResult>& result, const MousePickerArgs& mouseArgs);
};

/**
 * @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 * @brief Listens for mouse picking of entities in the world.
 *
 * An instance of this is used to allow for picking of entities and terrain in the world.
 */
class EntityWorldPickListener: public IWorldPickListener, public ConsoleObject
{
public:
	EntityWorldPickListener(Eris::View& view, Scene& scene);

	virtual ~EntityWorldPickListener();

	virtual void initializePickingContext(bool& willParticipate, unsigned int& queryMask, const MousePickerArgs& pickArgs);

	virtual void endPickingContext(const MousePickerArgs& mousePickerArgs);

	virtual void processPickResult(bool& continuePicking, Ogre::RaySceneQueryResultEntry& entry, Ogre::Ray& cameraRay, const MousePickerArgs& mousePickerArgs);

	sigc::signal<void, const std::vector<EntityPickResult>&, const MousePickerArgs&> EventPickedEntity;

	const ConsoleCommandWrapper VisualizePicking;

	/**
	 *    Reimplements the ConsoleObject::runCommand method
	 * @param command
	 * @param args
	 */
	virtual void runCommand(const std::string &command, const std::string &args);

protected:
	float mClosestPickingDistance, mFurthestPickingDistance;
	std::vector<EntityPickResult> mResult;

	std::unique_ptr<EntityWorldPickListenerVisualizer> mVisualizer;

	Eris::View& mView;

	Scene& mScene;

};

}

}

#endif
