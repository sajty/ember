//
// C++ Interface: EntityMappingManager
//
// Description:
//
//
// Author: Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2007
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
#ifndef EMBEROGRE_MODEL_MAPPINGMODELMAPPINGMANAGER_H
#define EMBEROGRE_MODEL_MAPPINGMODELMAPPINGMANAGER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <vector>
#include <unordered_map>

#include <Eris/TypeInfo.h>
#include <Eris/Entity.h>

#include "Definitions/EntityMappingDefinition.h"
#include "EntityMapping.h"


namespace Ember {



namespace EntityMapping {

class EntityMapping;
class IActionCreator;



/**
	Handles all EntityMapping instances, as well as creation and setup.

	Applications are expected to add definitions to the manager through the addDefinition(...) method. Definitions are managed by the manager and will be deleted by this upon destruction.
	New EntityMapping instances are created by calling createMapping(...). It's up to the application to delete all EntityMapping instances created by the manager.

	@author Erik Hjortsberg <erik.hjortsberg@gmail.com>
*/
class EntityMappingManager{
public:
	typedef std::unordered_map<std::string, Definitions::EntityMappingDefinition*> EntityMappingDefinitionStore;

	/**
	Default constructor.
	*/
    EntityMappingManager();

    ~EntityMappingManager();

    /**
    Sets the type service. Applications are required to set this before calling createMapping(...)
    @param typeService An Eris::TypeService instance.
    */
    void setTypeService(Eris::TypeService* typeService);

    /**
    Adds a definition to the manager. This definition will be deleted by the manager upon destruction.
    @param definition A valid definition.
    */
    void addDefinition(Definitions::EntityMappingDefinition* definition);


    /**
    Queries the internal list of definitions and return the defintion that's most suited for the supplied type.
    @param typeInfo An eris type info instance.
    */
    Definitions::EntityMappingDefinition* getDefinitionForType(Eris::TypeInfo* typeInfo);

    /**
    Creates a new EntityMapping instance. This will not be handled by the manager and needs to be deleted by the application when suitable.
    Remember to call EntityMapping::initialize(...).
    @param entity An eris type info instance.
    @param actionCreator An eris type info instance.
    @param view An optional view, if any such is available.
    */
    EntityMapping* createMapping(Eris::Entity& entity, IActionCreator& actionCreator, Eris::View* view);

protected:

	EntityMappingDefinitionStore mDefinitions;

	EntityMappingDefinitionStore mEntityTypeMappings;

	Eris::TypeService* mTypeService;

};

inline void EntityMappingManager::setTypeService(Eris::TypeService* typeService)
{
	mTypeService = typeService;
}


}

}

#endif
