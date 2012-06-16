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

#ifndef LODMANAGER_H
#define LODMANAGER_H

#include "LodDefinition.h"
#include "components/ogre/EmberOgrePrerequisites.h"
#include "framework/Singleton.h"

#include <string>

namespace Ember
{
namespace OgreView
{

/**
 * @brief LodManager manages all the resources
 * 
 */
class LodManager :
	public Ember::Singleton<LodManager>
{
public:
	/**
	 * @brief Ctor.
	 * @param exportDirectory Specifies the directory, where the changed loddef files will be saved.
	 */
	LodManager(const std::string& exportDirectory);
	virtual ~LodManager();

	/**
	 * @brief Loads the Lod for a mesh resource.
	 * @param mesh The mesh resource.
	 */
	void LoadLod(Ogre::MeshPtr& mesh);


	/**
	 * @brief Loads LodDefinition from file.
	 */
	void loadLodDefinition(LodDefinition& lodDef, const std::string& meshName);
	/**
	 * @brief Saves LodDefinition to file.
	 */
	bool saveLodDefinition(const LodDefinition& lodDef, const std::string& meshName);
private:
	void parseScript(LodDefinition& lodDef, const Ogre::DataStreamPtr& stream);
	bool generateScript(const LodDefinition& lodDef, const std::string& directory, const std::string& filename);

	const std::string mExportDirectory;
};
}
}
#endif // ifndef LODMANAGER_H
