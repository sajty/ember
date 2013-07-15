/*
 * Copyright (c) 2013 Peter Szucs <peter.szucs.dev@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "LodManager.h"
#include "LodDefinition.h"
#include "LodDefinitionManager.h"

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
		Ogre::QueuedProgressiveMeshGenerator pm;
		pm.generateAutoconfiguredLodLevels(mesh);
	}
}

void LodManager::loadLod(Ogre::MeshPtr mesh, const LodDefinition& def)
{
	if (def.getUseAutomaticLod()) {
		Ogre::QueuedProgressiveMeshGenerator pm;
		pm.generateAutoconfiguredLodLevels(mesh);
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
		mesh->setLodStrategy(strategy);

		if (def.getType() == LodDefinition::LT_AUTOMATIC_VERTEX_REDUCTION) {
			// Automatic vertex reduction
			Ogre::LodConfig lodConfig;
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
