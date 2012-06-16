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

#ifndef LODDEFINITION_H
#define LODDEFINITION_H

#include "components/ogre/EmberOgrePrerequisites.h"

#include <string>
#include <map>

namespace Ember
{
namespace OgreView
{

class LodDistance
{
public:

	enum LodDistanceType {
		automaticVertexReduction,
		manualMesh
	};

	LodDistance();

	LodDistanceType getType() const;
	void setType(LodDistanceType type);

	const std::string& getMeshName() const;
	void setMeshName(const std::string& meshName);

	Ogre::ProgressiveMesh::VertexReductionQuota getReductionMethod() const;
	void setReductionMethod(Ogre::ProgressiveMesh::VertexReductionQuota val);

	float getReductionValue() const;
	void setReductionValue(float val);

private:
	LodDistanceType mType;
	std::string mMeshName;
	Ogre::ProgressiveMesh::VertexReductionQuota mReductionMethod;
	float mReductionValue;
};

class LodDefinition
{
public:

	enum LodType {
		automaticLod,
		manualLod
	};

	LodDefinition();
	bool getUseAutomaticLod() const;
	void setUseAutomaticLod(bool useAutomaticLod);

	void addLodLevel(int distVal, const LodDistance& distance);
	bool hasLodLevel(int distVal) const;
	const LodDistance& getLodLevel(int distVal) const;
	void removeLodLevel(int distVal);
	const std::map<int, LodDistance>& getManualLodData() const;

private:
	bool mUseAutomaticLod;
	std::map<int, LodDistance> mManualLod;
};
}
}
#endif // ifndef LODDEFINITION_H
