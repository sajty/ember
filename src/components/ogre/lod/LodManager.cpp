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

#include "LodManager.h"
#include "LodDefinition.h"
#include "LodDefinitionManager.h"
#include "EmberOgreMesh.h"

#include <OgreQueuedProgressiveMeshGenerator.h>
#include <OgrePixelCountLodStrategy.h>
#include <OgreDistanceLodStrategy.h>

template<>
Ember::OgreView::Lod::LodManager * Ember::Singleton<Ember::OgreView::Lod::LodManager>::ms_Singleton = 0;

namespace Ember
{
namespace OgreView
{
namespace Lod
{

LodManager::LodManager()
{
}

LodManager::~LodManager()
{

}

void LodManager::loadLod(Ogre::MeshPtr mesh)
{
	assert(mesh->getNumLodLevels() == 1);
	std::string lodDefName = convertMeshNameToLodName(mesh->getName());

	try {
		Ogre::ResourcePtr resource = LodDefinitionManager::getSingleton().load(lodDefName, "General");
		const LodDefinition& def = *static_cast<const LodDefinition*>(resource.get());
		loadLod(mesh, def);
	} catch (const Ogre::FileNotFoundException& ex) {
		// Exception is thrown if a mesh hasn't got a loddef.
		// By default, use the automatic mesh lod management system.
		loadAutomaticLod(mesh);
	}
}

void LodManager::loadLod(Ogre::MeshPtr mesh, const LodDefinition& def)
{
	if (def.getUseAutomaticLod()) {
		loadAutomaticLod(mesh);
	} else if (def.getLodDistanceCount() == 0) {
		mesh->removeLodLevels();
		return;
	} else {
		Ogre::LodStrategy* strategy;
		if (def.getStrategy() == LodDefinition::LS_DISTANCE) {
			strategy = &Ogre::DistanceLodStrategy::getSingleton();
		} else {
			strategy = &Ogre::PixelCountLodStrategy::getSingleton();
		}

		if (def.getType() == LodDefinition::LT_AUTOMATIC_VERTEX_REDUCTION) {
			// Automatic vertex reduction
			Ogre::LodConfig lodConfig;
			lodConfig.strategy = strategy;
			lodConfig.mesh = mesh;
			const LodDefinition::LodDistanceMap& data = def.getManualLodData();
			if (def.getStrategy() == LodDefinition::LS_DISTANCE) {
				// TODO: Use C++11 lambda, instead of template.
				loadAutomaticLodImpl(data.begin(), data.end(), lodConfig);
			} else {
				loadAutomaticLodImpl(data.rbegin(), data.rend(), lodConfig);
			}
			// Uncomment the ProgressiveMesh of your choice.
			// NOTE: OgreProgressiveMeshExt doesn't support collapse cost based reduction.
			// OgreProgressiveMeshExt pm;
			// ProgressiveMeshGenerator pm;
			Ogre::QueuedProgressiveMeshGenerator pm;
			pm.generateLodLevels(lodConfig);
		} else {
			// User created Lod

			mesh->removeLodLevels();

			const LodDefinition::LodDistanceMap& data = def.getManualLodData();
			if (def.getStrategy() == LodDefinition::LS_DISTANCE) {
				// TODO: Use C++11 lambda, instead of template.
				loadUserLodImpl(data.begin(), data.end(), mesh.get());
			} else {
				loadUserLodImpl(data.rbegin(), data.rend(), mesh.get());
			}
		}
	}
}

std::string LodManager::convertMeshNameToLodName(std::string meshName)
{
	size_t start = meshName.find_last_of("/\\");
	if (start != std::string::npos) {
		meshName = meshName.substr(start + 1);
	}

	size_t end = meshName.find_last_of(".");
	if (end != std::string::npos) {
		meshName = meshName.substr(0, end);
	}

	meshName += ".loddef";
	return meshName;
}
void LodManager::loadAutomaticLod(Ogre::MeshPtr mesh)
{
	Ogre::QueuedProgressiveMeshGenerator pm;
	pm.generateAutoconfiguredLodLevels(mesh);
}

template<typename T>
void LodManager::loadAutomaticLodImpl(T it, T itEnd, Ogre::LodConfig& lodConfig)
{
	for (; it != itEnd; it++) {
		const LodDistance& dist = it->second;
		Ogre::LodLevel lodLevel;
		lodLevel.distance = it->first;
		lodLevel.reductionMethod = dist.getReductionMethod();
		lodLevel.reductionValue = dist.getReductionValue();
		lodConfig.levels.push_back(lodLevel);
	}
}

template<typename T>
void LodManager::loadUserLodImpl(T it, T itEnd, Ogre::Mesh* mesh)
{
	for (; it != itEnd; it++) {
		const Ogre::String& meshName = it->second.getMeshName();
		if (meshName != "") {
			assert(Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(meshName));
			mesh->createManualLodLevel(it->first, meshName);
		}
	}
}

}
}
}
