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

#include "ProgressiveMeshGenerator.h"

#include <OgreResource.h>

#include <string>
#include <map>

namespace Ember
{
namespace OgreView
{
namespace Lod
{

class LodDefinition;
class LodDefinitionPtr :
	public Ogre::SharedPtr<LodDefinition>
{
public:
	LodDefinitionPtr() :
		Ogre::SharedPtr<LodDefinition>()
	{
	}
	explicit LodDefinitionPtr(LodDefinition* rep) :
		Ogre::SharedPtr<LodDefinition>(rep)
	{
	}
	LodDefinitionPtr(const LodDefinitionPtr& r) :
		Ogre::SharedPtr<LodDefinition>(r)
	{
	}
	LodDefinitionPtr(const Ogre::ResourcePtr& r);

	LodDefinitionPtr& operator= (const Ogre::ResourcePtr& r);

};


/**
 * @brief Lod distance config container.
 */
class LodDistance
{
public:

	/**
	 * @brief Ctor.
	 *
	 * By default, the settings are set to proportional automatic vertex reduction to 50% of original vertex count.
	 */
	LodDistance();

	/**
	 * @brief Returns the mesh name of the Lod distance, which is used in user created meshes.
	 */
	const std::string& getMeshName() const;

	/**
	 * @brief Sets the mesh name of the Lod distance, which is used in user created meshes.
	 */
	void setMeshName(const std::string& meshName);

	/**
	 * @brief Returns the vertex reduction method of the Lod distance, which is used in automatic vertex reduction.
	 */
	LodLevel::VertexReductionMethod getReductionMethod() const;

	/**
	 * @brief Sets the vertex reduction method of the Lod distance, which is used in automatic vertex reduction.
	 */
	void setReductionMethod(LodLevel::VertexReductionMethod reductionMethod);

	/**
	 * @brief Returns the vertex reduction value of the Lod distance, which is used in automatic vertex reduction.
	 */
	float getReductionValue() const;

	/**
	 * @brief Sets the vertex reduction value of the Lod distance, which is used in automatic vertex reduction.
	 */
	void setReductionValue(float reductionValue);

private:
	std::string mMeshName;
	LodLevel::VertexReductionMethod mReductionMethod;
	float mReductionValue;
};

/**
 * @brief Lod Definition resource. Each *.loddef file is represented by a LodDefinition instance.
 */
class LodDefinition :
	public Ogre::Resource
{
public:
	typedef std::map<Ogre::Real, LodDistance> LodDistanceMap;

	/**
	 * @brief Enumeration of Distance types.
	 */
	enum LodType {
		/**
		 * @brief A built in algorithm should reduce the vertex count.
		 */
		LT_AUTOMATIC_VERTEX_REDUCTION,

		/**
		 * @brief User created mesh should be used.
		 */
		LT_USER_CREATED_MESH
	};

	/**
	 * @brief Enumeration of Lod strategies.
	 */
	enum LodStrategy {
		/**
		 * @brief It will use the distance to the camera.
		 */
		LS_DISTANCE,

		/**
		 * @brief It will use the pixel count of the bounding sphere inside the rendered frame.
		 */
		LS_PIXEL_COUNT
	};

	/**
	 * @brief Ctor. The parameters are passed directly to Ogre::Resource constructor.
	 *
	 * By default, the Automatic mesh Lod management system is enabled.
	 */
	LodDefinition(Ogre::ResourceManager* creator,
	              const Ogre::String& name,
	              Ogre::ResourceHandle handle,
	              const Ogre::String& group,
	              bool isManual = false,
	              Ogre::ManualResourceLoader* loader = 0);

	/**
	 * @brief Dtor.
	 */
	virtual ~LodDefinition();

	/**
	 * @brief Pure function inherited from Ogre::Resource.
	 *        Loads the resource file as DataStream, then it will call
	 *        XMLLodDefinitionSerializer to load the user data.
	 */
	void loadImpl(void);

	/**
	 * @brief Pure function inherited from Ogre::Resource.
	 */
	void unloadImpl(void);

	/**
	 * @brief Pure function inherited from Ogre::Resource.
	 *        Should return the size of the user data.
	 *        We don't use this, so it will always return 0.
	 */
	size_t calculateSize(void) const;

	/**
	 * @brief Returns whether automatic mesh Lod management is used.
	 */
	bool getUseAutomaticLod() const;

	/**
	 * @brief Sets whether automatic mesh Lod management should be used.
	 */
	void setUseAutomaticLod(bool useAutomaticLod);

	/**
	 * @brief Returns the type of the Lod.
	 */
	LodType getType() const;

	/**
	 * @brief Sets the type of the Lod.
	 */
	void setType(LodType type);

	/**
	 * @brief Returns the strategy of the Lod.
	 */
	LodStrategy getStrategy() const;

	/**
	 * @brief Sets the strategy of the Lod.
	 */
	void setStrategy(LodStrategy strategy);

	/**
	 * @brief Adds a Lod distance to the manual Lod configuration.
	 */
	void addLodDistance(Ogre::Real distVal, const LodDistance& distance);

	/**
	 * @brief Returns whether a Lod distance is existing in a manual Lod configuration.
	 */
	bool hasLodDistance(Ogre::Real distVal) const;

	/**
	 * @brief Returns a Lod distance from the manual Lod configuration.
	 */
	LodDistance& getLodDistance(Ogre::Real distVal);

	/**
	 * @brief Returns a Lod distance count for the manual Lod configuration.
	 */
	int getLodDistanceCount() const;

	/**
	 * @brief Creates a list of distances in a sorted order.
	 *
	 * This is meant for lua calls only.
	 */
	std::vector<float> createListOfDistances();

	/**
	 * @brief Creates a distance.
	 *
	 * This is meant for lua calls only. Use addLodDistance() if you can.
	 */
	LodDistance& createDistance(Ogre::Real distance);

	/**
	 * @brief Removes a Lod distance from the manual Lod configuration.
	 */
	void removeLodDistance(Ogre::Real distVal);
	/**
	 * @brief Returns a reference to the manual Lod configuration.
	 *
	 * This is useful for iterating through all elements.
	 */
	const LodDistanceMap& getManualLodData() const;

private:
	bool mUseAutomaticLod;
	LodType mType;
	LodStrategy mStrategy;
	LodDistanceMap mManualLod;
};

inline const std::string& LodDistance::getMeshName() const
{
	return mMeshName;
}

inline void LodDistance::setMeshName(const std::string& meshName)
{
	mMeshName = meshName;
}

inline LodLevel::VertexReductionMethod LodDistance::getReductionMethod() const
{
	return mReductionMethod;
}

inline void LodDistance::setReductionMethod(LodLevel::VertexReductionMethod reductionMethod)
{
	mReductionMethod = reductionMethod;
}

inline float LodDistance::getReductionValue() const
{
	return mReductionValue;
}

inline void LodDistance::setReductionValue(float reductionValue)
{
	mReductionValue = reductionValue;
}

inline bool LodDefinition::getUseAutomaticLod() const
{
	return mUseAutomaticLod;
}

inline void LodDefinition::setUseAutomaticLod(bool useAutomaticLod)
{
	mUseAutomaticLod = useAutomaticLod;
}

inline LodDefinition::LodType LodDefinition::getType() const
{
	return mType;
}

inline void LodDefinition::setType(LodType type)
{
	mType = type;
}

inline LodDefinition::LodStrategy LodDefinition::getStrategy() const
{
	return mStrategy;
}

inline void LodDefinition::setStrategy(LodStrategy type)
{
	mStrategy = type;
}

inline const LodDefinition::LodDistanceMap& LodDefinition::getManualLodData() const
{
	return mManualLod;
}
inline int LodDefinition::getLodDistanceCount() const
{
	return mManualLod.size();
}

}
}
}
#endif // ifndef LODDEFINITION_H
