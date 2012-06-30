//
// C++ Interface: OgreSetup
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
#ifndef EMBEROGREOGRESETUP_H
#define EMBEROGREOGRESETUP_H

#include "EmberOgrePrerequisites.h"
#include "OgreIncludes.h"
#include "framework/ConsoleCommandWrapper.h"
#include "framework/ConsoleObject.h"
#include <OgreConfigOptionMap.h>
#include <OgreFrameListener.h>

struct SDL_Surface;

namespace Ember {
namespace OgreView {

class EmberPagingSceneManager;
class EmberPagingSceneManagerFactory;
class MeshSerializerListener;

namespace Lod
{
	class EmberOgreRoot;
}
/**
	@brief A class used for setting up Ogre.

	Instead of creating the Ogre root object and the main render window directly, use this to guarantee that everything is set up correctly.

	@author Erik Hjortsberg <erik.hjortsberg@gmail.com>
*/
class OgreSetup : public Ogre::FrameListener, public ConsoleObject
{
public:
	OgreSetup();

	virtual ~OgreSetup();

	/**
	* Creates the Ogre base system.
	* @return The new Ogre Root object.
	*/
	Ogre::Root* createOgreSystem();

	/**
	 * @brief Configures the application - returns false if the user chooses to abandon configuration.
	 * @return True if everything was correctly set up, else false.
	 */
	bool configure();

	/**
	 * @brief Gets the main render window.
	 * @return The render window.
	 */
	Ogre::RenderWindow* getRenderWindow() const;

	/**
	 * @brief Chooses and sets up the correct scene manager.
	 * @return The newly created scene manager.
	 */
	Ogre::SceneManager* chooseSceneManager();

	/**
	* @brief Shuts down the Ogre system.
	*/
	void shutdown();

	/**
	 * @brief Swap the double buffers every frame.
	 * @param evt The Ogre frame event.
	 * @return Always return true.
	 */
	bool frameEnded(const Ogre::FrameEvent & evt);

	virtual void runCommand(const std::string& command, const std::string& args);


	/**
	 * @brief Command for simple diagnosis of Ogre.
	 */
	ConsoleCommandWrapper DiagnoseOgre;

private:

	/**
	 * @brief Holds the Ogre root object.
	 */
	Lod::EmberOgreRoot* mRoot;

	/**
	 * @brief Holds the main render window.
	 */
	Ogre::RenderWindow* mRenderWindow;

	/**
	The icon shown in the top of the window.
	*/
	SDL_Surface* mIconSurface;

	/**
	We'll use our own scene manager factory.
	*/
	EmberPagingSceneManagerFactory* mSceneManagerFactory;

	/**
	The main video surface. Since we use SDL for input we need SDL to handle it too.
	*/
	SDL_Surface* mMainVideoSurface;

	/**
	* @brief Provides the ability to use relative paths for skeletons in meshes.
	*/
	MeshSerializerListener* mMeshSerializerListener;

	/**
	 * @brief Attempts to parse out the user selected geometry options for Ogre.
	 * @param config The option map, usually found inside the currently selected RenderSystem.
	 * @param width The width of the window in pixels.
	 * @param height The height of the window in pixels.
	 * @param fullscreen Whether fullscreen should be used or not.
	 */
	void parseWindowGeometry(Ogre::ConfigOptionMap& config, unsigned int& width, unsigned int& height, bool& fullscreen);

	/**
	 * @brief Sets standard values in the Ogre environment.
	 */
	void setStandardValues();

	/**
	 * @brief Checks for the named gl extension.
	 * @param extension The name of the extension to check for.
	 * @return True if the extension was found.
	 */
	int isExtensionSupported(const char *extension);

	/**
	 * @brief Listen to alt+tab and release the mouse if grabbed.
	 */
	void Input_AltTab();

};

inline Ogre::RenderWindow* OgreSetup::getRenderWindow() const
{
	return mRenderWindow;
}

}

}

#endif
