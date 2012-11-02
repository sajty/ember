//
// C++ Implementation: FoliageBase
//
// Description:
//
//
// Author: Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2008
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

#include "FoliageBase.h"

#include "../Convert.h"
#include "../terrain/TerrainArea.h"
#include "../terrain/TerrainManager.h"
#include "../terrain/TerrainHandler.h"
#include "../terrain/TerrainShader.h"
#include "../terrain/TerrainPageSurfaceLayer.h"
#include "../terrain/TerrainPageSurface.h"
#include "../terrain/TerrainPage.h"
#include "../terrain/TerrainLayerDefinition.h"
#include "../terrain/TerrainLayerDefinitionManager.h"

#include "pagedgeometry/include/PagedGeometry.h"
#include "framework/LoggingInstance.h"
#include <wfmath/point.h>

using namespace Ember::OgreView::Terrain;

namespace Ember {
namespace OgreView {

namespace Environment {

FoliageBase::FoliageBase(Terrain::TerrainManager& terrainManager, const Terrain::TerrainLayerDefinition& terrainLayerDefinition, const Terrain::TerrainFoliageDefinition& foliageDefinition)
: mTerrainManager(terrainManager), mTerrainLayerDefinition(terrainLayerDefinition)
, mFoliageDefinition(foliageDefinition)
, mPagedGeometry(0)
{
	initializeDependentLayers();

	mTerrainManager.getHandler().EventLayerUpdated.connect(sigc::mem_fun(*this, &FoliageBase::TerrainHandler_LayerUpdated));
	mTerrainManager.getHandler().EventShaderCreated.connect(sigc::mem_fun(*this, &FoliageBase::TerrainHandler_EventShaderCreated));
	mTerrainManager.getHandler().EventAfterTerrainUpdate.connect(sigc::mem_fun(*this, &FoliageBase::TerrainHandler_AfterTerrainUpdate));

}

FoliageBase::~FoliageBase()
{
	delete mPagedGeometry;
}

void FoliageBase::initializeDependentLayers()
{
	bool foundLayer(false);
	for (TerrainLayerDefinitionManager::DefinitionStore::const_iterator I = TerrainLayerDefinitionManager::getSingleton().getDefinitions().begin(); I != TerrainLayerDefinitionManager::getSingleton().getDefinitions().end(); ++I) {

		if (foundLayer) {
			mDependentDefinitions.push_back((*I));
		} else if (!foundLayer && (*I) == &mTerrainLayerDefinition) {
			foundLayer = true;
		}
	}
}

void FoliageBase::TerrainHandler_LayerUpdated(const Terrain::TerrainShader* shader, const AreaStore& areas)
{
	if (mPagedGeometry) {
		//check if the layer update affects this layer, either if it's the actual layer, or one of the dependent layers
		bool isRelevant(0);
		if (&shader->getLayerDefinition() == &mTerrainLayerDefinition) {
			isRelevant = true;
		} else {
			if (std::find(mDependentDefinitions.begin(), mDependentDefinitions.end(), &shader->getLayerDefinition()) != mDependentDefinitions.end()) {
				isRelevant = true;
			}
		}
		if (isRelevant) {
			for (AreaStore::const_iterator I = areas.begin(); I != areas.end(); ++I) {
				const Ogre::TRect<Ogre::Real> ogreExtent(Convert::toOgre(*I));
				mPagedGeometry->reloadGeometryPages(ogreExtent);
			}
		}
	}
}

void FoliageBase::TerrainHandler_EventShaderCreated(const Terrain::TerrainShader& shader)
{
	//we'll assume that all shaders that are created after this foliage has been created will affect it, so we'll add it to the dependent layers and reload the geometry
	mDependentDefinitions.push_back(&shader.getLayerDefinition());
	if (mPagedGeometry) {
		mPagedGeometry->reloadGeometry();
	}
}

void FoliageBase::TerrainHandler_AfterTerrainUpdate(const std::vector<WFMath::AxisBox<2>>& areas, const std::set<Terrain::TerrainPage* >& pages)
{
	if (mPagedGeometry) {
		for (std::vector<WFMath::AxisBox<2>>::const_iterator I = areas.begin(); I != areas.end(); ++I) {
			const WFMath::AxisBox<2>& area(*I);
			const Ogre::TRect<Ogre::Real> ogreExtent(Convert::toOgre(area));

			mPagedGeometry->reloadGeometryPages(ogreExtent);
		}
	}
}

void FoliageBase::reloadAtPosition(const WFMath::Point<2>& worldPosition)
{
	if (mPagedGeometry) {
		mPagedGeometry->reloadGeometryPage(Ogre::Vector3(worldPosition.x(), 0, -worldPosition.y()), true);
	}
}


//Gets the height of the terrain at the specified x/z coordinate
//The userData parameter isn't used in this implementation of a height function, since
//there's no need for extra data other than the x/z coordinates.
float getTerrainHeight(float x, float z, void* userData)
{
	Domain::IHeightProvider* heightProvider = reinterpret_cast<Domain::IHeightProvider*>(userData);
	float height = 0;
	heightProvider->getHeight(Domain::TerrainPosition(x, -z), height);
	return height;
}

//Gets the height of the terrain at the specified x/z coordinate
//The userData parameter isn't used in this implementation of a height function, since
//there's no need for extra data other than the x/z coordinates.
double getTerrainHeight(double x, double z, void* userData)
{
	Domain::IHeightProvider* heightProvider = reinterpret_cast<Domain::IHeightProvider*>(userData);
	float height = 0;
	heightProvider->getHeight(Domain::TerrainPosition(x, -z), height);
	return (double)height;
}
}

}
}
