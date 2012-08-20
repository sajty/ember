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

#ifndef PMBENCH_H
#define PMBENCH_H

#include "LodConfig.h"
#include "ProgressiveMeshGenerator.h"
#include "OgreProgressiveMeshCopy.h"

#include "framework/LoggingInstance.h"
#include "components/ogre/SimpleRenderContext.h"

#include <soci.h>

#include <time.h>

namespace Ember
{
namespace OgreView
{
namespace Lod
{

class PMBench
{
	SimpleRenderContext* mRenderContext;
	const std::string mWorkDir;
	const std::string mDatabaseFile;
	const std::string mImagesDir;
	soci::session* mSQL;
	__int64 mTimerResolution;
	tm mDate;
public:
	PMBench(const std::string& dir);
	void run();
	void createCompareOgreVSGeneratorView();
private:
	void testAllMeshes();
	void testMesh(Ogre::MeshPtr mesh);
	void configureTimerResolution();
	template<typename GEN>
	void runTest(LodConfig& config);
	float getDiffTime(__int64 ctrStart, __int64 ctrEnd);
	int LastInsertID(const std::string& table);
	std::string getDate(tm* tmdate);
	void startHtml(std::ostream& stream);
	void endHtml(std::ostream& stream);
	void printSQLToHTML(const std::string& sqlQuery, const std::string& outfile);
	void appendToStream(std::ostream& stream, const soci::row& r, int i);
	void renderImage(LodConfig& config, int i, int levelID, std::string& typeName);
	void createImageView();
	void createImageViewDirect();
};

}
}
}
#endif // ifndef PMBENCH_H
