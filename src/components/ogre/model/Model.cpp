/*
 Copyright (C) 2004  Erik Hjortsberg
 Copyright (c) 2005 The Cataclysmos Team

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Model.h"
#include "SubModel.h"
#include "SubModelPart.h"
#include "AnimationSet.h"
#include "Action.h"
#include "ParticleSystem.h"
#include "ParticleSystemBinding.h"
#include "ModelPart.h"

#include "components/ogre/EmberOgre.h"
#include "ModelDefinitionManager.h"
#include "ModelDefinition.h"
#include "ModelBackgroundLoader.h"


#include "framework/TimeFrame.h"
#include "framework/TimedLog.h"

#include <OgreTagPoint.h>
#include <OgreMeshManager.h>
#include <OgreResourceBackgroundQueue.h>
#include <OgreSceneManager.h>
#include <OgreAnimationState.h>
#include <OgreSubEntity.h>
#include <OgreParticleSystem.h>
#include <OgreParticleEmitter.h>
#include <OgreMaterialManager.h>

namespace Ember
{
namespace OgreView
{
namespace Model
{

const Ogre::String Model::sMovableType = "Model";
unsigned long Model::msAutoGenId = 0;

Model::Model(const std::string& name) :
	Ogre::MovableObject(name), mSkeletonOwnerEntity(0), mSkeletonInstance(0), mScale(0), mRotation(Ogre::Quaternion::IDENTITY), mAnimationStateSet(0), mAttachPoints(0), mBackgroundLoader(0)
{
	mVisible = true;
}
Model::~Model()
{
	resetSubmodels();
	resetParticles();
	resetLights();
	if (!mDefinition.isNull()) {
		mDefinition->removeModelInstance(this);
	}
	if (mBackgroundLoader) {
		ModelDefinitionManager::getSingleton().removeBackgroundLoader(mBackgroundLoader);
	}
	delete mBackgroundLoader;
	S_LOG_VERBOSE("Deleted "<< getName());
}

void Model::reset()
{
	S_LOG_VERBOSE("Resetting "<< getName());
	Resetting.emit();
	//	resetAnimations();
	resetSubmodels();
	resetParticles();
	resetLights();
	mScale = 0;
	mRotation = Ogre::Quaternion::IDENTITY;
	mSkeletonInstance = 0;
	// , mAnimationStateSet(0)
	mSkeletonOwnerEntity = 0;
	mAttachPoints = std::auto_ptr<AttachPointWrapperStore>(0);

}

void Model::reload()
{
	//	resetAnimations();
	/*	resetSubmodels();
	 resetParticles();	*/
	reset();
	createFromDefn();
	//if we are attached, we have to nofify the new entities, else they won't appear in the scene
	_notifyAttached(mParentNode, mParentIsTagPoint);

	Reloaded.emit();
}

bool Model::create(const std::string& modelType)
{
	if (!mDefinition.isNull() && mDefinition->isValid()) {
		S_LOG_WARNING("Trying to call create('" + modelType + "') on a Model instance that already have been created as a '" + mDefinition->getName() + "'.");
		return false;
	}

	static const Ogre::String groupName("ModelDefinitions");
	//Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
	try {
		mDefinition = ModelDefinitionManager::getSingleton().load(modelType, groupName);
	} catch (const std::exception& ex) {
		S_LOG_FAILURE("Could not load model of type " << modelType << " from group " << groupName << "." << ex);
		return false;
	}
	if (false && !mDefinition->isValid()) {
		S_LOG_FAILURE("Model of type " << modelType << " from group " << groupName << " is not valid.");
		return false;
	} else {
		mDefinition->addModelInstance(this);
		return true;
		/*		bool success =  createFromDefn();
		 if (!success) {
		 reset();
		 }
		 return success;*/
	}
}

void Model::_notifyManager(Ogre::SceneManager* man)
{
	Ogre::MovableObject::_notifyManager(man);
	bool success = createFromDefn();
	if (!success) {
		reset();
	}
}

bool Model::createFromDefn()
{
	TimedLog timedLog("Model::createFromDefn");
	// create instance of model from definition
	mScale = mDefinition->mScale;
	mRotation = mDefinition->mRotation;

#if OGRE_THREAD_SUPPORT
	if (!mBackgroundLoader) {
		mBackgroundLoader = new ModelBackgroundLoader(*this);
	}

	if (mBackgroundLoader->poll(TimeFrame(0))) {
		timedLog.report("Initial poll.");
		return createActualModel();
	}
	else {
		ModelDefinitionManager::getSingleton().addBackgroundLoader(mBackgroundLoader);
		setRenderingDistance(mDefinition->getRenderingDistance());
		return true;
	}
#else
	return createActualModel();
#endif
}

bool Model::createActualModel()
{
	TimedLog timedLog("Model::createActualModel");
	Ogre::SceneManager* sceneManager = _getManager();
	std::vector<std::string> showPartVector;

	for (SubModelDefinitionsStore::const_iterator I_subModels = mDefinition->getSubModelDefinitions().begin(); I_subModels != mDefinition->getSubModelDefinitions().end(); ++I_subModels) {
		std::string entityName = mName + "/" + (*I_subModels)->getMeshName();
		try {

			Ogre::Entity* entity = sceneManager->createEntity(entityName, (*I_subModels)->getMeshName());
			timedLog.report("Created entity.");
			if (entity->getMesh().isNull()) {
				S_LOG_FAILURE("Could not load mesh " << (*I_subModels)->getMeshName() << " which belongs to model " << mDefinition->getName() << ".");
			}

			if (mDefinition->getRenderingDistance()) {
				entity->setRenderingDistance(mDefinition->getRenderingDistance());
			}

			SubModel* submodel = new SubModel(*entity);
			//Model::SubModelPartMapping* submodelPartMapping = new Model::SubModelPartMapping();

			for (PartDefinitionsStore::const_iterator I_parts = (*I_subModels)->getPartDefinitions().begin(); I_parts != (*I_subModels)->getPartDefinitions().end(); ++I_parts) {
				SubModelPart& part = submodel->createSubModelPart((*I_parts)->getName());
				std::string groupName("");

				if ((*I_parts)->getSubEntityDefinitions().size() > 0) {
					for (SubEntityDefinitionsStore::const_iterator I_subEntities = (*I_parts)->getSubEntityDefinitions().begin(); I_subEntities != (*I_parts)->getSubEntityDefinitions().end(); ++I_subEntities) {
						try {
							Ogre::SubEntity* subEntity(0);
							//try with a submodelname first
							if ((*I_subEntities)->getSubEntityName() != "") {
								subEntity = entity->getSubEntity((*I_subEntities)->getSubEntityName());
							} else {
								//no name specified, use the index instead
								if (entity->getNumSubEntities() > (*I_subEntities)->getSubEntityIndex()) {
									subEntity = entity->getSubEntity((*I_subEntities)->getSubEntityIndex());
								} else {
									S_LOG_WARNING("Model definition " << mDefinition->getName() << " has a reference to entity with index " << (*I_subEntities)->getSubEntityIndex() << " which is out of bounds.");
								}
							}
							if (subEntity) {
								part.addSubEntity(subEntity, *I_subEntities);

								if ((*I_subEntities)->getMaterialName() != "") {
									subEntity->setMaterialName((*I_subEntities)->getMaterialName());
								}
							} else {
								S_LOG_WARNING("Could not add subentity.");
							}
						} catch (const std::exception& ex) {
							S_LOG_WARNING("Error when getting sub entities for model '" << mDefinition->getName() << "'." << ex);
						}
					}
				} else {
					//if no subentities are defined, add all subentities
					unsigned int numSubEntities = entity->getNumSubEntities();
					for (unsigned int i = 0; i < numSubEntities; ++i) {
						part.addSubEntity(entity->getSubEntity(i), 0);
					}
				}
				if ((*I_parts)->getGroup() != "") {
					mGroupsToPartMap[(*I_parts)->getGroup()].push_back((*I_parts)->getName());
					//mPartToGroupMap[(*I_parts)->getName()] = (*I_parts)->getGroup();
				}

				if ((*I_parts)->getShow()) {
					showPartVector.push_back((*I_parts)->getName());
				}

				ModelPart& modelPart = mModelParts[(*I_parts)->getName()];
				modelPart.addSubModelPart(&part);
				modelPart.setGroupName((*I_parts)->getGroup());
			}
			addSubmodel(submodel);
			timedLog.report("Created submodel.");
		} catch (const std::exception& e) {
			S_LOG_FAILURE( "Submodel load error for " << entityName << "." << e);
			return false;
		}
	}

	setRenderingDistance(mDefinition->getRenderingDistance());

	createActions();
	timedLog.report("Created actions.");

	createParticles();
	timedLog.report("Created particles.");

	createLights();
	timedLog.report("Created lights.");

	std::vector<std::string>::const_iterator I_end = showPartVector.end();
	for (std::vector<std::string>::const_iterator I = showPartVector.begin(); I != I_end; I++) {
		showPart(*I);
	}
	return true;
}

void Model::createActions()
{

	ActionDefinitionsStore::const_iterator I_actions_end = mDefinition->getActionDefinitions().end();
	for (ActionDefinitionsStore::const_iterator I_actions = mDefinition->getActionDefinitions().begin(); I_actions != I_actions_end; ++I_actions) {
		Action action;
		action.setName((*I_actions)->getName());
		action.getAnimations().setSpeed((*I_actions)->getAnimationSpeed());

		if (getSkeleton() && getAllAnimationStates()) {
			if (!mDefinition->getBoneGroupDefinitions().empty()) {
				//If there are bone groups, we need to use a cumulative blend mode. Note that this will affect all animations in the model.
				getSkeleton()->setBlendMode(Ogre::ANIMBLEND_CUMULATIVE);
			}
			if (mSubmodels.size()) {
				AnimationDefinitionsStore::const_iterator I_anims_end = (*I_actions)->getAnimationDefinitions().end();
				for (AnimationDefinitionsStore::const_iterator I_anims = (*I_actions)->getAnimationDefinitions().begin(); I_anims != I_anims_end; ++I_anims) {
					Animation animation((*I_anims)->getIterations(), getSkeleton()->getNumBones());
					AnimationPartDefinitionsStore::const_iterator I_animParts_end = (*I_anims)->getAnimationPartDefinitions().end();
					for (AnimationPartDefinitionsStore::const_iterator I_animParts = (*I_anims)->getAnimationPartDefinitions().begin(); I_animParts != I_animParts_end; ++I_animParts) {
						if (getAllAnimationStates()->hasAnimationState((*I_animParts)->Name)) {
							AnimationPart animPart;
							try {
								Ogre::AnimationState* state = getAnimationState((*I_animParts)->Name);
								animPart.state = state;
								for (std::vector<BoneGroupRefDefinition>::const_iterator I_boneGroupRef = (*I_animParts)->BoneGroupRefs.begin(); I_boneGroupRef != (*I_animParts)->BoneGroupRefs.end(); ++I_boneGroupRef) {
									BoneGroupDefinitionStore::const_iterator I_boneGroup = mDefinition->getBoneGroupDefinitions().find(I_boneGroupRef->Name);
									if (I_boneGroup != mDefinition->getBoneGroupDefinitions().end()) {
										BoneGroupRef boneGroupRef;
										boneGroupRef.boneGroupDefinition = I_boneGroup->second;
										boneGroupRef.weight = I_boneGroupRef->Weight;
										animPart.boneGroupRefs.push_back(boneGroupRef);
									}
								}
								animation.addAnimationPart(animPart);
							} catch (const std::exception& ex) {
								S_LOG_FAILURE("Error when loading animation: " << (*I_animParts)->Name << "." << ex);
							}
						}
					}
					action.getAnimations().addAnimation(animation);
				}
			}
		}

		//TODO: add sounds too

		mActions[(*I_actions)->getName()] = action;
	}
}

void Model::createParticles()
{
	std::vector<ModelDefinition::ParticleSystemDefinition>::const_iterator I_particlesys_end = mDefinition->mParticleSystems.end();
	for (std::vector<ModelDefinition::ParticleSystemDefinition>::const_iterator I_particlesys = mDefinition->mParticleSystems.begin(); I_particlesys != I_particlesys_end; ++I_particlesys) {
		//first try to create the ogre particle system
		std::string name(mName + "/particle" + I_particlesys->Script);
		Ogre::ParticleSystem* ogreParticleSystem;
		try {
			ogreParticleSystem = _getManager()->createParticleSystem(name, I_particlesys->Script);
		} catch (const std::exception& ex) {
			S_LOG_FAILURE("Could not create particle system: " << name << "." << ex);
			continue;
		}
		if (ogreParticleSystem) {
			//ogreParticleSystem->setDefaultDimensions(1, 1);
			ParticleSystem* particleSystem = new ParticleSystem(ogreParticleSystem, I_particlesys->Direction);
			for (ModelDefinition::BindingSet::const_iterator I = I_particlesys->Bindings.begin(); I != I_particlesys->Bindings.end(); ++I) {
				ParticleSystemBinding* binding = particleSystem->addBinding(I->EmitterVar, I->AtlasAttribute);
				mAllParticleSystemBindings.push_back(binding);
			}
			mParticleSystems.push_back(particleSystem);
		}

	}
}

void Model::createLights()
{
	ModelDefinition::LightSet::const_iterator I_lights_end = mDefinition->mLights.end();
	int j = 0;
	for (ModelDefinition::LightSet::const_iterator I_lights = mDefinition->mLights.begin(); I_lights != I_lights_end; ++I_lights) {
		//first try to create the ogre lights
		//std::string name(mName + "/light");
		std::stringstream name;
		name << mName << "/light" << (j++);
		LightInfo lightInfo;
		Ogre::Light* ogreLight;
		try {
			ogreLight = _getManager()->createLight(name.str());
		} catch (const std::exception& ex) {
			S_LOG_FAILURE("Could not create light: " << name.str() << "." << ex);
			continue;
		}
		if (ogreLight) {
			ogreLight->setType(Ogre::Light::LT_POINT);
			ogreLight->setDiffuseColour(I_lights->diffuseColour);
			ogreLight->setSpecularColour(I_lights->specularColour);
			ogreLight->setAttenuation(I_lights->range, I_lights->constant, I_lights->linear, I_lights->quadratic);
			ogreLight->setPosition(I_lights->position);
			//ogreLight->setDiffuseColour(Ogre::ColourValue(0.5f,0.0f,0.0f));
			//ogreLight->setSpecularColour(Ogre::ColourValue(0.5f,0.0f,0.0f));
			//ogreLight->setAttenuation(100,1,0,0);
			//ogreLight->setSpotlightRange(Ogre::Degree(60), Ogre::Degree(70));
			//ogreLight->setDirection(Ogre::Vector3::NEGATIVE_UNIT_Y);

			lightInfo.light = ogreLight;
			lightInfo.position = I_lights->position;
			mLights.push_back(lightInfo);
		}

	}
}

bool Model::hasParticles() const
{
	return mParticleSystems.size() > 0;
}

const ParticleSystemBindingsPtrSet& Model::getAllParticleSystemBindings() const
{
	return mAllParticleSystemBindings;
}

ParticleSystemSet& Model::getParticleSystems()
{
	return mParticleSystems;
}

LightSet& Model::getLights()
{
	return mLights;
}

bool Model::addSubmodel(SubModel* submodel)
{
	//if the submodel has a skeleton, check if it should be shared with existing models
	if (submodel->getEntity()->getSkeleton()) {
		if (mSkeletonOwnerEntity != 0) {
			submodel->getEntity()->shareSkeletonInstanceWith(mSkeletonOwnerEntity);
		} else {
			mSkeletonOwnerEntity = submodel->getEntity();
			// 			mAnimationStateSet = submodel->getEntity()->getAllAnimationStates();
		}
	}
	mSubmodels.insert(submodel);
	return true;
}

bool Model::removeSubmodel(SubModel* submodel)
{
	mSubmodels.erase(submodel);
	return true;
}

SubModel* Model::getSubModel(size_t index)
{
	size_t i = 0;
	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		if (i == index) {
			return *I;
		}
	}
	S_LOG_FAILURE("Could not find submodel with index "<< index << " in model " << getName());
	return 0;

}

void Model::showPart(const std::string& partName, bool hideOtherParts)
{
	ModelPartStore::iterator I = mModelParts.find(partName);
	if (I != mModelParts.end()) {
		ModelPart& modelPart = I->second;
		if (hideOtherParts) {
			const std::string& groupName = modelPart.getGroupName();
			//make sure that all other parts in the same group are hidden
			PartGroupStore::iterator partBucketI = mGroupsToPartMap.find(groupName);
			if (partBucketI != mGroupsToPartMap.end()) {
				for (std::vector<std::string>::iterator I = partBucketI->second.begin(); I != partBucketI->second.end(); ++I) {
					if (*I != partName) {
						hidePart(*I, true);
					}
				}
			}
		}

		modelPart.show();
	}
}

void Model::hidePart(const std::string& partName, bool dontChangeVisibility)
{
	ModelPartStore::iterator I = mModelParts.find(partName);
	if (I != mModelParts.end()) {
		ModelPart& modelPart = I->second;
		modelPart.hide();
		if (!dontChangeVisibility) {
			modelPart.setVisible(false);
			const std::string& groupName = modelPart.getGroupName();
			//if some part that was hidden before now should be visible
			PartGroupStore::iterator partBucketI = mGroupsToPartMap.find(groupName);
			if (partBucketI != mGroupsToPartMap.end()) {
				for (std::vector<std::string>::iterator J = partBucketI->second.begin(); J != partBucketI->second.end(); ++J) {
					if (*J != partName) {
						ModelPartStore::iterator I_modelPart = mModelParts.find(partName);
						if (I_modelPart != mModelParts.end()) {
							if (I_modelPart->second.getVisible()) {
								I_modelPart->second.show();
								break;
							}
						}
					}
				}
			}

		}
	}

}

void Model::setVisible(bool visible)
{
	mVisible = visible;
	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		(*I)->getEntity()->setVisible(visible);
	}

	LightSet::const_iterator lightsI_end = mLights.end();
	for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
		I->light->setVisible(visible);
	}

	ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		(*I)->setVisible(visible);
	}
}

void Model::setDisplaySkeleton(bool display)
{
	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		(*I)->getEntity()->setDisplaySkeleton(display);
	}
}

bool Model::getDisplaySkeleton(void) const
{
	SubModelSet::const_iterator I = mSubmodels.begin();
	if (I != mSubmodels.end()) {
		return (*I)->getEntity()->getDisplaySkeleton();
	}
	return false;
}

bool Model::isLoaded() const
{
	//If there's no background loader available the model is loaded in the main thread, and therefore is considered to be loaded already.
	return mBackgroundLoader == 0 || mBackgroundLoader->getState() == ModelBackgroundLoader::LS_DONE;
}

const Ogre::Real Model::getScale() const
{
	return mScale;
}

const Ogre::Quaternion& Model::getRotation() const
{
	return mRotation;
}

const ModelDefinition::UseScaleOf Model::getUseScaleOf() const
{
	return mDefinition->getUseScaleOf();
}

Action* Model::getAction(const std::string& name)
{
	ActionStore::iterator I = mActions.find(name);
	if (I == mActions.end()) {
		return 0;
	}
	return &(I->second);
}

Action* Model::getAction(const ActivationDefinition::Type type, const std::string& trigger)
{
	for (ActionDefinitionsStore::const_iterator I = mDefinition->mActions.begin(); I != mDefinition->mActions.end(); ++I) {
		for (ActivationDefinitionStore::const_iterator J = (*I)->getActivationDefinitions().begin(); J !=  (*I)->getActivationDefinitions().end(); ++J) {
			const ActivationDefinition* activationDefinition = *J;
			if (type == activationDefinition->type && trigger == activationDefinition->trigger) {
				return getAction((*I)->getName());
			}
		}
	}
	return 0;
}

void Model::resetSubmodels()
{
	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		delete *I;
		/* 		SubModel* submodel = *I;
		 Ogre::SceneManager* sceneManager = ModelDefinitionManager::instance().getSceneManager();
		 sceneManager->removeEntity(submodel->getEntity());*/
	}
	mSubmodels.clear();
	mModelParts.clear();
}

void Model::resetParticles()
{
	ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		ParticleSystem* system = *I;
		delete system;
	}
	mParticleSystems.clear();
	mAllParticleSystemBindings.clear();
}

void Model::resetLights()
{

	LightSet::const_iterator lightsI_end = mLights.end();
	for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
		Ogre::Light* light = I->light;
		if (light) {
			//Try first with the manager to which the light belongs to. If none is found, try to see if we belong to a maneger. And if that's not true either, just delete it.
			if (light->_getManager()) {
				light->_getManager()->destroyLight(light);
			} else if (_getManager()) {
				_getManager()->destroyLight(light);
			} else {
				delete light;
			}

		}
	}
	mLights.clear();
}

Model::AttachPointWrapper Model::attachObjectToAttachPoint(const Ogre::String &attachPointName, Ogre::MovableObject *pMovable, const Ogre::Vector3 &scale, const Ogre::Quaternion &offsetOrientation, const Ogre::Vector3 &offsetPosition)
{
	for (AttachPointDefinitionStore::iterator I = mDefinition->mAttachPoints.begin(); I != mDefinition->mAttachPoints.end(); ++I) {
		if (I->Name == attachPointName) {
			const std::string& boneName = I->BoneName;
			//use the rotation in the attach point def
			Ogre::TagPoint* tagPoint = attachObjectToBone(boneName, pMovable, offsetOrientation * I->Rotation, offsetPosition, scale);
			if (!mAttachPoints.get()) {
				mAttachPoints = std::auto_ptr<AttachPointWrapperStore>(new AttachPointWrapperStore());
			}

			AttachPointWrapper wrapper;
			wrapper.TagPoint = tagPoint;
			wrapper.Movable = pMovable;
			wrapper.Definition = *I;
			mAttachPoints->push_back(wrapper);
			return wrapper;
		}
	}
	return AttachPointWrapper();
}

bool Model::hasAttachPoint(const std::string& attachPoint) const
{
	for (AttachPointDefinitionStore::iterator I = mDefinition->mAttachPoints.begin(); I != mDefinition->mAttachPoints.end(); ++I) {
		if (I->Name == attachPoint) {
			return true;
		}
	}
	return false;
}

Ogre::AnimationState* Model::getAnimationState(const Ogre::String& name)
{
	if (mSubmodels.size() && mSkeletonOwnerEntity) {
		return mSkeletonOwnerEntity->getAnimationState(name);
	} else {
		return 0;
	}
}

Ogre::AnimationStateSet* Model::getAllAnimationStates()
{
	if (mSubmodels.size() && mSkeletonOwnerEntity) {
		return mSkeletonOwnerEntity->getAllAnimationStates();
	} else {
		return 0;
	}
}

Ogre::SkeletonInstance * Model::getSkeleton()
{
	if (mSubmodels.size() && mSkeletonOwnerEntity) {
		return mSkeletonOwnerEntity->getSkeleton();
	} else {
		return 0;
	}
}

Ogre::TagPoint* Model::attachObjectToBone(const Ogre::String &boneName, Ogre::MovableObject *pMovable, const Ogre::Quaternion &offsetOrientation, const Ogre::Vector3 &offsetPosition)
{
	return attachObjectToBone(boneName, pMovable, offsetOrientation, offsetPosition, Ogre::Vector3::UNIT_SCALE);
}

Ogre::TagPoint* Model::attachObjectToBone(const Ogre::String &boneName, Ogre::MovableObject *pMovable, const Ogre::Quaternion &offsetOrientation, const Ogre::Vector3 &offsetPosition, const Ogre::Vector3 &scale)
{
	if (mSubmodels.size()) {
		Ogre::Entity* entity = mSkeletonOwnerEntity;

		Ogre::TagPoint* tagPoint = entity->attachObjectToBone(boneName, pMovable, offsetOrientation, offsetPosition);

		if (mParentNode) {
			//since we're using inherit scale on the tagpoint, divide by the parent's scale now, so it evens out later on when the TagPoint is scaled in TagPoint::_updateFromParent(
			Ogre::Vector3 parentScale = mParentNode->_getDerivedScale();
			tagPoint->setScale(scale / parentScale);
		} else {
			//no parent node, this is not good...
			tagPoint->setScale(scale);
		}
		return tagPoint;

	} else {
		OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "There are no entities loaded!", "Model::attachObjectToBone");
	}
}

Ogre::MovableObject* Model::detachObjectFromBone(const Ogre::String &movableName)
{

	if (mSubmodels.size() && mSkeletonOwnerEntity) {
		if (mAttachPoints.get()) {
			for (AttachPointWrapperStore::iterator I = mAttachPoints->begin(); I != mAttachPoints->end(); ++I) {
				if (I->Movable->getName() == movableName) {
					Ogre::MovableObject* result = I->Movable;
					if (I->TagPoint == I->Movable->getParentNode()) {
						result = mSkeletonOwnerEntity->detachObjectFromBone(movableName);
					}
					mAttachPoints->erase(I);
					return result;
				}
			}
		}
		return 0;
	} else {
		OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "There are no entities loaded!", "Model::detachObjectFromBone");
	}
}

//-----------------------------------------------------------------------
void Model::detachAllObjectsFromBone(void)
{
	if (mSubmodels.size() && mSkeletonOwnerEntity) {
		mSkeletonOwnerEntity->detachAllObjectsFromBone();
		mAttachPoints = std::auto_ptr<AttachPointWrapperStore>(0);

	} else {
		OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "There are no entities loaded!", "Model::detachAllObjectsFromBone");
	}
}

/** Overridden - see MovableObject.
 */
void Model::_notifyCurrentCamera(Ogre::Camera* cam)
{
	MovableObject::_notifyCurrentCamera(cam);
	if (isVisible()) {
		if (mVisible) {
			SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
			for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
				(*I)->getEntity()->_notifyCurrentCamera(cam);
			}

			// Notify any child objects
			Ogre::Entity::ChildObjectList::iterator child_itr_end = mChildObjectList.end();
			for (Ogre::Entity::ChildObjectList::iterator child_itr = mChildObjectList.begin(); child_itr != child_itr_end; child_itr++) {
				child_itr->second->_notifyCurrentCamera(cam);
			}

			ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
			for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
				(*I)->getOgreParticleSystem()->_notifyCurrentCamera(cam);
			}

			LightSet::const_iterator lightsI_end = mLights.end();
			for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
				I->light->_notifyCurrentCamera(cam);
			}
		}
	}
}

void Model::_notifyMoved()
{
	MovableObject::_notifyMoved();

	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		(*I)->getEntity()->_notifyMoved();
	}

	ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		(*I)->getOgreParticleSystem()->_notifyMoved();
	}

	LightSet::const_iterator lightsI_end = mLights.end();
	for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
		I->light->_notifyMoved();
	}
}

void Model::setUserAny(const Ogre::Any &anything)
{
	Ogre::MovableObject::setUserAny(anything);
	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		(*I)->getEntity()->setUserAny(anything);
	}
	ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		(*I)->getOgreParticleSystem()->setUserAny(anything);
	}

	LightSet::const_iterator lightsI_end = mLights.end();
	for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
		I->light->setUserAny(anything);
	}
}

// Overridden - see MovableObject.
void Model::setRenderQueueGroup(Ogre::RenderQueueGroupID queueID)
{
	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		(*I)->getEntity()->setRenderQueueGroup(queueID);
	}
	ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		(*I)->getOgreParticleSystem()->setRenderQueueGroup(queueID);
	}

	LightSet::const_iterator lightsI_end = mLights.end();
	for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
		I->light->setRenderQueueGroup(queueID);
	}

}

/** Overridden - see MovableObject.
 */
const Ogre::AxisAlignedBox& Model::getBoundingBox(void) const
{
	mFull_aa_box.setNull();

	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		mFull_aa_box.merge((*I)->getEntity()->getBoundingBox());
	}
	ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		mFull_aa_box.merge((*I)->getOgreParticleSystem()->getBoundingBox());
	}

	return mFull_aa_box;
}

/** Overridden - see MovableObject.
 */
const Ogre::AxisAlignedBox& Model::getWorldBoundingBox(bool derive) const
{
	mWorldFull_aa_box.setNull();

	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		mWorldFull_aa_box.merge((*I)->getEntity()->getWorldBoundingBox(derive));
	}
	ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		mWorldFull_aa_box.merge((*I)->getOgreParticleSystem()->getWorldBoundingBox(derive));
	}

	return mWorldFull_aa_box;
}

Ogre::Real Model::getBoundingRadius() const
{
	Ogre::Real rad(0);
	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		rad = std::max<Ogre::Real>(rad, (*I)->getEntity()->getBoundingRadius());
	}
	ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		rad = std::max<Ogre::Real>(rad, (*I)->getOgreParticleSystem()->getBoundingRadius());
	}
	return rad;

}

/** Overridden - see MovableObject.
 */
void Model::_updateRenderQueue(Ogre::RenderQueue* queue)
{
	//check with both the model visibility setting and with the general model setting to see whether the model should be shown
	if (isVisible()) {
		SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
		for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
			if ((*I)->getEntity()->isVisible()) {
				(*I)->getEntity()->_updateRenderQueue(queue);
			}
		}

		if (getSkeleton() != 0) {
			//updateAnimation();
			Ogre::Entity::ChildObjectList::iterator child_itr = mChildObjectList.begin();
			Ogre::Entity::ChildObjectList::iterator child_itr_end = mChildObjectList.end();
			for (; child_itr != child_itr_end; child_itr++) {
				//make sure to do _update here, else attached entities won't be updated if no animation is playing
				child_itr->second->getParentNode()->_update(true, true);
				if (child_itr->second->isVisible())
					child_itr->second->_updateRenderQueue(queue);
			}
		}
		ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
		for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
			Ogre::ParticleSystem* particleSystem = (*I)->getOgreParticleSystem();
			//Alter the direction of the emitter so that it's always emits upwards in the world
			if (!(*I)->getDirection().isNaN()) {
				Ogre::Vector3 direction = particleSystem->getParentNode()->convertWorldToLocalPosition((*I)->getDirection());
				direction.normalise();
				//const Ogre::Quaternion& rotation = particleSystem->getParentNode()->_getDerivedOrientation();
				Ogre::Quaternion rotation = particleSystem->getParentNode()->convertWorldToLocalOrientation(Ogre::Quaternion(Ogre::Degree(0), (*I)->getDirection()));
				for (int i = 0; i < particleSystem->getNumEmitters(); ++i) {
					particleSystem->getEmitter(i)->setDirection(rotation * (*I)->getDirection());
				}
			}
			particleSystem->_updateRenderQueue(queue);
		}

		LightSet::const_iterator lightsI_end = mLights.end();
		for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
			I->light->_updateRenderQueue(queue);
		}

	}

}

/** Overridden from MovableObject */
// const Ogre::String& Model::getName(void) const
// {
// 	return mName;
// }

/** Overridden from MovableObject */
const Ogre::String& Model::getMovableType(void) const
{
	return sMovableType;
}

void Model::setRenderingDistance(Ogre::Real dist)
{
	MovableObject::setRenderingDistance(dist);
	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		(*I)->getEntity()->setRenderingDistance(dist);
	}
	ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		(*I)->getOgreParticleSystem()->setRenderingDistance(dist);
	}

	LightSet::const_iterator lightsI_end = mLights.end();
	for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
		I->light->setRenderingDistance(dist);
	}
}

void Model::setQueryFlags(unsigned long flags)
{
	MovableObject::setQueryFlags(flags);
	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		(*I)->getEntity()->setQueryFlags(flags);
	}

	ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		(*I)->getOgreParticleSystem()->setQueryFlags(flags);
	}

	LightSet::const_iterator lightsI_end = mLights.end();
	for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
		I->light->setQueryFlags(flags);
	}
}

void Model::addQueryFlags(unsigned long flags)
{
	MovableObject::addQueryFlags(flags);
	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		(*I)->getEntity()->addQueryFlags(flags);
	}

	ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		(*I)->getOgreParticleSystem()->addQueryFlags(flags);
	}

	LightSet::const_iterator lightsI_end = mLights.end();
	for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
		I->light->addQueryFlags(flags);
	}
}

void Model::removeQueryFlags(unsigned long flags)
{
	MovableObject::removeQueryFlags(flags);
	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		(*I)->getEntity()->removeQueryFlags(flags);
	}

	ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		(*I)->getOgreParticleSystem()->removeQueryFlags(flags);
	}

	LightSet::const_iterator lightsI_end = mLights.end();
	for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
		I->light->removeQueryFlags(flags);
	}

}

void Model::visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables)
{
	if (isVisible()) {
		SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
		for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
			if ((*I)->getEntity()->isVisible()) {
				(*I)->getEntity()->visitRenderables(visitor, debugRenderables);
			}
		}

		ParticleSystemSet::const_iterator particleSystemsI_end = mParticleSystems.end();
		for (ParticleSystemSet::const_iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
			(*I)->getOgreParticleSystem()->visitRenderables(visitor, debugRenderables);
		}

		LightSet::const_iterator lightsI_end = mLights.end();
		for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
			I->light->visitRenderables(visitor, debugRenderables);
		}
	}
}

/** Overridden from MovableObject */
void Model::_notifyAttached(Ogre::Node* parent, bool isTagPoint)
{
	if (parent != mParentNode) {
		MovableObject::_notifyAttached(parent, isTagPoint);
	}

	SubModelSet::const_iterator submodelsI_end = mSubmodels.end();
	for (SubModelSet::const_iterator I = mSubmodels.begin(); I != submodelsI_end; ++I) {
		if ((*I)->getEntity()->getParentNode() != parent) {
			(*I)->getEntity()->_notifyAttached(parent, isTagPoint);
		}
	}

	ParticleSystemSet::iterator particleSystemsI_end = mParticleSystems.end();
	for (ParticleSystemSet::iterator I = mParticleSystems.begin(); I != particleSystemsI_end; ++I) {
		(*I)->getOgreParticleSystem()->_notifyAttached(parent, isTagPoint);
		try {
			//Try to trigger a load of any image resources used by affectors.
			//The reason we want to do this now is that otherwise it will happen during rendering. An exception will then be thrown
			//which will bubble all the way up to the main loop, thus aborting all frames.
			(*I)->getOgreParticleSystem()->_update(0);

			//Check if the material used is transparent. If so, assign it a later render queue.
			//This is done to make transparent particle systems play better with the foliage and the water.
			//The foliage would be rendered at an earlier render queue (RENDER_QUEUE_6 normally) and the water at RENDER_QUEUE_8.
			//This of course means that there's still an issue when the camera is below the water
			//(as the water, being rendered first, will prevent the particles from being rendered). That will need to be solved.
			std::pair<Ogre::ResourcePtr, bool> result = Ogre::MaterialManager::getSingleton().createOrRetrieve((*I)->getOgreParticleSystem()->getMaterialName(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			Ogre::MaterialPtr materialPtr = static_cast<Ogre::MaterialPtr>(result.first);
			if (!materialPtr.isNull()) {
				if (materialPtr->isTransparent()) {
					(*I)->getOgreParticleSystem()->setRenderQueueGroup(Ogre::RENDER_QUEUE_9);
				}
			}


		} catch (const Ogre::Exception& ex) {
			//An exception occurred when forcing an update of the particle system. Remove it.
			S_LOG_FAILURE("Error when loading particle system " << (*I)->getOgreParticleSystem()->getName() << ". Removing it.");
			delete *I;
			I = mParticleSystems.erase(I);
		}
	}

	LightSet::const_iterator lightsI_end = mLights.end();
	for (LightSet::const_iterator I = mLights.begin(); I != lightsI_end; ++I) {
		I->light->_notifyAttached(parent, isTagPoint);
	}

}

bool Model::isVisible(void) const
{
	//check with both the model visibility setting and with the general model setting to see whether the model should be shown
	return Ogre::MovableObject::isVisible() && ModelDefinitionManager::getSingleton().getShowModels();
}

Model* Model::createModel(Ogre::SceneManager& sceneManager, const std::string& modelType, const std::string& name)
{

	Ogre::String modelName(name);
	if (name == "") {
		std::stringstream ss;
		ss << "__AutogenModel_" << Model::msAutoGenId++;
		modelName = ss.str();
	}

	// delegate to factory implementation
	Ogre::NameValuePairList params;
	params["modeldefinition"] = modelType;
	return static_cast<Model*> (sceneManager.createMovableObject(modelName, ModelFactory::FACTORY_TYPE_NAME, &params));

}

const Model::AttachPointWrapperStore* Model::getAttachedPoints() const
{
	return mAttachPoints.get();
}

Ogre::String ModelFactory::FACTORY_TYPE_NAME = "Model";
//-----------------------------------------------------------------------
const Ogre::String& ModelFactory::getType(void) const
{
	return FACTORY_TYPE_NAME;
}
//-----------------------------------------------------------------------
Ogre::MovableObject* ModelFactory::createInstanceImpl(const Ogre::String& name, const Ogre::NameValuePairList* params)
{

	// must have mesh parameter
	if (params != 0) {
		Ogre::NameValuePairList::const_iterator ni = params->find("modeldefinition");
		if (ni != params->end()) {
			Model* model = new Model(name);
			model->create(ni->second);
			return model;
		}

	}
	OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS,
			"'modeldefinition' parameter required when constructing a Model.",
			"ModelFactory::createInstance");

}
//-----------------------------------------------------------------------
void ModelFactory::destroyInstance(Ogre::MovableObject* obj)
{
	delete obj;
}

}
}
}
