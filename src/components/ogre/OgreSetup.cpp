//
// C++ Implementation: OgreSetup
//
// Description:
//
//
// Author: Erik Ogenvik <erik@ogenvik.org>, (C) 2006
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
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.//
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "OgreSetup.h"
#include "OgreInfo.h"
#include "OgreConfigurator.h"
#include "MeshSerializerListener.h"
#include "lod/ScaledPixelCountLodStrategy.h"

#include "services/EmberServices.h"
#include "services/config/ConfigService.h"
#include "services/config/ConfigListenerContainer.h"
#include "services/input/Input.h"

#include "framework/Tokeniser.h"
#include "framework/ConsoleBackend.h"
#include "framework/MainLoopController.h"
#include "EmberWorkQueue.h"

#ifdef BUILD_WEBEMBER
#include "extensions/webember/WebEmberManager.h"
#endif

#include <RenderSystems/GL/OgreGLContext.h>

#ifdef _WIN32
#include "platform/platform_windows.h"
#endif

#include <OgreRenderWindow.h>
#include <OgreMeshManager.h>
#include <OgreStringConverter.h>
#include <OgreSceneManager.h>
#include <Overlay/OgreOverlaySystem.h>
#include <OgreLogManager.h>
#include <OgreRoot.h>
#include <OgreConfigDialog.h>
#include <OgreTextureManager.h>
#include <OgreLodStrategyManager.h>

#include <SDL.h>
#include <Ogre.h>

namespace Ember {
namespace OgreView {

OgreSetup::OgreSetup() :
		DiagnoseOgre("diagnoseOgre", this, "Diagnoses the current Ogre state and writes the output to the log."),
		mRoot(nullptr),
		mRenderWindow(nullptr),
		mSceneManagerFactory(nullptr),
		mMeshSerializerListener(nullptr),
		mOverlaySystem(nullptr),
#ifdef BUILD_WEBEMBER
		mOgreWindowProvider(nullptr),
#endif
		mConfigListenerContainer(nullptr),
		mSaveShadersToCache(false){
}

OgreSetup::~OgreSetup() {
#ifdef BUILD_WEBEMBER
	delete mOgreWindowProvider;
#endif
	delete mConfigListenerContainer;
}

void OgreSetup::runCommand(const std::string& command, const std::string& args) {
	if (DiagnoseOgre == command) {
		std::stringstream ss;
		OgreInfo::diagnose(ss);
		S_LOG_INFO(ss.str());
		ConsoleBackend::getSingleton().pushMessage("Ogre diagnosis information has been written to the log.", "info");
	}
}

void OgreSetup::shutdown() {
	S_LOG_INFO("Shutting down Ogre.");
	if (mRoot) {

		if (Ogre::GpuProgramManager::getSingletonPtr()) {
			try {
				auto cacheStream = mRoot->createFileStream(EmberServices::getSingleton().getConfigService().getHomeDirectory(BaseDirType_CACHE) + "/gpu-" VERSION ".cache");
				if (cacheStream) {
					Ogre::GpuProgramManager::getSingleton().saveMicrocodeCache(cacheStream);
				}
			} catch (...) {
				S_LOG_WARNING("Error when trying to save GPU cache file.");
			}
		}
		if (mSceneManagerFactory) {
			mRoot->removeSceneManagerFactory(mSceneManagerFactory);
			delete mSceneManagerFactory;
			mSceneManagerFactory = nullptr;
		}

		//This should normally not be needed, but there seems to be a bug in Ogre for Windows where it will hang if the render window isn't first detached.
		//The bug appears in Ogre 1.7.2.
		if (mRenderWindow) {
			mRoot->detachRenderTarget(mRenderWindow);
			mRenderWindow = nullptr;
		}
	}
	delete mOverlaySystem;
	mOverlaySystem = nullptr;
	delete mRoot;
	mRoot = nullptr;
	S_LOG_INFO("Ogre shut down.");

	delete mMeshSerializerListener;

}

Ogre::Root* OgreSetup::createOgreSystem() {
	ConfigService& configSrv(EmberServices::getSingleton().getConfigService());

	if (!configSrv.getPrefix().empty()) {
		//We need to set the current directory to the prefix before trying to load Ogre.
		//The reason for this is that Ogre loads a lot of dynamic modules, and in some build configuration
		//(like AppImage) the lookup path for some of these are based on the installation directory of Ember.
		if (chdir(configSrv.getPrefix().c_str())) {
			S_LOG_WARNING("Failed to change to the prefix directory '" << configSrv.getPrefix() << "'. Ogre loading might fail.");
		}
	}

	std::string pluginExtension = ".so";
	mRoot = new Ogre::Root("", configSrv.getHomeDirectory(BaseDirType_CONFIG) + "/ogre.cfg", "");
	//Ownership of the queue instance is passed to Root.
	mRoot->setWorkQueue(OGRE_NEW EmberWorkQueue(MainLoopController::getSingleton().getEventService()));

	mOverlaySystem = new Ogre::OverlaySystem();

	mPluginLoader.loadPlugin("Plugin_ParticleFX");
	mPluginLoader.loadPlugin("RenderSystem_GL"); //We'll use OpenGL on Windows too, to make it easier to develop

	if (chdir(configSrv.getEmberDataDirectory().c_str())) {
		S_LOG_WARNING("Failed to change to the data directory '" << configSrv.getEmberDataDirectory() << "'.");
	}

	return mRoot;
}

bool OgreSetup::showConfigurationDialog() {
	OgreConfigurator configurator;
	OgreConfigurator::Result result;
	try {
		result = configurator.configure();
	} catch (const std::exception& ex) {
		S_LOG_WARNING("Error when showing configuration window." << ex);
		delete mOverlaySystem;
		mOverlaySystem = nullptr;
		delete mRoot;
		mRoot = nullptr;
		createOgreSystem();
		throw ex;
	}
	delete mOverlaySystem;
	mOverlaySystem = nullptr;
	delete mRoot;
	mRoot = nullptr;
	if (result == OgreConfigurator::OC_CANCEL) {
		return false;
	}
	createOgreSystem();
	if (result == OgreConfigurator::OC_ADVANCED_OPTIONS) {
		Ogre::ConfigDialog* dlg = OGRE_NEW Ogre::ConfigDialog();
		bool isOk = mRoot->showConfigDialog(dlg);
		OGRE_DELETE dlg;

		if (!isOk) {
			return false;
		}
	} else {
		mRoot->setRenderSystem(mRoot->getRenderSystemByName(configurator.getChosenRenderSystemName()));
		const Ogre::ConfigOptionMap& configOptions = configurator.getConfigOptions();
		for (const auto& configOption : configOptions) {
			mRoot->getRenderSystem()->setConfigOption(configOption.first, configOption.second.currentValue);
		}
		mRoot->saveConfig();
	}
	return true;
}

void OgreSetup::Config_ogreLogChanged(const std::string& section, const std::string& key, varconf::Variable& variable) {
	if (variable.is_string()) {
		auto string = variable.as_string();
		if (string == "low") {
			Ogre::LogManager::getSingleton().getDefaultLog()->setLogDetail(Ogre::LL_LOW);
		} else if (string == "normal") {
			Ogre::LogManager::getSingleton().getDefaultLog()->setLogDetail(Ogre::LL_NORMAL);
		} else if (string == "boreme") {
			Ogre::LogManager::getSingleton().getDefaultLog()->setLogDetail(Ogre::LL_BOREME);
		}
	}
}

/** Configures the application - returns false if the user chooses to abandon configuration. */
Ogre::Root* OgreSetup::configure() {
	delete mConfigListenerContainer;
	mConfigListenerContainer = new ConfigListenerContainer();
	mConfigListenerContainer->registerConfigListener("ogre", "loglevel", sigc::mem_fun(*this, &OgreSetup::Config_ogreLogChanged), true);

	ConfigService& configService(EmberServices::getSingleton().getConfigService());
	createOgreSystem();
#ifndef BUILD_WEBEMBER
	bool success = false;
	bool suppressConfig = false;
	if (configService.itemExists("ogre", "suppressconfigdialog")) {
		suppressConfig = static_cast<bool>(configService.getValue("ogre", "suppressconfigdialog"));
	}
	try {
		success = mRoot->restoreConfig();
		if (!success || !suppressConfig) {
			success = showConfigurationDialog();
		}

	} catch (const std::exception& ex) {
		S_LOG_WARNING("Error when showing config dialog. Will try to remove ogre.cfg file and retry." << ex);
		unlink((EmberServices::getSingleton().getConfigService().getHomeDirectory(BaseDirType_CONFIG) + "ogre.cfg").c_str());
		try {
			Ogre::ConfigDialog* dlg = OGRE_NEW Ogre::ConfigDialog();
			success = mRoot->showConfigDialog(dlg);
			OGRE_DELETE dlg;
		} catch (const std::exception& ex) {
			S_LOG_CRITICAL("Could not configure Ogre. Will shut down." << ex);
		}
	}
	if (!success) {
		return nullptr;
	}

	// we start by trying to figure out what kind of resolution the user has selected, and whether full screen should be used or not
	unsigned int height = 768, width = 1024;
	bool fullscreen = false;

	parseWindowGeometry(mRoot->getRenderSystem()->getConfigOptions(), width, height, fullscreen);

	bool handleOpenGL = false;
#ifdef __APPLE__
	handleOpenGL = true;
#endif

	std::string windowId = Input::getSingleton().createWindow(width, height, fullscreen, true, true, handleOpenGL);

	mRoot->initialise(false, "Ember");
	Ogre::NameValuePairList misc;
#ifdef __APPLE__
	misc["currentGLContext"] = Ogre::String("true");
	misc["macAPI"] = Ogre::String("cocoa");
#else
//We should use "externalWindowHandle" on Windows, and "parentWindowHandle" on Linux.
#ifdef _WIN32
	misc["externalWindowHandle"] = windowId;
#else
	misc["parentWindowHandle"] = windowId;
#endif
#endif

	mRenderWindow = mRoot->createRenderWindow("MainWindow", width, height, fullscreen, &misc);

	Input::getSingleton().EventSizeChanged.connect(sigc::mem_fun(*this, &OgreSetup::input_SizeChanged));

	registerOpenGLContextFix();

	if (mSaveShadersToCache) {
		Ogre::GpuProgramManager::getSingleton().setSaveMicrocodesToCache(true);

		std::string cacheFilePath = configService.getHomeDirectory(BaseDirType_CACHE) + "/gpu-" VERSION ".cache";
		if (std::ifstream(cacheFilePath).good()) {
			try {
				auto cacheStream = mRoot->openFileStream(cacheFilePath);
				if (cacheStream) {
					Ogre::GpuProgramManager::getSingleton().loadMicrocodeCache(cacheStream);
				}
			} catch (...) {
				S_LOG_WARNING("Error when trying to open GPU cache file.");
			}
		}
	}

#else //BUILD_WEBEMBER == true
	//In webember we will disable the config dialog.
	//Also we will use fixed resolution and windowed mode.
	try {
		mRoot->restoreConfig();
	} catch (const std::exception& ex) {
		//this isn't a problem, we will set the needed functions manually.
	}
	Ogre::RenderSystem* renderer = mRoot->getRenderSystem();
#ifdef _WIN32
	//on windows, the default renderer is directX, we will force OpenGL.
	Ogre::RenderSystem* renderer = mRoot->getRenderSystemByName("OpenGL Rendering Subsystem");
	if(renderer != nullptr) {
		mRoot->setRenderSystem(renderer);
	} else {
		S_LOG_WARNING("OpenGL RenderSystem not found. Starting with default RenderSystem.");
		renderer = mRoot->getRenderSystem();
	}
#endif // _WIN32
	renderer->setConfigOption("Video Mode", "800 x 600");
	renderer->setConfigOption("Full Screen", "no");

	mRoot->initialise(false, "Ember");

	Ogre::NameValuePairList options;

	if (configService.itemExists("ogre", "windowhandle")) {
		//set the owner window
		std::string windowhandle = configService.getValue("ogre", "windowhandle");
		options["parentWindowHandle"] = windowhandle;

		//put it in the top left corner
		options["top"] = "0";
		options["left"] = "0";
	}

	mRenderWindow = mRoot->createRenderWindow("Ember",800,600,false,&options);
	mOgreWindowProvider = new OgreWindowProvider(*mRenderWindow);
	Input::getSingleton().attach(mOgreWindowProvider);

#endif // BUILD_WEBEMBER

	mRenderWindow->setActive(true);
	mRenderWindow->setAutoUpdated(true);
	mRenderWindow->setVisible(true);

	setStandardValues();


	mConfigListenerContainer->registerConfigListener("ogre", "profiler", [&](const std::string& section, const std::string& key, varconf::Variable& variable) {
		if (variable.is_bool()) {
			auto* profiler = Ogre::Profiler::getSingletonPtr();
			if (profiler) {
				if ((bool) variable) {
					auto& resourceGroupMgr = Ogre::ResourceGroupManager::getSingleton();
					if (!resourceGroupMgr.resourceGroupExists("Profiler")) {
						resourceGroupMgr.addResourceLocation(OGRE_MEDIA_DIR"/packs/profiler.zip", "Zip", "Profiler", true);
						resourceGroupMgr.addResourceLocation(OGRE_MEDIA_DIR"/packs/SdkTrays.zip", "Zip", "Profiler", true);
						resourceGroupMgr.initialiseResourceGroup("Profiler");
					}
				}

				if (profiler->getEnabled() != (bool) variable) {
					profiler->reset();
					profiler->setEnabled((bool) variable);
				}
			}
		}
	});

	// Create new scene manager factory
	//mSceneManagerFactory = new EmberPagingSceneManagerFactory();

	//// Register our factory
	//mRoot->addSceneManagerFactory(mSceneManagerFactory);

	return mRoot;
}

void OgreSetup::input_SizeChanged(unsigned int width, unsigned int height) {

//On Windows we can't tell the window to resize, since that will lead to an infinite loop of resize events (probably stemming from how Windows lacks a proper window manager).
#ifndef _WIN32
	mRenderWindow->resize(width, height);
#endif
	mRenderWindow->windowMovedOrResized();
}

void OgreSetup::setStandardValues() {
	// Set default mipmap level (NB some APIs ignore this)
	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

	// Set default animation mode
	Ogre::Animation::setDefaultInterpolationMode(Ogre::Animation::IM_SPLINE);

	//remove padding for bounding boxes
	Ogre::MeshManager::getSingletonPtr()->setBoundsPaddingFactor(0);

	//all new movable objects shall by default be unpickable; it's up to the objects themselves to make themselves pickable
	Ogre::MovableObject::setDefaultQueryFlags(0);

	//Default to require tangents for all meshes. This could perhaps be turned off on platforms which has no use, like Android?
	mMeshSerializerListener = new MeshSerializerListener(true);

	Ogre::MeshManager::getSingleton().setListener(mMeshSerializerListener);

	//We provide our own pixel size scaled LOD strategy. Note that ownership is transferred to the LodStrategyManager, hence we won't hold on to this instance.
	Ogre::LodStrategy* lodStrategy = OGRE_NEW Lod::ScaledPixelCountLodStrategy();
	Ogre::LodStrategyManager::getSingleton().addStrategy(lodStrategy);

}

void OgreSetup::parseWindowGeometry(Ogre::ConfigOptionMap& config, unsigned int& width, unsigned int& height, bool& fullscreen) {
	auto opt = config.find("Video Mode");
	if (opt != config.end()) {
		Ogre::String val = opt->second.currentValue;
		Ogre::String::size_type pos = val.find('x');
		if (pos != Ogre::String::npos) {

			width = Ogre::StringConverter::parseUnsignedInt(val.substr(0, pos));
			height = Ogre::StringConverter::parseUnsignedInt(val.substr(pos + 1));
		}
	}

	//now on to whether we should use fullscreen
	opt = config.find("Full Screen");
	if (opt != config.end()) {
		fullscreen = (opt->second.currentValue == "Yes");
	}

}

void OgreSetup::registerOpenGLContextFix() {
	/**
	 * This is needed to combat a bug found at least on KDE 4.14.4 when using OpenGL in the window manager.
	 * For some reason the OpenGL context of the application somtimes is altered when the window is minimized and restored.
	 * This results in segfaults when Ogre then tries to issue OpenGL commands.
	 * The exact cause and reasons for this bug are unknown, but by making sure that the OpenGL context is set each
	 * time the window is resized, minimized or restored we seem to avoid the bug.
	 *
	 */
	Ogre::GLContext* ogreGLcontext = nullptr;
	mRenderWindow->getCustomAttribute("GLCONTEXT", &ogreGLcontext);
	if (ogreGLcontext) {
		S_LOG_INFO("Registering OpenGL context loss fix.");
		Input::getSingleton().EventSDLEventReceived.connect([=](const SDL_Event& event) {
			if (event.type == SDL_WINDOWEVENT) {
				switch (event.window.event) {
					case SDL_WINDOWEVENT_SHOWN:
					case SDL_WINDOWEVENT_HIDDEN:
					case SDL_WINDOWEVENT_RESIZED:
					case SDL_WINDOWEVENT_SIZE_CHANGED:
					case SDL_WINDOWEVENT_MINIMIZED:
					case SDL_WINDOWEVENT_MAXIMIZED:
					case SDL_WINDOWEVENT_RESTORED:
						ogreGLcontext->setCurrent();
						break;
					default:
						break;
				}
			}
		});
	}
}

}
}
