//
// C++ Implementation: ShrubberyFoliage
//
// Description:
//
//
// Author: Erik Ogenvik <erik@ogenvik.org>, (C) 2008
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

#include "ShrubberyFoliage.h"

#include "framework/LoggingInstance.h"

#include "FoliageLoader.h"

#include "../Scene.h"
#include "../Convert.h"
#include "../terrain/TerrainInfo.h"
#include "../terrain/TerrainManager.h"
#include "../terrain/TerrainLayerDefinition.h"

#include "../terrain/ITerrainAdapter.h"

#include "pagedgeometry/include/BatchPage.h"
#include "pagedgeometry/include/TreeLoader3D.h"

namespace Ember {
namespace OgreView {

namespace Environment {

ShrubberyFoliage::ShrubberyFoliage(Terrain::TerrainManager& terrainManager, const Terrain::TerrainLayerDefinition& terrainLayerDefinition, const Terrain::TerrainFoliageDefinition& foliageDefinition)
: FoliageBase(terrainManager, terrainLayerDefinition, foliageDefinition)
, mLoader(0)
{
}

ShrubberyFoliage::~ShrubberyFoliage()
{
	delete mLoader;
}

void ShrubberyFoliage::initialize()
{
	mPagedGeometry = new ::Forests::PagedGeometry(&mTerrainManager.getScene().getMainCamera(), mTerrainManager.getFoliageBatchSize());

	mPagedGeometry->setInfinite();


	mPagedGeometry->addDetailLevel<Forests::BatchPage>(64, 32);

	mLoader = new FoliageLoader(mTerrainManager.getScene().getSceneManager(), mTerrainManager, mTerrainLayerDefinition, mFoliageDefinition, *mPagedGeometry);
 	mPagedGeometry->setPageLoader(mLoader);

	std::list<Forests::GeometryPageManager*> detailLevels = mPagedGeometry->getDetailLevels();
	for (std::list<Forests::GeometryPageManager*>::iterator I = detailLevels.begin(); I != detailLevels.end(); ++I) {
		DistanceStore tempDistance = { (*I)->getFarRange(), (*I)->getNearRange(), (*I)->getTransition() };
		mDistanceStore.push_back(tempDistance);
	}
}

void ShrubberyFoliage::frameStarted()
{
	if (mPagedGeometry) {
		try {
			mPagedGeometry->update();
		} catch (const std::exception& ex)
		{
			S_LOG_FAILURE("Error when updating shrubbery for terrain layer " << mTerrainLayerDefinition.getName() << " and areaId " << mTerrainLayerDefinition.getAreaId() << ". Will disable shrubbery."<< ex);
			delete mPagedGeometry;
			delete mLoader;
			mPagedGeometry = 0;
			mLoader = 0;
		}
	}
}

void ShrubberyFoliage::setDensity(float newGrassDensity)
{
	mLoader->setDensityFactor(newGrassDensity);
	mPagedGeometry->reloadGeometry();
}

void ShrubberyFoliage::setFarDistance(float factor)
{
	std::list<Forests::GeometryPageManager*> detailLevels = mPagedGeometry->getDetailLevels();

	std::list<DistanceStore>::iterator J = mDistanceStore.begin();
	for (std::list<Forests::GeometryPageManager*>::iterator I = detailLevels.begin(); I != detailLevels.end() && J != mDistanceStore.end(); ++I) {
		(*I)->setFarRange(factor * J->farDistance);
		(*I)->setNearRange(factor * J->nearDistance);
		(*I)->setTransition(factor * J->transition);
		++J;
	}
	mPagedGeometry->reloadGeometry();
}

}

}
}
