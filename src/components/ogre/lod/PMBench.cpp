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

// We need to compile without lean and mean, because mmsystem.h is needed.
// _WINDOWS_ means you have already included windows.h
#if defined(_WINDOWS_) && defined(WIN32_LEAN_AND_MEAN)
#error We need to load windows.h without lean and mean.
#endif
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "PMBench.h"
#include "EmberOgreMesh.h"
#include "QueuedProgressiveMeshGenerator.h"
#include "OgreProgressiveMeshExt.h"

#include "framework/LoggingInstance.h"

#include "components/ogre/widgets/MeshInfoProvider.h"

#include "platform/platform_windows.h"

#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreMeshManager.h>
#include <OgreLodStrategy.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>

#include <soci-sqlite3.h>

using soci::use;
using soci::into;

namespace Ember
{
namespace OgreView
{
namespace Lod
{

PMBench::PMBench(const std::string& dir) :
	mWorkDir(dir),
	mDatabaseFile(dir + "/pmbench.db"),
	mImagesDir(dir + "/images/"),
	mSQL(NULL),
	mTimerResolution(0),
	mRenderContext(new SimpleRenderContext("PMBenchRenderer", 256, 256))
{
	time_t t;
	time(&t);
	mDate = *localtime(&t);

	mRenderContext->yaw(Ogre::Degree(30));
	mRenderContext->pitch(Ogre::Degree(-10));
	mRenderContext->setActive(true);
	mRenderContext->getCamera()->setAspectRatio(256.f / 256.f);
}

void PMBench::run()
{
	try {
		_mkdir(mImagesDir.c_str());
		soci::session sql(soci::sqlite3, mDatabaseFile);
		mSQL = &sql;

		// truncate tables
		*mSQL << "delete from benchmarks";
		*mSQL << "delete from lodlevels";
		*mSQL << "delete from images";

		configureTimerResolution();
		testAllMeshes();
		// createImageView();
		// createImageViewDirect();
		createCompareOgreVSGeneratorView();
	} catch (soci::soci_error err) {
		std::string str("Soci error: ");
		str += err.what();
		S_LOG_FAILURE(str);
	} catch (std::exception e) {
		std::string str("Exception in benchmark: ");
		str += e.what();
		S_LOG_FAILURE(str);
	}
	mSQL = 0;
}

void PMBench::testAllMeshes()
{

	Ogre::StringVectorPtr pFilenames = Ogre::ResourceGroupManager::getSingleton().findResourceNames("General", "*.mesh");
	for (unsigned int i = 0; i < 5 /*pFilenames->size()*/; i++) {
		testMesh(Ogre::MeshManager::getSingleton().load(pFilenames->at(i), "General"));
	}
}

void PMBench::testMesh(Ogre::MeshPtr mesh)
{
	LodConfig conf;
	LodLevel level;
	level.reductionMethod = LodLevel::VRM_COLLAPSE_COST;
	
	conf.mesh = mesh;

	level.distance = 2 * mesh->getBoundingSphereRadius();
	level.reductionValue = mesh->getBoundingSphereRadius() / 32;
	conf.levels.push_back(level);

	// runTest<OgreProgressiveMeshExt>(conf);
	//runTest<ProgressiveMeshGenerator>(conf);
}

void PMBench::configureTimerResolution()
{
	const UINT resolution = 1;   // 1 milisec

	TIMECAPS tc;
	UINT wTimerRes;

	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
		assert(0);
	}

	wTimerRes = std::min(std::max(tc.wPeriodMin, resolution), tc.wPeriodMax);
	timeBeginPeriod(wTimerRes);
	QueryPerformanceFrequency((LARGE_INTEGER*) &mTimerResolution);
	assert(mTimerResolution);
}

float PMBench::getDiffTime(__int64 ctrStart, __int64 ctrEnd)
{
	return (float) (ctrEnd - ctrStart) / (float) mTimerResolution;
}

int PMBench::LastInsertID(const std::string& table)
{
	int id;
	*mSQL << "select last_insert_rowid() from " << table, into(id);
	return id;
}

template<typename GEN>
void PMBench::runTest(LodConfig& config)
{
	__int64 ctrBegin = 0, ctrEnd = 0;

	QueryPerformanceCounter((LARGE_INTEGER*) &ctrBegin);

	for (int i = 0; i < 2; i++) {
		GEN generator;
		generator.build(config);
	}

	QueryPerformanceCounter((LARGE_INTEGER*) &ctrEnd);

	std::string meshName = config.mesh->getName();
	std::string typeName = typeid(GEN).name();
	typeName = typeName.substr(typeName.find_last_of(":") + 1);
	float diff = getDiffTime(ctrBegin, ctrEnd) / 10.0f;
	size_t meshVertexCount = Ember::OgreView::Gui::MeshInfoProvider::calcUniqueVertexCount(config.mesh.get());
	std::string date = getDate(&mDate);

	std::stringstream str;

	str << "insert into benchmarks(MeshName, GeneratorName, CalcTime, MeshVertexCount, RunDate) "
	       "values(" <<
	"'" << meshName << "', " <<
	"'" << typeName << "', " <<
	"'" << diff << "', " <<
	"'" << meshVertexCount << "', " <<
	"'" << date << "');";

	*mSQL << str.str();
	int benchmarkID = LastInsertID("Benchmarks");


	Ogre::Entity* entity = mRenderContext->getSceneNode()->getCreator()->createEntity("EntityName", config.mesh->getName());
	mRenderContext->getSceneNode()->attachObject(entity);
	mRenderContext->showFull(entity);
	unsigned short lodLevels = config.levels.size();
	for (int i = 0; i < lodLevels; i++) {
		LodLevel& level = config.levels[i];
		*mSQL << "insert into lodlevels(BenchmarkID, Distance, ReductionMethod, ReductionValue, VertexCount) "
		         "values(" <<
		"'" << benchmarkID << "', " <<
		"'" << level.distance << "', " <<
		"'" << level.reductionMethod << "', " <<
		"'" << level.reductionValue << "', " <<
		"'" << level.outUniqueVertexCount << "')";

		int levelID = LastInsertID("LodLevels");
		Ogre::Real userValue = config.mesh->getLodStrategy()->transformUserValue(config.levels[i].distance);
		unsigned short lodIndex = config.mesh->getLodIndex(userValue);
		entity->setMeshLodBias(0.0001f, lodIndex, lodIndex);
		renderImage(config, i, levelID, typeName);
	}
	mRenderContext->getSceneNode()->detachAllObjects();
	mRenderContext->getSceneNode()->getCreator()->destroyEntity(entity);
}
void PMBench::renderImage(LodConfig& config, int i, int levelID, std::string& typeName)
{
	mRenderContext->getSceneManager()->setAmbientLight(Ogre::ColourValue(0.7, 0.7, 0.7));
	mRenderContext->setBackgroundColour(Ogre::ColourValue::ZERO);
	float dist = config.levels[i].distance;
	mRenderContext->getCamera()->setNearClipDistance(dist / 100);
	mRenderContext->getCamera()->setFarClipDistance(dist * 2);


	mRenderContext->repositionCamera();

	mRenderContext->getRenderTexture()->update();
	std::string meshName(config.mesh->getName());
	size_t start = meshName.find_last_of("/\\");
	if (start != std::string::npos) {
		meshName = meshName.substr(start + 1);
	}
	size_t end = meshName.find_last_of(".");
	if (end != std::string::npos) {
		meshName = meshName.substr(0, end);
	}
	_mkdir(std::string(mImagesDir + "/" + meshName).c_str());
	std::stringstream str;
	str << meshName;
	str << "/" << levelID << "-" << typeName << "-";
	switch (config.levels[i].reductionMethod) {
	case LodLevel::VRM_PROPORTIONAL:
		str << (config.levels[i].reductionValue * 100.f) << "percentage";
		break;

	case LodLevel::VRM_COLLAPSE_COST:
		str << config.levels[i].reductionValue << "collapsecost";
		break;

	case LodLevel::VRM_CONSTANT:
		str << config.levels[i].reductionValue << "vertices";
		break;

	default:
		assert(0);
	}
	str << ".png";

	*mSQL << "insert into images(LodLevelID, Path) "
	         "values(" <<
	"'" << levelID << "', " <<
	"'" << str.str() << "');";

	mRenderContext->getRenderTexture()->writeContentsToFile(mImagesDir + "/" + str.str());
}
std::string PMBench::getDate(tm* tmdate)
{
	char buffer[16];
	strftime(buffer, 16, "%Y.%m.%d.", tmdate);
	return std::string(buffer);
}
void PMBench::startHtml(std::ostream& stream)
{
	stream << "<html><header></header><body>\n";
}
void PMBench::endHtml(std::ostream& stream)
{
	stream << "</body></html>";
}
void PMBench::appendToStream(std::ostream& stream, const soci::row& r, int i)
{
	const soci::column_properties& props = r.get_properties(i);
	switch (props.get_data_type()) {
	case soci::dt_string:
		stream << r.get<std::string>(i);
		break;

	case soci::dt_double:
		stream << r.get<double>(i);
		break;

	case soci::dt_integer:
		stream << r.get<int>(i);
		break;

	case soci::dt_unsigned_long:
		stream << r.get<unsigned long>(i);
		break;

	case soci::dt_long_long:
		stream << r.get<long long>(i);
		break;

	case soci::dt_date:
		tm when = r.get<tm>(i);
		stream << getDate(&when);
		break;
	}
}
void PMBench::printSQLToHTML(const std::string& sqlQuery, const std::string& outputFilePath)
{
	soci::rowset<soci::row> rs = (mSQL->prepare << sqlQuery);

	if (rs.begin() == rs.end()) {
		std::string err = "No results found for query: ";
		err += sqlQuery;
		S_LOG_WARNING(err);
		return;
	}

	std::ofstream outfile;
	outfile.open(std::string(mWorkDir + "/" + outputFilePath).c_str());
	startHtml(outfile);

	outfile << sqlQuery << "<br><br><br>\n";

	outfile << "<table border=\"1\">\n<tr>";
	soci::rowset<soci::row>::const_iterator it = rs.begin();
	for (size_t i = 0; i != it->size(); i++) {
		outfile << "<th>" << it->get_properties(i).get_name() << "</th>";
	}
	outfile << "</tr>\n";
	for (; it != rs.end(); it++) {
		outfile << "<tr>";
		for (size_t i = 0; i < it->size(); i++) {
			std::string str;
			outfile << "<td>";
			appendToStream(outfile, *it, i);
			outfile << "</td>";
		}
		outfile << "</tr>\n";
	}
	outfile << "</table>\n";

	endHtml(outfile);
}
void PMBench::createCompareOgreVSGeneratorView()
{
	char* sqlQuery =
	    "select "
	        "b1.MeshName as MeshName, "
	        "(b1.CalcTime || ' sec') as OgreProgressiveMeshExt, "
	        "(b2.CalcTime || ' sec') as ProgressiveMeshGenerator, "
	        "(round(b1.CalcTime/b2.CalcTime, 2) || 'x') as Speedup "
	    "from "
	        "benchmarks as b1, "
	        "benchmarks as b2 "
	    "where "
	        "b1.MeshName = b2.MeshName and "
	        "b1.GeneratorName = 'OgreProgressiveMeshExt' and "
	        "b2.GeneratorName = 'ProgressiveMeshGenerator';";

	printSQLToHTML(sqlQuery, "OgreVSGeneratorSpeedTest.html");


}
void PMBench::createImageView()
{
	char* sqlQuery =
	    "select "
	        "b.MeshName as MeshName, "
	        "(round((1-cast(l.VertexCount as real)/cast(b.MeshVertexCount as real))*100, 2) || '%') as Reduction, "
	        "('<a href=\"images/' || i.Path || '\">view image</a>') as ImagePath "
	    "from "
	        "benchmarks as b, "
	        "lodlevels as l, "
	        "images as i "
	    "where "
	        "b.ID = l.BenchmarkID and "
	        "l.ID = i.LodLevelID;";

	printSQLToHTML(sqlQuery, "ImageDB.html");
}
void PMBench::createImageViewDirect()
{
	char* sqlQuery =
	    "select "
	        "b.MeshName as MeshName, "
	        "i.Path as ImagePath, "
	        "(round((1-cast(l.VertexCount as real)/cast(b.MeshVertexCount as real))*100, 2) || '%') as Reduction "
	    "from "
	        "benchmarks as b, "
	        "lodlevels as l, "
	        "images as i "
	    "where "
	        "b.ID = l.BenchmarkID and "
	        "l.ID = i.LodLevelID;";

	std::string outputFilePath("ImageDirectView.html");
	soci::rowset<soci::row> rs = (mSQL->prepare << sqlQuery);

	if (rs.begin() == rs.end()) {
		std::string err = "No results found for query: ";
		err += sqlQuery;
		S_LOG_WARNING(err);
		return;
	}

	std::ofstream outfile;
	outfile.open(std::string(mWorkDir + "/" + outputFilePath).c_str());
	startHtml(outfile);

	outfile << sqlQuery << "<br><br><br>\n";

	soci::rowset<soci::row>::const_iterator it = rs.begin();
	for (; it != rs.end(); it++) {
		std::string meshName;
		std::string imagePath;
		std::string reductionPercentage;
		*it >> meshName >> imagePath >> reductionPercentage;
		outfile << "Reduced " << meshName << " by " << reductionPercentage << ".<br>\n";
		outfile << "<img src=\"images/" << imagePath << "\" /><br><br>\n";
	}

	endHtml(outfile);

}


}
}
}
