/*
 * Copyright (C) 2012 Peter Szucs <peter.szucs.dev@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef OGREPROGRESSIVEMESHEXT_H
#define OGREPROGRESSIVEMESHEXT_H

#include "LodConfig.h"
#include "OgreProgressiveMeshCopy.h"

namespace Ember
{
namespace OgreView
{
namespace Lod
{


/**
 * @brief Improved version of Ogre::ProgressiveMesh.
 */
class OgreProgressiveMeshExt :
	public Ogre::ProgressiveMeshCopy
{
public:
	/**
	 * @brief Ctor.
	 */
	OgreProgressiveMeshExt::OgreProgressiveMeshExt( /*const Ogre::VertexData* vertexData, const Ogre::IndexData* indexData*/);

	/**
	 * @brief Builds the Lods for a submesh based on a LodConfigList.
	 *
	 * @param lodConfigs Specification of the requested lods.
	 * @param outList The output of the algorithm.
	 */
	void build(LodConfig& lodConfigs);

private:

	void build(LodConfig& lodConfigs, LODFaceList& outList);

	/**
	 * @brief Calculates the user requested vertex count for a LodConfig
	 */
	size_t calcLodVertexCount(const LodLevel& lodLevel);
};

}
}
}
#endif // ifndef OGREPROGRESSIVEMESHEXT_H
