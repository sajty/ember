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

#include "OgreProgressiveMeshExt.h"

#include "EmberOgreMesh.h"

#include <OgreSubMesh.h>

namespace Ember
{
namespace OgreView
{
namespace Lod
{

OgreProgressiveMeshExt::OgreProgressiveMeshExt( /*const Ogre::VertexData* vertexData, const Ogre::IndexData* indexData*/) :
	Ogre::ProgressiveMeshCopy( /*vertexData, indexData*/)
{

}

void OgreProgressiveMeshExt::build(LodConfig& lodConfigs)
{
	lodConfigs.mesh->removeLodLevels();
	for (int i = 0; i < lodConfigs.mesh->getNumSubMeshes(); i++) {
		// Check if triangles are present.
		if (lodConfigs.mesh->getSubMesh(i)->indexData->indexCount > 0) {
			// Set up data for reduction.
			Ogre::VertexData* pVertexData =
			    lodConfigs.mesh->getSubMesh(i)->useSharedVertices ? lodConfigs.mesh->sharedVertexData : lodConfigs.mesh
			    ->getSubMesh(i)->vertexData;
			mWorkingData.clear();
			addWorkingData(pVertexData, lodConfigs.mesh->getSubMesh(i)->indexData);
			mpVertexData = pVertexData;
			mpIndexData = lodConfigs.mesh->getSubMesh(i)->indexData;
			mWorstCosts.resize(pVertexData->vertexCount);
			build(lodConfigs, lodConfigs.mesh->getSubMesh(i)->mLodFaceList);

		} else {
			// Create empty index data for each lod.
			for (size_t i = 0; i < lodConfigs.levels.size(); ++i) {
				lodConfigs.mesh->getSubMesh(i)->mLodFaceList.push_back(OGRE_NEW Ogre::IndexData);
			}
		}
	}
	static_cast<EmberOgreMesh*>(lodConfigs.mesh.get())->_configureMeshLodUsage(lodConfigs);
}
void OgreProgressiveMeshExt::build(LodConfig& lodConfigs, LODFaceList& outList)
{
#ifndef NDEBUG
	// Do not call this with empty Lod.
	assert(lodConfigs.levels.size());

	// Lod distances needs to be sorted.
	for (int i = 1; i < lodConfigs.levels.size(); i++) {
		assert(lodConfigs.levels[i - 1].distance < lodConfigs.levels[i].distance);
	}
#endif

	computeAllCosts();

	mCurrNumIndexes = mpIndexData->indexCount;
	int numVerts = mNumCommonVertices;
	bool abandon = false;
	int lodCount = lodConfigs.levels.size();
	outList.resize(lodCount);
	for (int i = 0; i < lodCount; i++) {

		size_t neededVerts = calcLodVertexCount(lodConfigs.levels[i]);
		neededVerts = std::max(neededVerts, (size_t) 3);

		// Vertex count should be more then 3 and less then max vertices.
		for (; !abandon && neededVerts < numVerts; numVerts--) {
			size_t nextIndex = getNextCollapser();

			int workDataLen = mWorkingData.size();
			for (int i = 0; i < workDataLen; i++) {
				PMVertex& collapser = mWorkingData[i].mVertList.at(nextIndex);
				// This will reduce mCurrNumIndexes and recalc costs as required.
				if (collapser.collapseTo == NULL) {
					// Must have run out of valid collapsables.
					abandon = true;
					break;
				}
				assert(collapser.collapseTo->removed == false);
				collapse(&collapser);
			}
		}
		
		lodConfigs.levels[i].outSkipped = false;
		lodConfigs.levels[i].outUniqueVertexCount = 0;

		// Bake a new LOD and add it to the list
		Ogre::IndexData* newLod = OGRE_NEW Ogre::IndexData();
		bakeNewLOD(newLod);
		outList[i] = newLod;
	}

}

size_t OgreProgressiveMeshExt::calcLodVertexCount(const LodLevel& lodConfig)
{
	switch (lodConfig.reductionMethod) {
	case LodLevel::VRM_PROPORTIONAL:
		return mNumCommonVertices - (mNumCommonVertices * lodConfig.reductionValue);

	case LodLevel::VRM_CONSTANT:
		return mNumCommonVertices - lodConfig.reductionValue;

	default:
		// Collapse cost based reduction is not supported, so we don't reduce any vertices.
		return mNumCommonVertices;
	}
}

}
}
}
