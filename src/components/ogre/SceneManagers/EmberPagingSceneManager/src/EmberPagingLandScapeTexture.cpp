//
// C++ Implementation: EmberPagingLandScapeTexture
//
// Description:
//
//
// Author: Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2005
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
#include "OgrePagingLandScapePrecompiledHeaders.h"

#include "EmberPagingLandScapeTexture.h"
#include "EmberPagingSceneManager.h"
#include "OgrePagingLandScapeTextureManager.h"
#include <memory>

namespace Ember
{
namespace OgreView
{

EmberPagingLandScapeTexture::EmberPagingLandScapeTexture(Ogre::PagingLandScapeTextureManager* pageMgr) :
	Ogre::PagingLandScapeTexture(pageMgr, "EmberTexture", 1, false)
{
}

EmberPagingLandScapeTexture::~EmberPagingLandScapeTexture()
{
}

Ogre::PagingLandScapeTexture* EmberPagingLandScapeTexture::newTexture()
{
	return new EmberPagingLandScapeTexture(mParent);
}

bool EmberPagingLandScapeTexture::isMaterialSupported(bool recursive)
{
	//TODO: check for stuff here
	return true;
}

void EmberPagingLandScapeTexture::setOptions()
{
}

void EmberPagingLandScapeTexture::_loadMaterial()
{
	EmberPagingSceneManager* emberPagingSceneManager = static_cast<EmberPagingSceneManager*> (mParent->getSceneManager());
	IPageDataProvider* provider = emberPagingSceneManager->getProvider();
	if (provider) {
		std::unique_ptr<IPageData> pageData(provider->getPageData(IPageDataProvider::OgreIndex(mDataX, mDataZ)));
		mMaterial = pageData->getMaterial();
	}
}

void EmberPagingLandScapeTexture::_unloadMaterial()
{
	S_LOG_VERBOSE("Unloading terrain material.");
}

}
}
