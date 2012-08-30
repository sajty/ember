//
// C++ Implementation: OgreEntityRenderer
//
// Description:
//
//
// Author: Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2006
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

#include "OgreEntityRenderer.h"
#include "EntityCEGUITexture.h"
#include "../SimpleRenderContext.h"

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgrePixelCountLodStrategy.h>
#include <CEGUIImage.h>
#include <elements/CEGUIGUISheet.h>

namespace Ember
{
namespace OgreView
{
namespace Gui
{

OgreEntityRenderer::OgreEntityRenderer(CEGUI::Window* image) :
	MovableObjectRenderer(image), mEntity(0)
{
}


OgreEntityRenderer::~OgreEntityRenderer()
{
}

Ogre::Entity* OgreEntityRenderer::getEntity()
{
	return mEntity;
}

void OgreEntityRenderer::showEntity(const std::string& mesh)
{
	unloadEntity();
	try {
		std::string entityName(mTexture->getImage()->getName().c_str());
		entityName += "_entity";
		mEntity = mTexture->getRenderContext()->getSceneNode()->getCreator()->createEntity(entityName, mesh);
		setEntity(mEntity);
		mTexture->getRenderContext()->setActive(true);
	} catch (const std::exception& ex) {
		S_LOG_FAILURE("Error when creating entity." << ex);
	}
}


Ogre::MovableObject* OgreEntityRenderer::getMovableObject()
{
	return mEntity;
}

void OgreEntityRenderer::setEntity(Ogre::Entity* entity)
{
	Ogre::SceneNode* node = mTexture->getRenderContext()->getSceneNode();

	node->detachAllObjects();
	node->attachObject(entity);
	mTexture->getRenderContext()->repositionCamera();
	if (mAutoShowFull) {
		showFull();
	}
}

Ogre::SceneManager* OgreEntityRenderer::getSceneManager()
{
	return mTexture->getRenderContext()->getSceneManager();
}

void OgreEntityRenderer::unloadEntity()
{

	Ogre::SceneNode* node = mTexture->getRenderContext()->getSceneNode();
	node->detachAllObjects();
	if (mEntity) {
		Ogre::SceneManager* scenemgr = mTexture->getRenderContext()->getSceneManager();
		scenemgr->destroyEntity(mEntity);
		mEntity = NULL;
	}
}

bool OgreEntityRenderer::getWireframeMode()
{
	return (mTexture->getRenderContext()->getCamera()->getPolygonMode() == Ogre::PM_WIREFRAME);
}

void OgreEntityRenderer::setWireframeMode(bool enabled)
{
	Ogre::PolygonMode mode = enabled ? Ogre::PM_WIREFRAME : Ogre::PM_SOLID;
	mTexture->getRenderContext()->getCamera()->setPolygonMode(mode);
}

void OgreEntityRenderer::setForcedLodLevel(int lodLevel)
{
	mEntity->setMeshLodBias(1, lodLevel, lodLevel);
}

void OgreEntityRenderer::clearForcedLodLevel()
{
	mEntity->setMeshLodBias(1.0, 0, std::numeric_limits<unsigned short>::max());
}

}
}
}
