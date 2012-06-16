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
#include "LodDefinition.h"

namespace Ember
{
namespace OgreView
{

LodDistance::LodDistance() :
	mType(automaticVertexReduction),
	mReductionMethod(Ogre::ProgressiveMesh::VRQ_CONSTANT),
	mReductionValue(0.5)
{

}

Ember::OgreView::LodDistance::LodDistanceType LodDistance::getType() const
{
	return mType;
}

void LodDistance::setType(LodDistanceType type)
{
	mType = type;
}

const std::string& LodDistance::getMeshName() const
{
	return mMeshName;
}

void LodDistance::setMeshName(const std::string& meshName)
{
	mMeshName = meshName;
}

Ogre::ProgressiveMesh::VertexReductionQuota LodDistance::getReductionMethod() const
{
	return mReductionMethod;
}

void LodDistance::setReductionMethod(Ogre::ProgressiveMesh::VertexReductionQuota val)
{
	mReductionMethod = val;
}

float LodDistance::getReductionValue() const
{
	return mReductionValue;
}

void LodDistance::setReductionValue(float val)
{
	mReductionValue = val;
}


LodDefinition::LodDefinition() :
	mUseAutomaticLod(true)
{

}

bool LodDefinition::getUseAutomaticLod() const
{
	return mUseAutomaticLod;
}

void LodDefinition::setUseAutomaticLod(bool useAutomaticLod)
{
	mUseAutomaticLod = useAutomaticLod;
}

void LodDefinition::addLodLevel(int distVal, const LodDistance& distance)
{
	mManualLod.insert(std::make_pair(distVal, distance));
}

bool LodDefinition::hasLodLevel(int distVal) const
{
	return mManualLod.find(distVal) != mManualLod.end();
}

const LodDistance& LodDefinition::getLodLevel(int distVal) const
{
	std::map<int, LodDistance>::const_iterator it = mManualLod.find(distVal);
	assert(it != mManualLod.end());
	return it->second;
}

void LodDefinition::removeLodLevel(int distVal)
{
	std::map<int, LodDistance>::const_iterator it = mManualLod.find(distVal);
	assert(it != mManualLod.end());
	mManualLod.erase(it);
}

const std::map<int, LodDistance>& LodDefinition::getManualLodData() const
{
	return mManualLod;
}

}
}
