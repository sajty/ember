//
// C++ Interface: EntityMover
//
// Description:
//
//
// Author: Erik Ogenvik <erik@ogenvik.org>, (C) 2006
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
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.//
//
#ifndef EMBEROGREENTITYMOVER_H
#define EMBEROGREENTITYMOVER_H

#include "EntityMoverBase.h"
#include "components/ogre/EmberOgrePrerequisites.h"
#include "domain/IEntityControlDelegate.h"

#include <wfmath/point.h>

namespace Ember
{
namespace OgreView
{
class NodeAttachment;

namespace Authoring
{

class SnapToMovement;

class EntityMoveManager;

/**
 @author Erik Ogenvik <erik@ogenvik.org>

 @brief An implementation of IEntityMoveBridge which allows for EmberEntity instances to be moved.
 */
class EntityMover: public Authoring::EntityMoverBase
{
public:

	EntityMover(NodeAttachment& NodeAttachment, EntityMoveManager& manager);
	virtual ~EntityMover();

	virtual void finalizeMovement();
	virtual void cancelMovement();

	const IEntityControlDelegate& getControlDelegate() const;

protected:

	virtual void newEntityPosition(const Ogre::Vector3& position);

	void cleanup();

private:

	NodeAttachment& mNodeAttachment;
	EntityMoveManager& mManager;

	IEntityControlDelegate* mPreviousControlDelegate;
	IEntityControlDelegate* mControlDelegate;

};

class EntityMoverControlDelegate: public IEntityControlDelegate
{
public:

	EntityMoverControlDelegate(EntityMover& entityMover);

	virtual const WFMath::Point<3>& getPosition() const;
	virtual const WFMath::Quaternion& getOrientation() const;
	virtual const WFMath::Vector<3>& getVelocity() const;

protected:
	EntityMover& mEntityMover;

};

}
}

}

#endif
