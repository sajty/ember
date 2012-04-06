//
// C++ Implementation: EntityMoveManager
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

#include "EntityMoveManager.h"
#include "components/ogre/World.h"
#include "MovementAdapter.h"
#include "EntityMover.h"
#include "../GUIManager.h"
#include "framework/Tokeniser.h"
#include "framework/ConsoleBackend.h"
#include "../EmberOgre.h"
#include "../EmberEntity.h"
#include "components/ogre/NodeAttachment.h"

namespace Ember
{
namespace OgreView
{

namespace Authoring
{

EntityMoveInstance::EntityMoveInstance(EmberEntity& entity, MovementAdapter& moveAdapter, sigc::signal<void>& eventFinishedMoving, sigc::signal<void>& eventCancelledMoving) :
	EntityObserverBase(entity, true), mMoveAdapter(moveAdapter)
{
	eventCancelledMoving.connect(sigc::mem_fun(*this, &EntityObserverBase::deleteOurselves));
	eventFinishedMoving.connect(sigc::mem_fun(*this, &EntityObserverBase::deleteOurselves));
}

EntityMoveInstance::~EntityMoveInstance()
{
}

void EntityMoveInstance::cleanup()
{
	mMoveAdapter.detach();
}

EntityMoveManager::EntityMoveManager(World& world) :
	Move("move", this, "Moves an entity."), mWorld(world), mMoveAdapter(world.getMainCamera()), mAdjuster(this)
{
	GUIManager::getSingleton().EventEntityAction.connect(sigc::mem_fun(*this, &EntityMoveManager::GuiManager_EntityAction));
}

void EntityMoveManager::GuiManager_EntityAction(const std::string& action, EmberEntity* entity)
{

	if (action == "move") {
		startMove(*entity);
	}
}

void EntityMoveManager::startMove(EmberEntity& entity)
{
	//disallow moving of the root entity
	if (entity.getLocation()) {
		//Only provide movement for entities which have a node attachment.
		NodeAttachment* attachment = dynamic_cast<NodeAttachment*> (entity.getAttachment());
		if (attachment) {
			EntityMover* mover = new EntityMover(*attachment, *this);
			mMoveAdapter.attachToBridge(mover);
			//The EntityMoveInstance will delete itself when either movement is finished or the entity is deleted, so we don't need to hold a reference to it.
			new EntityMoveInstance(entity, mMoveAdapter, EventFinishedMoving, EventCancelledMoving);
			EventStartMoving.emit(entity, *mover);
		}
	}
}

void EntityMoveManager::runCommand(const std::string &command, const std::string &args)
{
	if (Move == command) {
		//the first argument must be a valid entity id
		Tokeniser tokeniser;
		tokeniser.initTokens(args);
		std::string entityId = tokeniser.nextToken();
		if (entityId != "") {
			EmberEntity* entity = mWorld.getEmberEntity(entityId);
			if (entity != 0) {
				startMove(*entity);
			}
		} else {
			ConsoleBackend::getSingleton().pushMessage("You must specify a valid entity id to move.", "error");
		}

	}
}

World& EntityMoveManager::getWorld() const
{
	return mWorld;
}


}

}
}
