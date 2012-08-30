//
// C++ Interface: OgreEntityRenderer
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
#ifndef EMBEROGREOGREENTITYRENDERER_H
#define EMBEROGREOGREENTITYRENDERER_H

#include "MovableObjectRenderer.h"

namespace Ember
{
namespace OgreView
{
namespace Gui
{

/**
 * Renders a single Ogre::Entity to a EntityCEGUITexture.
 *
 * @author Erik Hjortsberg
 */
class OgreEntityRenderer :
	public MovableObjectRenderer
{
public:
	OgreEntityRenderer(CEGUI::Window* image);

	virtual ~OgreEntityRenderer();

	/**
	 * Renders the submitted Entity.
	 * @param modelName a mesh namel
	 */
	void showEntity(const std::string& mesh);

	/**
	 * @brief Unloads the Entity.
	 *
	 * The getEntity() will return nullptr after this call.
	 * You can call showEntity() after this call to load an entity again.
	 */
	void unloadEntity();

	/**
	 * @brief Returns the scene manager used by OgreEntityRenderer
	 */
	Ogre::SceneManager* getSceneManager();

	/**
	 * Returns the current rendered Entity, or null if none is set.
	 * @return
	 */
	Ogre::Entity* getEntity();
	bool getWireframeMode();
	void setWireframeMode(bool enabled);

	void setForcedLodLevel(int lodLevel);
	void clearForcedLodLevel();

protected:

	Ogre::Entity* mEntity;

	virtual Ogre::MovableObject* getMovableObject();
	void setEntity(Ogre::Entity* entity);


};
}
}

}

#endif // ifndef EMBEROGREOGREENTITYRENDERER_H
