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

#include "components/ogre/widgets/OgreEntityRenderer.h"
#include <OgreLodListener.h>
#include <OgreVector3.h>

#include <boost/unordered_set.hpp>

namespace Ember
{
namespace OgreView
{
namespace Gui
{

class MeshInfoProvider :
	public Ogre::LodListener
{
public:

	sigc::signal<void> EventLodChanged;

	MeshInfoProvider(OgreEntityRenderer* entityRenderer);
	~MeshInfoProvider();
	std::string getInfo(int submeshIndex);
	std::string getPreviewInfo();
	bool prequeueEntityMeshLodChanged(Ogre::EntityMeshLodChangedEvent& evt);
	int getLodIndex();

	static size_t calcUniqueVertexCount(const Ogre::Mesh* mesh);
	static size_t calcUniqueVertexCount(const Ogre::VertexData& data);
private:

	// Hash function for UniqueVertexSet.
	struct UniqueVertexHash {
		size_t operator() (const Ogre::Vector3& v) const;
	};

	typedef boost::unordered_set<Ogre::Vector3, UniqueVertexHash> UniqueVertexSet;

	static int getVertexSize(const Ogre::VertexData* data);
	static void calcUniqueVertexCount(UniqueVertexSet& uniqueVertexSet, const Ogre::VertexData& data);

	OgreEntityRenderer* mEntityRenderer;
	int mLodIndex;
};

}
}
}
