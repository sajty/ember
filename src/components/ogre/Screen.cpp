/*
 Copyright (C) 2011 Erik Ogenvik

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

#include "Screen.h"
#include "camera/Recorder.h"

#include "services/EmberServices.h"
#include "services/config/ConfigService.h"

#include "framework/osdir.h"
#include "framework/Exception.h"
#include "framework/LoggingInstance.h"
#include "framework/ConsoleBackend.h"

#include <OgreCamera.h>
#include <OgreRenderWindow.h>
#include <OgreRoot.h>
#include <OgreRenderSystem.h>
#include <OgreRenderTarget.h>

namespace Ember
{
namespace OgreView
{

Screen::Screen(Ogre::RenderWindow& window) :
		ToggleRendermode("toggle_rendermode", this, "Toggle between wireframe and solid render modes."), ToggleFullscreen("toggle_fullscreen", this, "Switch between windowed and full screen mode."), Screenshot("screenshot", this, "Take a screenshot and write to disk."), Record("+record", this, "Record to disk."), mWindow(window), mRecorder(new Camera::Recorder()), mPolygonMode(Ogre::PM_SOLID)
{
}

Screen::~Screen()
{
	delete mRecorder;
}

void Screen::runCommand(const std::string &command, const std::string &args)
{
	if (Screenshot == command) {
		//just take a screen shot
		takeScreenshot();
	} else if (ToggleFullscreen == command) {
		Ogre::ConfigOptionMap& configOptions = Ogre::Root::getSingleton().getRenderSystem()->getConfigOptions();
		const Ogre::ConfigOptionMap::iterator opti = configOptions.find("Video Mode");
		if (opti != configOptions.end()) {
			Ogre::StringVector vmopts = Ogre::StringUtil::split(opti->second.currentValue, " x");
			unsigned int w = Ogre::StringConverter::parseUnsignedInt(vmopts[0]);
			unsigned int h = Ogre::StringConverter::parseUnsignedInt(vmopts[1]);
			mWindow.setFullscreen(!mWindow.isFullScreen(), w, h);
		} else {
			S_LOG_FAILURE("Failed to toogle fullscreen mode, because the Current RenderSystem hasn't got a 'Video Mode' option.");
		}
		
	} else if (ToggleRendermode == command) {
		toggleRenderMode();
	} else if (Record == command) {
		mRecorder->startRecording();
	} else if (Record.getInverseCommand() == command) {
		mRecorder->stopRecording();
	}
}

void Screen::toggleRenderMode()
{

	if (mPolygonMode == Ogre::PM_SOLID) {
		mPolygonMode = Ogre::PM_WIREFRAME;
	} else {
		mPolygonMode = Ogre::PM_SOLID;
	}

	Ogre::RenderSystem::RenderTargetIterator renderTargetI = Ogre::Root::getSingleton().getRenderSystem()->getRenderTargetIterator();

	for (Ogre::RenderSystem::RenderTargetIterator::iterator I = renderTargetI.begin(); I != renderTargetI.end(); ++I) {
		Ogre::RenderTarget* renderTarget = I->second;
		for (unsigned short i = 0; i < renderTarget->getNumViewports(); ++i) {
			Ogre::Camera* camera = renderTarget->getViewport(i)->getCamera();
			if (camera) {
				camera->setPolygonMode(mPolygonMode);
			}
		}

	}

}

const std::string Screen::_takeScreenshot()
{
	// retrieve current time
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	// construct filename string
	// padding with 0 for single-digit values
	std::stringstream filename;
	filename << "screenshot_" << ((*timeinfo).tm_year + 1900); // 1900 is year "0"
	int month = ((*timeinfo).tm_mon + 1); // January is month "0"
	if (month <= 9) {
		filename << "0";
	}
	filename << month;
	int day = (*timeinfo).tm_mday;
	if (day <= 9) {
		filename << "0";
	}
	filename << day << "_";
	int hour = (*timeinfo).tm_hour;
	if (hour <= 9) {
		filename << "0";
	}
	filename << hour;
	int min = (*timeinfo).tm_min;
	if (min <= 9) {
		filename << "0";
	}
	filename << min;
	int sec = (*timeinfo).tm_sec;
	if (sec <= 9) {
		filename << "0";
	}
	filename << sec << ".jpg";

	const std::string dir = EmberServices::getSingleton().getConfigService().getHomeDirectory() + "/screenshots/";
	try {
		//make sure the directory exists

		oslink::directory osdir(dir);

		if (!osdir.isExisting()) {
			oslink::directory::mkdir(dir.c_str());
		}
	} catch (const std::exception& ex) {
		S_LOG_FAILURE("Error when creating directory for screenshots." << ex);
		throw Exception("Error when saving screenshot.");
	}

	try {
		// take screenshot
		mWindow.writeContentsToFile(dir + filename.str());
	} catch (const std::exception& ex) {
		S_LOG_FAILURE("Could not write screenshot to disc." << ex);
		throw Exception("Error when saving screenshot.");
	}
	return dir + filename.str();
}

void Screen::takeScreenshot()
{
	try {
		const std::string& result = _takeScreenshot();
		S_LOG_INFO("Screenshot saved at: " << result);
		ConsoleBackend::getSingletonPtr()->pushMessage("Wrote image: " + result, "info");
	} catch (const std::exception& ex) {
		ConsoleBackend::getSingletonPtr()->pushMessage(std::string("Error when saving screenshot: ") + ex.what(), "error");
	} catch (...) {
		ConsoleBackend::getSingletonPtr()->pushMessage("Unknown error when saving screenshot.", "error");
	}
}
}
}
