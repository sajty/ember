/*
 Copyright (C) 2009 Erik Hjortsberg <erik.hjortsberg@gmail.com>

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

#ifndef TERRAINMODTASKBASE_H_
#define TERRAINMODTASKBASE_H_


#include "framework/tasks/TemplateNamedTask.h"
#include "Types.h"
#include <string>

namespace Mercator
{
class Terrain;
class TerrainMod;
}


namespace Ember
{
namespace OgreView
{

namespace Terrain
{
class TerrainHandler;
class TerrainMod;

/**
 * @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 * @brief Common base class for terrain mod related tasks.
 */
class TerrainModTaskBase: public Tasks::TemplateNamedTask<TerrainModTaskBase>
{
public:
	TerrainModTaskBase(Mercator::Terrain& terrain, const std::string& entityId, TerrainHandler& handler, TerrainModMap& terrainMods);
	virtual ~TerrainModTaskBase();

protected:
	/**
	 * @brief The terrain.
	 */
	Mercator::Terrain& mTerrain;

	/**
	 * @brief The entity to which the mod belongs to.
	 */
	std::string mEntityId;

	/**
	 * @brief The terrain manager.
	 */
	TerrainHandler& mHandler;

	/**
	 * @brief A shared store of terrain mods.
	 */
	TerrainModMap& mTerrainMods;

	/**
	 * @brief A list of updates areas. Any geometry in these areas will need to be recalculated.
	 */
	std::vector<WFMath::AxisBox<2>> mUpdatedAreas;

};

}

}

}

#endif /* TERRAINMODTASKBASE_H_ */
