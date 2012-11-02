//
// C++ Implementation: EntityRecipe
//
// Description:
//
//
// Author: Alexey Torkhov <atorkhov@gmail.com>, (C) 2008
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "EntityRecipe.h"
#include "components/lua/LuaScriptingCallContext.h"
#include "components/lua/LuaScriptingProvider.h"
#include "services/scripting/ScriptingService.h"
#include "services/EmberServices.h"

#include <Atlas/Message/Element.h>
#include <Atlas/Objects/Operation.h>
#include <Atlas/Message/QueuedDecoder.h>
#include <Atlas/Codecs/XML.h>

#include <Eris/TypeInfo.h>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
//#include <lua.hpp>
#include <tolua++.h>

namespace Ember
{
namespace OgreView
{
namespace Authoring
{
EntityRecipe::EntityRecipe(Ogre::ResourceManager* creator, const Ogre::String& name, Ogre::ResourceHandle handle, const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader) :
	Resource(creator, name, handle, group, isManual, loader), mEntitySpec(0)
{
	if (createParamDictionary("EntityRecipe")) {
		// no custom params
	}
}

EntityRecipe::EntityRecipe(const std::string& name, const std::string& entityType) :
	mEntitySpec(0), mEntityType(entityType)
{
	mName = name;
}


EntityRecipe::~EntityRecipe()
{
	for (GUIAdaptersStore::iterator I = mGUIAdapters.begin(); I != mGUIAdapters.end(); ++I) {
		delete I->second;
	}

	for (BindingsStore::iterator I = mBindings.begin(); I != mBindings.end(); ++I) {
		delete I->second;
	}

	delete mEntitySpec;
}

void EntityRecipe::loadImpl(void)
{
}

void EntityRecipe::unloadImpl(void)
{
}

size_t EntityRecipe::calculateSize(void) const
{
	//TODO:implement this
	return 0;
}

const std::string& EntityRecipe::getEntityType() const
{
	return mEntityType;
}

GUIAdapter* EntityRecipe::createGUIAdapter(const std::string& name, const std::string& type, const std::string& tooltip)
{
	GUIAdapter* adapter;
	adapter = new GUIAdapter(type);
	adapter->setTooltip(tooltip);
	mGUIAdapters[name] = adapter;
	adapter->EventValueChanged.connect(sigc::mem_fun(*this, &EntityRecipe::valueChanged));
	return adapter;
}

GUIAdapter* EntityRecipe::getGUIAdapter(const std::string& name)
{
	GUIAdaptersStore::iterator adapter;
	if ((adapter = mGUIAdapters.find(name)) != mGUIAdapters.end()) {
		return adapter->second;
	} else {
		return nullptr;
	}
}

const GUIAdaptersStore& EntityRecipe::getGUIAdapters()
{
	return mGUIAdapters;
}

GUIAdapterBindings* EntityRecipe::createGUIAdapterBindings(const std::string& name)
{
	GUIAdapterBindings* adapterBindings;
	adapterBindings = new GUIAdapterBindings();
	mBindings[name] = adapterBindings;
	return adapterBindings;
}

void EntityRecipe::associateBindings()
{
	S_LOG_VERBOSE("Associating bindings.");
	if (mEntitySpec) {
		// Iterate over all entity spec XML nodes
		EntityRecipe::SpecIterator iter(this);
		TiXmlElement *elem = mEntitySpec->FirstChildElement("atlas");
		if (elem) {
			elem->Accept(&iter);
		}
	}
}

EntityRecipe::SpecIterator::SpecIterator(EntityRecipe* recipe) :
	TiXmlVisitor(), mRecipe(recipe)
{
}

bool EntityRecipe::SpecIterator::Visit(const TiXmlText& textNode)
{
	// We should be the only child of our parent
	if (textNode.Parent()->FirstChild() != textNode.Parent()->LastChild()) {
		return false;
	}

	std::string text = textNode.ValueStr();

	// If text looks like placeholder, try to look up it in bindings and associate if found
	if (!text.empty() && text.at(0) == '$') {
		BindingsStore::iterator bindings = mRecipe->mBindings.find(text.substr(1));
		if (bindings != mRecipe->mBindings.end()) {
			bindings->second->associateXmlElement(const_cast<TiXmlNode&> (*textNode.Parent()));
			S_LOG_VERBOSE("Associated " << bindings->first << " with " << text);
		} else {
			S_LOG_WARNING("Binding for " << text << " not found.");
		}
	}

	return true;
}

Atlas::Message::MapType EntityRecipe::createEntity(Eris::TypeService& typeService)
{
	S_LOG_VERBOSE("Creating entity.");

	ScriptingService& scriptingService = EmberServices::getSingleton().getScriptingService();
	// Loading script code
	scriptingService.executeCode(mScript, "LuaScriptingProvider");

	// Walking through adapter bindings
	for (BindingsStore::iterator I = mBindings.begin(); I != mBindings.end(); ++I) {
		const std::string& func = I->second->getFunc();

		S_LOG_VERBOSE(" binding: " << I->first << " to func " << func);

		if (func.empty()) {
			std::vector<std::string>& adapters = I->second->getAdapters();

			if (adapters.size() == 1) {
				std::string adapterName = adapters[0];
				Atlas::Message::Element val = mGUIAdapters[adapterName]->getValue();
				I->second->setValue(val);
			} else {
				S_LOG_WARNING("Should be only one adapter without calling function.");
			}
		} else {
			Lua::LuaScriptingCallContext callContext;

			lua_State* L = static_cast<Lua::LuaScriptingProvider*> (scriptingService.getProviderFor("LuaScriptingProvider"))->getLuaState();

			// Pushing function params
			std::vector<std::string>& adapters = I->second->getAdapters();
			for (std::vector<std::string>::iterator J = adapters.begin(); J != adapters.end(); J++) {
				std::string adapterName = *J;
				Atlas::Message::Element* val = new Atlas::Message::Element(mGUIAdapters[adapterName]->getValue());
				tolua_pushusertype_and_takeownership(L, val, "Atlas::Message::Element");
			}

			// Calling test function
			scriptingService.callFunction(func, adapters.size(), "LuaScriptingProvider", &callContext);

			LuaRef returnValue(callContext.getReturnValue());

			Atlas::Message::Element returnObj;
			returnObj = returnValue.asObject<Atlas::Message::Element> ("Atlas::Message::Element");
			I->second->setValue(returnObj);
		}
	}
	//Inject all default attributes that aren't yet added.
	// 	TiXmlElement *elem = mEntitySpec->FirstChildElement("atlas");
	// 	if (elem)
	// 	{
	// 		Eris::TypeInfo* erisType = mConn->getTypeService()->getTypeByName(getEntityType());
	// 		if (erisType) {
	// 			const Atlas::Message::MapType& defaultAttributes = erisType->getAttributes();
	// 			for (Atlas::Message::MapType::const_iterator I = defaultAttributes.begin(); I != defaultAttributes.end(); ++I) {
	// 				bool hasAttribute = false;
	// 				TiXmlNode* child(0);
	// 				while(child = elem->IterateChildren(child)) {
	// 					if (child->ToElement()) {
	// 						if (std::string(child->ToElement()->Attribute("name")) == I->first) {
	// 							hasAttribute = true;
	// 							break;
	// 						}
	// 					}
	// 				}
	//
	// 				if (!hasAttribute) {
	// 					//The attribute isn't present, we'll inject it
	// 					//This a bit contrived, since we'll now first convert the atlas into xml and inject it into the TiXmlElement (which will convert the xml strings into TiXml structures). And then later on we'll parse the xml again and create the final atlas data from it. However, the main reason for doing it this way is that in the future we would want to have nested child elements, which could be repeated. And in those cases we'll want to work directly with xml.
	// 				}
	// 			}
	// 		}
	// 	}
	/*
	 std::stringstream str;

	 Atlas::Message::Element element(message);

	 Atlas::Message::QueuedDecoder decoder;

	 Atlas::Codecs::XML codec(str, decoder);
	 Atlas::Formatter formatter(str, codec);
	 Atlas::Message::Encoder encoder(formatter);
	 formatter.streamBegin();
	 encoder.streamMessageElement(message);
	 formatter.streamEnd();
	 */
	if (mEntitySpec) {
		// Print entity into string
		TiXmlPrinter printer;
		printer.SetStreamPrinting();
		mEntitySpec->Accept(&printer);

		S_LOG_VERBOSE("Composed entity: " << printer.Str());

		std::stringstream strStream(printer.CStr(), std::ios::in);

		// Create objects
		Atlas::Message::QueuedDecoder decoder;
		Atlas::Codecs::XML codec(strStream, decoder);

		// Read whole stream into decoder queue
		while (!strStream.eof()) {
			codec.poll();
		}

		// Read decoder queue
		while (decoder.queueSize() > 0) {
			Atlas::Message::MapType m = decoder.popMessage();
			Eris::TypeInfo* erisType = typeService.getTypeByName(getEntityType());
			if (erisType) {
				const Atlas::Message::MapType& defaultAttributes = erisType->getAttributes();
				for (Atlas::Message::MapType::const_iterator I = defaultAttributes.begin(); I != defaultAttributes.end(); ++I) {
					if (m.find(I->first) == m.end()) {
						m.insert(Atlas::Message::MapType::value_type(I->first, I->second));
					}
				}
			}
			return m;
		}
	} else {
		Atlas::Message::MapType msg;
		msg["parents"] = Atlas::Message::ListType(1, mEntityType);
		msg["name"] = getName();
		return msg;
	}
	S_LOG_WARNING("No entity composed");
	return Atlas::Message::MapType();
}

void EntityRecipe::setAuthor(const std::string& author)
{
	mAuthor = author;
}

const std::string& EntityRecipe::getAuthor() const
{
	return mAuthor;
}

void EntityRecipe::setDescription(const std::string& description)
{
	mDescription = description;
}

const std::string& EntityRecipe::getDescription() const
{
	return mDescription;
}

void EntityRecipe::valueChanged()
{
	EventValueChanged.emit();
}

/*
 void EntityRecipe::doTest()
 {
 S_LOG_VERBOSE("Doing test.");

 LuaScriptingCallContext callContext;

 // Loading code
 EmberServices::getSingleton().getScriptingService().executeCode(mScript, "LuaScriptingProvider");

 // Calling test function
 EmberServices::getSingleton().getScriptingService().callFunction("fTest", 0, "LuaScriptingProvider", &callContext);

 LuaRef returnValue( callContext.getReturnValue() );

 S_LOG_VERBOSE(lua_typename(0, returnValue.type()));

 Atlas::Message::Element returnObj;
 returnObj = returnValue.asObject<Atlas::Message::Element>("Atlas::Message::Element");

 S_LOG_VERBOSE("Returned element type is " << returnObj.getType());
 }
 */
}
}
}
