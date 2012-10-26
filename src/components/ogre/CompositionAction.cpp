/*
 Copyright (C) 2012 Erik Ogenvik

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "CompositionAction.h"
#include "EmberEntity.h"

namespace Ember
{
namespace OgreView
{

CompositionAction::CompositionAction(EmberEntity& entity, const std::string& mode) :
		mEntity(entity), mMode(mode)
{
}

void CompositionAction::activate(EntityMapping::ChangeContext& context)
{
	EmberEntity::CompositionMode compositionMode = EmberEntity::CM_COMPOSITION;
	if (mMode == "none") {
		compositionMode = EmberEntity::CM_DISABLED;
	} else if (mMode == "exclusive") {
		compositionMode = EmberEntity::CM_COMPOSITION_EXCLUSIVE;
	}
	mEntity.setCompositionMode(compositionMode);
}

void CompositionAction::deactivate(EntityMapping::ChangeContext& context)
{
}

}
}
