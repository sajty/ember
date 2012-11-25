//
// C++ Implementation: Water
//
// Description:
//
//
// Author: Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2004
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

#include "Water.h"
#include "WaterCollisionDetector.h"
#include "services/EmberServices.h"
#include "services/config/ConfigService.h"
#include "components/ogre/EmberOgre.h"
#include "components/ogre/MousePicker.h"
#include <OgreSceneManager.h>
#include <OgreColourValue.h>
#include <OgreRenderTargetListener.h>
#include <OgrePlane.h>
#include <OgreEntity.h>
#include <OgreRoot.h>
#include <OgreRenderSystem.h>
#include <OgreMeshManager.h>
#include <OgreGpuProgramManager.h>
#include <OgreMaterialManager.h>
#include <OgreHardwarePixelBuffer.h>

using namespace Ogre;

namespace Ember {
namespace OgreView {

namespace Environment {

class RefractionTextureListener : public RenderTargetListener
{
Entity* pPlaneEnt;

public:

	void setPlaneEntity(Entity* plane)
	{
		pPlaneEnt = plane;
	}



    void preRenderTargetUpdate(const RenderTargetEvent& evt)
    {
        // Hide plane and objects above the water
        pPlaneEnt->setVisible(false);

    }
    void postRenderTargetUpdate(const RenderTargetEvent& evt)
    {
        pPlaneEnt->setVisible(true);
    }

};
class ReflectionTextureListener : public RenderTargetListener
{
Plane reflectionPlane;
Entity* pPlaneEnt;
Ogre::Camera* theCam;

public:

	void setPlaneEntity(Entity* plane)
	{
		pPlaneEnt = plane;
	}

	void setReflectionPlane(Plane &aPlane)
	{
		reflectionPlane = aPlane;
	}

	void setCamera(Ogre::Camera* aCamera) {
		 theCam = aCamera;
	}


    void preRenderTargetUpdate(const RenderTargetEvent& evt)
    {
        // Hide plane and objects below the water
        pPlaneEnt->setVisible(false);
        theCam->enableReflection(reflectionPlane);

    }
    void postRenderTargetUpdate(const RenderTargetEvent& evt)
    {
        // Show plane and objects below the water
        pPlaneEnt->setVisible(true);
        theCam->disableReflection();
    }

};



Water::Water(Ogre::Camera &camera, Ogre::SceneManager& sceneMgr, Ogre::RenderTarget& mainRenderTarget) :
	mCamera(&camera), mSceneMgr(sceneMgr), mRefractionListener(0), mReflectionListener(0), mWaterNode(0), mWaterEntity(0), mMainRenderTarget(mainRenderTarget),
	mWaterMaterial("Water/FresnelReflectionRefraction")
{
}

bool Water::isSupported() const
{
	// Check prerequisites first
	const RenderSystemCapabilities* caps = Root::getSingleton().getRenderSystem()->getCapabilities();
	if (!caps->hasCapability(RSC_VERTEX_PROGRAM) || !(caps->hasCapability(RSC_FRAGMENT_PROGRAM)))
	{
		return false;
	}
	else
	{
		 if (!GpuProgramManager::getSingleton().isSyntaxSupported("arbfp1") &&
            !GpuProgramManager::getSingleton().isSyntaxSupported("ps_4_0") &&
            !GpuProgramManager::getSingleton().isSyntaxSupported("ps_2_0") &&
			!GpuProgramManager::getSingleton().isSyntaxSupported("ps_1_4"))
		{
			return false;
		}
	}
	Ogre::MaterialPtr mat = MaterialManager::getSingleton().getByName(mWaterMaterial);
	if(mat.isNull()) {
		S_LOG_FAILURE("Unable to get " << mWaterMaterial << " material.");
		return false;
	}
	return true;
}


/**
	* @brief Initializes the water. You must call this in order for the water to show up.
	* @return True if the water technique could be setup, else false.
	*/
bool Water::initialize()
{
	try {
		Ogre::Plane waterPlane(Ogre::Vector3::UNIT_Y, 0);

		Ogre::Real farClipDistance = mCamera->getFarClipDistance();
		float textureSize = 10.0f;
		float planeSize = (farClipDistance + textureSize) * 2;

		// create a water plane/scene node
		Ogre::MeshManager::getSingleton().createPlane("SimpleWaterPlane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, waterPlane, planeSize, planeSize, 5, 5, true, 1, planeSize / textureSize, planeSize / textureSize, Ogre::Vector3::UNIT_Z);

		mRefractionListener = new RefractionTextureListener();
		mReflectionListener = new ReflectionTextureListener();

		Ogre::MaterialPtr mat = MaterialManager::getSingleton().getByName(mWaterMaterial);
		assert(!mat.isNull());

		Ogre::Real aspectRation = mCamera->getAspectRatio();
		Ogre::TexturePtr texture = TextureManager::getSingleton().createManual("Refraction", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 512, 512, 0, PF_A8R8G8B8, TU_RENDERTARGET);
		mRefractionTexture = texture->getBuffer()->getRenderTarget();
		{
			Viewport *v = mRefractionTexture->addViewport( mCamera );
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTexture(texture);
			v->setOverlaysEnabled(false);
			mRefractionTexture->addListener(mRefractionListener);
		}
		
		texture = TextureManager::getSingleton().createManual("Reflection", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 512, 512, 0, PF_A8R8G8B8, TU_RENDERTARGET);
		mReflectionTexture = texture->getBuffer()->getRenderTarget();
		{
			Viewport *v = mReflectionTexture->addViewport( mCamera );
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTexture(texture);
			v->setOverlaysEnabled(false);
			mReflectionTexture->addListener(mReflectionListener);
		}
		mCamera->setAspectRatio(aspectRation);

		mWaterNode = mSceneMgr.getRootSceneNode()->createChildSceneNode("water");

		mWaterEntity = mSceneMgr.createEntity("water", "SimpleWaterPlane");
		mWaterEntity->setMaterial(mat);
		//Render the water very late on, so that any transparent entity which is half submerged is already rendered.
		mWaterEntity->setRenderQueueGroup(Ogre::RENDER_QUEUE_8);
		mWaterEntity->setCastShadows(false);
		mWaterEntity->setQueryFlags(MousePicker::CM_NATURE);

		mWaterNode->attachObject(mWaterEntity);
		
		mRefractionListener->setPlaneEntity(mWaterEntity);
		mReflectionListener->setPlaneEntity(mWaterEntity);
		mReflectionListener->setReflectionPlane(mReflectionPlane);
		mReflectionListener->setCamera(mCamera);
		return true;
	} catch (const std::exception& ex) {
		S_LOG_FAILURE("Error when creating simple water." << ex);
		return false;
	}
}



Water::~Water()
{
	mRefractionTexture->removeAllViewports();
	mRefractionTexture->removeAllListeners();
	mReflectionTexture->removeAllViewports();
	mReflectionTexture->removeAllListeners();
	delete mRefractionListener;
	delete mReflectionListener;
	if (mWaterNode) {
		mWaterNode->detachAllObjects();
		mSceneMgr.destroySceneNode(mWaterNode);
	}
	if (mWaterEntity) {
		mSceneMgr.destroyEntity(mWaterEntity);
	}
}
ICollisionDetector* Water::createCollisionDetector()
{
	return new WaterCollisionDetector(*this);
}

bool Water::setUserAny(const Ogre::Any &anything)
{
	if (mWaterEntity) {
		mWaterEntity->setUserAny(anything);
		return true;
	}
	return false;
}

void Water::setLevel(float height)
{
	if (mWaterNode) {
		Ogre::Vector3 position = mWaterNode->getPosition();
		position.y = height;
		mWaterNode->setPosition(position);
	}
}

float Water::getLevel() const
{
	if (mWaterNode) {
		return mWaterNode->getPosition().y;
	}
	return 0;
}

}

}
}
