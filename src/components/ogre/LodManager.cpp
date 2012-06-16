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
#include "LodManager.h"

#include "components/ogre/XMLHelper.h"

#include "framework/osdir.h"

template<>
Ember::OgreView::LodManager* Ember::Singleton<Ember::OgreView::LodManager>::ms_Singleton = 0;

namespace Ember
{
namespace OgreView
{

LodManager::LodManager(const std::string& exportDirectory) :
	mExportDirectory(exportDirectory)
{
	LodDefinition lodDef;
	loadLodDefinition(lodDef, "acorn");
	saveLodDefinition(lodDef, "acorn2");
}

LodManager::~LodManager()
{

}

void LodManager::LoadLod(Ogre::MeshPtr& mesh)
{
	mesh->removeLodLevels();
	LodDefinition lodDef;
	std::string meshName = mesh->getOrigin();
	size_t pos = meshName.find_last_of("/\\");
	if (pos != std::string::npos) {
		meshName.substr(pos);
	}
	try {
		loadLodDefinition(lodDef, meshName);
	} catch (Ogre::Exception ex){
		//Exception is thrown if a mesh hasn't got a loddef.
	}
}

void LodManager::loadLodDefinition(LodDefinition& lodDef, const std::string& meshName)
{
	Ogre::DataStreamPtr data = Ogre::ResourceGroupManager::getSingleton().openResource(meshName + ".loddef");
	parseScript(lodDef, data);
	
}

bool LodManager::saveLodDefinition(const LodDefinition& lodDef, const std::string& meshName)
{
	return generateScript(lodDef, mExportDirectory, meshName + ".loddef");
}

void LodManager::parseScript(LodDefinition& lodDef, const Ogre::DataStreamPtr& stream)
{
	TiXmlDocument xmlDoc;
	XMLHelper xmlHelper;
	if (!xmlHelper.Load(xmlDoc, stream)) {
		return;
	}

	// <lod>...</lod>
	TiXmlElement* rootElem = xmlDoc.RootElement();
	if (rootElem) {

		// <automatic enabled="true|false" />
		TiXmlElement* autElem = rootElem->FirstChildElement("automatic");
		if (autElem) {
			const char* tmp = autElem->Attribute("enabled");
			if (tmp) {
				lodDef.setUseAutomaticLod(Ogre::StringConverter::parseBool(tmp, true));
			}
		}

		// <manual>...</manual>
		TiXmlElement* manElem = rootElem->FirstChildElement("manual");
		if (manElem) {
			// <level>...</level> <level>...</level> <level>...</level>
			for (TiXmlElement* distElem = manElem->FirstChildElement("level");
			     distElem != 0; distElem = distElem->NextSiblingElement("level")) {
				LodDistance dist;

				// <type>manual|automatic</type>
				TiXmlElement* elem = distElem->FirstChildElement("type");
				if (elem) {
					const char* tmp = elem->GetText();
					if (tmp && strcmp(tmp, "manual") == 0) {
						dist.setType(LodDistance::manualMesh);
					} else {
						dist.setType(LodDistance::automaticVertexReduction);
					}
				}

				if( dist.getType() == LodDistance::manualMesh) {
					// <meshName>.../test.mesh</meshName>
					elem = distElem->FirstChildElement("meshName");
					if (elem) {
						const char* tmp = elem->GetText();
						if (tmp) {
							dist.setMeshName(tmp);
						}
					}
				} else {
					// <method>constant|proportional</method>
					elem = distElem->FirstChildElement("method");
					if (elem) {
						const char* tmp = elem->GetText();
						if (tmp && strcmp(tmp, "constant") == 0) {
							dist.setReductionMethod(Ogre::ProgressiveMesh::VRQ_CONSTANT);
						} else {
							dist.setReductionMethod(Ogre::ProgressiveMesh::VRQ_PROPORTIONAL);
						}
					}

					// <value>0.5</value>
					elem = distElem->FirstChildElement("value");
					if (elem) {
						const char* tmp = elem->GetText();
						if (tmp) {
							dist.setReductionValue(Ogre::StringConverter::parseReal(tmp));
						}
					}
				}

				// <level distance="10">...</level>
				const char* distVal = distElem->Attribute("distance");
				if (distVal) {
					lodDef.addLodLevel(Ogre::StringConverter::parseInt(distVal), dist);
				}
			}
		}
	}
}

bool LodManager::generateScript(const LodDefinition& lodDef, const std::string& directory, const std::string& filename)
{
	if (filename == "") {
		return false;
	}

	TiXmlDocument xmlDoc;

	if (!oslink::directory(directory).isExisting()) {
		S_LOG_INFO("Creating directory " << directory);
		oslink::directory::mkdir(directory.c_str());
	}

	// <lod>...</lod>
	TiXmlElement rootElem("lod");

	{
		// <automatic enabled="true|false" />
		TiXmlElement autElem("automatic");
		autElem.SetAttribute("enabled", lodDef.getUseAutomaticLod() ? "true" : "false");

		// <manual>...</manual>
		TiXmlElement manElem("manual");

		{
			// <level>...</level> <level>...</level> <level>...</level>
			const std::map<int, LodDistance>& manualLod = lodDef.getManualLodData();
			std::map<int, LodDistance>::const_iterator it;
			for (it = manualLod.begin(); it != manualLod.end(); it++) {


				// <level distance="10">...</level>
				TiXmlElement levelElem("level");
				levelElem.SetAttribute("distance", Ogre::StringConverter::toString(it->first));

				const LodDistance& dist = it->second;
				{
					// <type>manual|automatic</type>
					TiXmlElement typeElem("type");
					TiXmlText typeText(dist.getType() == LodDistance::automaticVertexReduction ? "automatic" : "manual");
					typeElem.InsertEndChild(typeText);
					levelElem.InsertEndChild(typeElem);

					if( dist.getType() == LodDistance::manualMesh) {
						// <meshName>.../test.mesh</meshName>
						TiXmlElement meshElem("meshName");
						TiXmlText meshText(dist.getMeshName());
						meshElem.InsertEndChild(meshText);
						levelElem.InsertEndChild(meshElem);
					} else {
						// <method>constant|proportional</method>
						TiXmlElement methodElem("method");
						TiXmlText methodText(
							dist.getReductionMethod() == Ogre::ProgressiveMesh::VRQ_PROPORTIONAL ? "proportional" : "constant");
						methodElem.InsertEndChild(methodText);
						
						// <value>0.5</value>
						TiXmlElement valueElem("value");
						TiXmlText valueText(Ogre::StringConverter::toString(dist.getReductionValue()));
						valueElem.InsertEndChild(valueText);

						levelElem.InsertEndChild(methodElem);
						levelElem.InsertEndChild(valueElem);
					}
					
				}
				manElem.InsertEndChild(levelElem);
			}
		}
		rootElem.InsertEndChild(autElem);
		rootElem.InsertEndChild(manElem);
	}

	xmlDoc.InsertEndChild(rootElem);
	xmlDoc.SaveFile((directory + filename).c_str());
	S_LOG_INFO("Saved file " << (directory + filename));
	return true;
}

}
}
