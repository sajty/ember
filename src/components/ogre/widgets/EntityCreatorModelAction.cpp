//
// C++ Implementation: EntityCreatorModelAction
//
// Description:
//
//
// Author: Erik Ogenvik <erik@ogenvik.org>, (C) 2009
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "EntityCreatorModelAction.h"

#include "EntityCreatorCreationInstance.h"
#include "framework/LoggingInstance.h"

namespace Ember {
namespace OgreView {

namespace Gui {

EntityCreatorModelAction::EntityCreatorModelAction(EntityCreatorCreationInstance& creationInstance, const std::string& modelName)
		: mCreationInstance(creationInstance), mModelName(modelName)
{
}

EntityCreatorModelAction::~EntityCreatorModelAction()
{
}

void EntityCreatorModelAction::activate(EntityMapping::ChangeContext& context)
{
	S_LOG_VERBOSE("Showing creator model " << mModelName);
	mCreationInstance.setModel(mModelName);
}

void EntityCreatorModelAction::deactivate(EntityMapping::ChangeContext& context)
{
	S_LOG_VERBOSE("Hiding creator model " << mModelName);
	mCreationInstance.setModel("");
}

}

}
}
