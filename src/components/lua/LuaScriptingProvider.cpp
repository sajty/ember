//
// C++ Implementation: LuaScriptingProvider
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "LuaScriptingProvider.h"
#include "luaobject.h"

#include "framework/Exception.h"
#include "framework/LoggingInstance.h"
#include "services/EmberServices.h"
#include "services/config/ConfigService.h"


// include Lua libs and tolua++
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
//#include <lua.hpp>
#include <tolua++.h>
#include "LuaHelper.h"
#include "LuaScriptingCallContext.h"



namespace Ember {

namespace Lua {

LuaScriptingProvider::LuaScriptingProvider()
: mService(0), mErrorHandlingFunctionName("")
{
	initialize();
}


LuaScriptingProvider::~LuaScriptingProvider()
{
	S_LOG_INFO("Shutting down lua environment.");
	lua_close(mLuaState);
}

void LuaScriptingProvider::stop()
{
	try {
		//we want to clear up the lua environment without destroying it (lua_close destroys it)
		std::string shutdownScript("for key,value in pairs(_G) do if key ~= \"_G\" and key ~= \"pairs\" then _G[key] = nil end end");
		{
			executeScript(shutdownScript, 0);
		}
		forceGC();
	} catch (...) {
		S_LOG_WARNING("Error when stopping lua.");
	}
}

void LuaScriptingProvider::initialize()
{
	createState();

}

void LuaScriptingProvider::createState()
{
	bool loadDebugLib = true;
	if (EmberServices::getSingleton().getConfigService().itemExists("lua", "debug")) {
		loadDebugLib = static_cast<bool>(EmberServices::getSingleton().getConfigService().getValue("lua", "debug"));
	}
	if (loadDebugLib) {
		mErrorHandlingFunctionName = "debug.traceback";
		S_LOG_VERBOSE("Loading lua debug library.");
	}


	static const luaL_Reg lualibs[] = {
		{"", luaopen_base},
		{LUA_LOADLIBNAME, luaopen_package},
		{LUA_TABLIBNAME, luaopen_table},
		{LUA_IOLIBNAME, luaopen_io},
		{LUA_STRLIBNAME, luaopen_string},
		{LUA_MATHLIBNAME, luaopen_math},
		{nullptr, nullptr}
		};
	static const luaL_Reg lualibsDebug[] = {
		{"", luaopen_base},
		{LUA_LOADLIBNAME, luaopen_package},
		{LUA_TABLIBNAME, luaopen_table},
		{LUA_IOLIBNAME, luaopen_io},
		{LUA_STRLIBNAME, luaopen_string},
		{LUA_MATHLIBNAME, luaopen_math},
		{LUA_DBLIBNAME, luaopen_debug},
		{nullptr, nullptr}
		};

	mLuaState = lua_open();

	// init all standard libraries

	const luaL_Reg *lib = loadDebugLib ? lualibsDebug : lualibs;
	for (; lib->func; lib++)
	{
		lua_pushcfunction(mLuaState, lib->func);
		lua_pushstring(mLuaState, lib->name);
		lua_call(mLuaState, 1, 0);
	}


// 	lua_pushcfunction(mLuaState, ::OgreView::Scripting::LuaHelper::luaErrorHandler);
// 	mErrorHandlingFunctionIndex = luaL_ref(mLuaState, LUA_REGISTRYINDEX);


}

// int LuaScriptingProvider::getErrorHandlingFunctionIndex() const
// {
// 	return mErrorHandlingFunctionIndex;
// }

const std::string& LuaScriptingProvider::getErrorHandlingFunctionName() const
{
	return mErrorHandlingFunctionName;
}

lua_State* LuaScriptingProvider::getLuaState()
{
	return mLuaState;
}



void LuaScriptingProvider::loadScript(ResourceWrapper& resWrapper)
{
	executeScriptImpl(std::string(resWrapper.getDataPtr(), resWrapper.getSize()), 0, resWrapper.getName());
}

void LuaScriptingProvider::executeScript(const std::string& scriptCode, IScriptingCallContext* callContext)
{
	executeScriptImpl(scriptCode, static_cast<LuaScriptingCallContext*>(callContext), "");
}

void LuaScriptingProvider::callFunction(const std::string& functionName, int narg, IScriptingCallContext* callContext)
{
	callFunctionImpl(functionName, narg, static_cast<LuaScriptingCallContext*>(callContext));
}

void LuaScriptingProvider::executeScriptImpl(const std::string& scriptCode, LuaScriptingCallContext* luaCallContext, const std::string& scriptName)
{
	try {
		int top = lua_gettop(mLuaState);
		int loaderr = luaL_loadbuffer(mLuaState, scriptCode.c_str(), scriptCode.length(), scriptCode.c_str());

		if (loaderr)
		{
			std::string errMsg(lua_tostring(mLuaState,-1));
			lua_settop(mLuaState,top);
			if (!scriptName.empty())
			{
				throw Exception("Unable to load Lua script file: '"+scriptName+"'\n\n"+errMsg+"\n");
			}
			else
			{
				throw Exception("Unable to load Lua script: '" + scriptCode + "'\n\n"+errMsg+"\n");
			}
		}

		//push our error handling method before calling the code
		int error_index = lua_gettop(mLuaState);
		lua_pushcfunction(mLuaState, LuaHelper::luaErrorHandler);
		lua_insert(mLuaState, error_index);

// 		lua_rawgeti(mLuaState, LUA_REGISTRYINDEX, mErrorHandlingFunctionIndex);
//         int error_index = lua_gettop(mLuaState);


		// load code into lua and call it
		int error;
//		int nresults;
		lua_gettop(mLuaState); // top of stack position
		// if we have context to store return values, then get them
		if (luaCallContext)
		{
			error = lua_pcall(mLuaState, 0, LUA_MULTRET, error_index);
//			nresults = lua_gettop(mLuaState) - level; // number of results
		}
		else
		{
			error = lua_pcall(mLuaState, 0, 0, error_index);
		}

		// handle errors
		if (error)
		{
			std::string errMsg(lua_tostring(mLuaState,-1));
			lua_settop(mLuaState,top);
			if (!scriptName.empty())
			{
				throw Exception("Unable to load Lua script file: '"+scriptName+"'\n\n"+errMsg+"\n");
			}
			else
			{
				throw Exception("Unable to load Lua script: '" + scriptCode + "'\n\n"+errMsg+"\n");
			}
		}

		if (luaCallContext)
		{
			fromStack fs(mLuaState);
			LuaRef* luaRef = new LuaRef(fs);
			luaCallContext->setReturnValue(luaRef);
		}

		lua_settop(mLuaState,top); // just in case :P - do we need it?

// 		getScriptModule().executeString(scriptCode);
	} catch (const std::exception& ex) {
		throw;
	} catch (...) {
		throw Exception("Unknown error when executing lua script.");
	}
}

void LuaScriptingProvider::callFunctionImpl(const std::string& functionName, int narg, LuaScriptingCallContext* luaCallContext)
{
	try {
																				// st: args
		int top = lua_gettop(mLuaState);
		LuaHelper::pushNamedFunction(mLuaState, functionName.c_str());		// st: args func

		//push our error handling method before calling the code
		int error_index = top + 1; //lua_gettop(mLuaState);
		#if LUA_VERSION_NUM >= 501
		lua_pushcfunction(mLuaState, LuaHelper::luaErrorHandler);   // st: args func err_h
		#else
		lua_pushliteral(mLuaState, "_TRACEBACK");
		lua_rawget(mLuaState, LUA_GLOBALSINDEX);  /* get traceback function */
		#endif
		lua_insert(mLuaState, error_index);										// st: args err_h func

		luaPop pop(mLuaState, 1); // pops error handler on exit

		// insert error handler and function before arguments
		lua_insert(mLuaState, top - narg + 1);									// st: func args err_h
		lua_insert(mLuaState, top - narg + 1);									// st: err_h func args
		error_index = top - narg + 1;
		// load code into lua and call it
		int error, nresults;
		int level = lua_gettop(mLuaState); // top of stack position
		// if we have context to store return values, then get them
		if (luaCallContext)
		{
			S_LOG_VERBOSE("Calling function with " << narg << " arguments");
			error = lua_pcall(mLuaState, narg, LUA_MULTRET, error_index);
			nresults = lua_gettop(mLuaState) - level; // number of results
			S_LOG_VERBOSE("Got " << nresults << " results");
		}
		else
		{
			error = lua_pcall(mLuaState, narg, 0, error_index);
		}

		// handle errors
		if (error)
		{
			std::string errMsg(lua_tostring(mLuaState,-1));
			//lua_settop(mLuaState,top);
			throw Exception("Unable to call Lua function '"+functionName+"'\n\n"+errMsg+"\n");
		}

		if (luaCallContext)
		{
			fromStack fs(mLuaState);
			LuaRef* luaRef = new LuaRef(fs);
			luaCallContext->setReturnValue(luaRef);
		}

		lua_settop(mLuaState,top); // just in case :P - do we need it?

// 		getScriptModule().executeString(scriptCode);
	} catch (const std::exception& ex) {
		throw;
	} catch (...) {
		throw Exception("Unknown error when calling lua.");
	}
}

bool LuaScriptingProvider::willLoadScript(const std::string& scriptName)
{
	size_t pos = scriptName.rfind(".lua");
	return (pos == scriptName.size() - 4);
}

const std::string& LuaScriptingProvider::getName() const
{
	static std::string name = "LuaScriptingProvider";
	return name;
}

void LuaScriptingProvider::_registerWithService(ScriptingService* service)
{
	mService = service;
}


void LuaScriptingProvider::forceGC()
{
#if LUA_VERSION_NUM >= 501
	lua_gc(mLuaState, LUA_GCCOLLECT, 0);
#else
	lua_setgcthreshold(mLuaState,0);
#endif
}

}

}
