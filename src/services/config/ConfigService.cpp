/*
    Copyright (C) 2002  Miguel Guzman Miranda [Aglanor]
                        Joel Schander         [nullstar]
                        Erik Hjortsberg

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

#include "ConfigService.h"
#include "framework/LoggingInstance.h"
// #include "framework/binreloc.h"

#include <iostream>
#include <cstdlib>
#include "framework/ConsoleBackend.h"
#include "framework/Tokeniser.h"

#ifdef _WIN32
#include "platform/platform_windows.h"

//we need this for the PathRemoveFileSpec(...) method
#include <shlwapi.h>
#include <Shlobj.h>
#endif

// #include <iostream>
#include <fstream>

// From sear
#ifdef __APPLE__

#include <CoreFoundation/CFBundle.h>
#include <CoreServices/CoreServices.h>

using namespace Ember;

std::string getBundleResourceDirPath()
{
	/* the following code looks for the base package directly inside
	the application bundle. This can be changed fairly easily by
	fiddling with the code below. And yes, I know it's ugly and verbose.
	*/
	std::string result;
#ifdef BUILD_WEBEMBER

	const char* strBundleID = "com.WebEmberLib.WebEmber";
	CFStringRef bundleID = CFStringCreateWithCString(kCFAllocatorDefault,strBundleID,kCFStringEncodingMacRoman);
	CFBundleRef appBundle = CFBundleGetBundleWithIdentifier(bundleID);
	CFRelease(bundleID);
	if(!appBundle){
		S_LOG_FAILURE("Bundle with identifier " << strBundleID << " not found!");
	}else{
#else
		CFBundleRef appBundle = CFBundleGetMainBundle();
#endif
		CFURLRef resUrl = CFBundleCopyResourcesDirectoryURL(appBundle);
		CFURLRef absResUrl = CFURLCopyAbsoluteURL(resUrl);

		// now convert down to a path, and the a c-string
		CFStringRef path = CFURLCopyFileSystemPath(absResUrl, kCFURLPOSIXPathStyle);
		result = CFStringGetCStringPtr(path, CFStringGetSystemEncoding());

		FRelease(resUrl);
		CFRelease(absResUrl);
		CFRelease(path);
#ifdef BUILD_WEBEMBER
	}
#endif
	return result;
}

std::string getAppSupportDirPath()
{
	FSRef fs;
	OSErr err = FSFindFolder(kUserDomain, kApplicationSupportFolderType, true, &fs);
	if (err != noErr) {
		S_LOG_FAILURE("error doing FindFolder");
		return std::string();
	}

	CFURLRef dirURL = CFURLCreateFromFSRef(kCFAllocatorSystemDefault, &fs);
	char fsRepr[1024];
	if (!CFURLGetFileSystemRepresentation(dirURL, true, (UInt8*) fsRepr, 1024)) {
		std::cerr << "error invoking CFURLGetFileSystemRepresentation" << std::endl;
		return std::string();
	}

	CFRelease(dirURL);
	return fsRepr;
}

#endif

namespace Ember
{
	
	const std::string ConfigService::SETVALUE ( "set_value" );
	const std::string ConfigService::GETVALUE ( "get_value" );

	ConfigService::ConfigService() :
	Service()
	, mSharedDataDir( "" )
	, mEtcDir( "" )
	, mHomeDir ( "" )
	, mConfig(new varconf::Config())
	{
#ifdef __WIN32__
		char cwd[512];
		//get the full path for the current executable
		GetModuleFileName ( 0, cwd, 512 );

		//use this utility function for removing the file part
		PathRemoveFileSpec ( cwd );
		baseDir = std::string ( cwd ) + "\\";
		mSharedDataDir = baseDir + "\\..\\share\\ember\\";
		mEtcDir = baseDir + "\\..\\etc\\ember\\";
		
#endif

#if !defined(__APPLE__) && !defined(__WIN32__)
		mSharedDataDir = EMBER_DATADIR "/ember/";
		mEtcDir = EMBER_SYSCONFDIR "/ember/";
#endif

		setName ( "Configuration Service" );
		setDescription ( "Service for management of Ember user-defined configuration" );

		setStatusText ( "Configuration Service status OK." );
	}


	void ConfigService::setPrefix ( const std::string& prefix )
	{
		mPrefix = prefix;
		mSharedDataDir = prefix + "/share/ember/";
		mEtcDir = prefix + "/etc/ember/";
	}

	const std::string& ConfigService::getPrefix() const
	{
		return mPrefix;
	}


	void ConfigService::setHomeDirectory ( const std::string& path )
	{
		mHomeDir = path;
	}

	const ConfigService::SectionMap& ConfigService::getSection ( const std::string& sectionName )
	{
		return mConfig->getSection ( sectionName );
	}


	varconf::Variable ConfigService::getValue ( const std::string& section, const std::string& key ) const
	{
		return mConfig->getItem ( section, key );
	}

	bool ConfigService::getValue ( const std::string& section, const std::string& key, varconf::Variable& value ) const
	{
		if ( hasItem ( section, key ) )
		{
			value = getValue ( section, key );
			return true;
		}
		return false;
	}

	void ConfigService::setValue ( const std::string& section, const std::string& key, const varconf::Variable& value )
	{
		mConfig->setItem ( section, key, value );
	}

	bool ConfigService::isItemSet ( const std::string& section, const std::string& key, const std::string& value ) const
	{
		return ( hasItem ( section, key ) && getValue ( section, key ) == value );
	}

	Service::Status ConfigService::start()
	{
		mConfig->sige.connect ( sigc::mem_fun ( *this, &ConfigService::configError ) );
		mConfig->sigv.connect ( sigc::mem_fun ( *this, &ConfigService::updatedConfig ) );
		registerConsoleCommands();
		setRunning ( true );
		//can't do this since we must be the first thing initialized
		//LoggingService::getInstance()->slog(__FILE__, __LINE__, LoggingService::INFO) << getName() << " initialized" << ENDM;
		return Service::OK;
	}

	void ConfigService::stop ( int code )
	{
		Service::stop ( code );
		deregisterConsoleCommands();
		return;
	}

	void ConfigService::deregisterConsoleCommands()
	{
		ConsoleBackend::getSingletonPtr()->deregisterCommand ( SETVALUE );
		ConsoleBackend::getSingletonPtr()->deregisterCommand ( GETVALUE );
	}

	void ConfigService::registerConsoleCommands()
	{
		ConsoleBackend::getSingletonPtr()->registerCommand ( SETVALUE, this, "Sets a value in the configuration. Usage: set_value <section> <key> <value>" );
		ConsoleBackend::getSingletonPtr()->registerCommand ( GETVALUE, this, "Gets a value from the configuration. Usage: get_value <section> <key>" );
	}

	bool ConfigService::itemExists(const std::string& section, const std::string& key) const
	{
		return hasItem(section, key);
	}

	bool ConfigService::hasItem ( const std::string& section, const std::string& key ) const
	{
		return mConfig->find ( section, key );
	}

	bool ConfigService::deleteItem ( const std::string& section, const std::string& key )
	{
		return mConfig->erase ( section, key );
	}

	bool ConfigService::loadSavedConfig ( const std::string& filename )
	{
		S_LOG_INFO ( "Loading shared config file from " << getSharedConfigDirectory() + "/"+ filename << "." );
		bool success = mConfig->readFromFile ( getSharedConfigDirectory() + "/"+ filename, varconf::GLOBAL );
		std::string userConfigPath ( getHomeDirectory() + "/" + filename );
		std::ifstream file ( userConfigPath.c_str() );
		if ( !file.fail() )
		{
			S_LOG_INFO ( "Loading user config file from "<< getHomeDirectory() + "/" + filename <<"." );
			try
			{
				mConfig->parseStream ( file, varconf::USER );
			}
			catch ( varconf::ParseError p )
			{
				std::string p_str ( p );
				S_LOG_FAILURE ( "Error loading user config file: " << p_str );
				return false;
			}
		}
		else
		{
			S_LOG_INFO ( "Could not find any user config file." );
		}

		return success;
	}

	bool ConfigService::saveConfig ( const std::string& filename )
	{
		return mConfig->writeToFile ( filename );
	}

	void ConfigService::runCommand ( const std::string &command, const std::string &args )
	{
		if ( command == SETVALUE )
		{
			Tokeniser tokeniser;
			tokeniser.initTokens ( args );
			std::string section ( tokeniser.nextToken() );
			std::string key ( tokeniser.nextToken() );
			std::string value ( tokeniser.remainingTokens() );

			if ( section == "" || key == "" || value == "" )
			{
				ConsoleBackend::getSingletonPtr()->pushMessage ( "Usage: set_value <section> <key> <value>" );
			}
			else
			{
				setValue ( section, key, value );
				ConsoleBackend::getSingletonPtr()->pushMessage ( "New value set, section: " +  section + " key: " + key + " value: " + value );
			}

		}
		else if ( command == GETVALUE )
		{
			Tokeniser tokeniser;
			tokeniser.initTokens ( args );
			std::string section ( tokeniser.nextToken() );
			std::string key ( tokeniser.nextToken() );

			if ( section == "" || key == "" )
			{
				ConsoleBackend::getSingletonPtr()->pushMessage ( "Usage: get_value <section> <key>" );
			}
			else
			{
				if ( !hasItem ( section, key ) )
				{
					ConsoleBackend::getSingletonPtr()->pushMessage ( "No such value." );
				}
				else
				{
					varconf::Variable value = getValue ( section, key );
					ConsoleBackend::getSingletonPtr()->pushMessage ( std::string ( "Value: " ) + static_cast<std::string> ( value ) );
				}
			}
		}

	}

	void ConfigService::updatedConfig (const std::string& section, const std::string& key)
	{
		EventChangedConfigItem.emit ( section, key );
	}

	void ConfigService::configError ( const char* error )
	{
		S_LOG_FAILURE ( std::string ( error ) );
	}

	const std::string& ConfigService::getHomeDirectory() const
	{
		//check if the home directory is set, and if so use the setting. If else, fall back to the default path.
		if ( mHomeDir != "" )
		{
			return mHomeDir;
		}
		else
		{
#ifdef __WIN32__
			static std::string finalPath;
			if ( !finalPath.empty() )
			{
				return finalPath;
			}

			//special folders in windows:
			//http://msdn.microsoft.com/en-us/library/bb762494%28v=vs.85%29.aspx
			//CSIDL_MYDOCUMENTS would probably be more appropriate, but that's not available in msys (as of 2011-05-23).
			char path[MAX_PATH];
			if(SHGetSpecialFolderPath(NULL, path, CSIDL_PERSONAL, TRUE) == TRUE){
				finalPath = path;
			}else{
				finalPath = ".";
			}
			finalPath += "\\Ember\\";
			return finalPath;
#elif __APPLE__
			static std::string path ( getAppSupportDirPath() + "/Ember/" );
			return path;
#else
			static std::string path ( std::string ( getenv ( "HOME" ) ) + "/.ember/" );
			return path;
#endif
		}
	}

	const std::string& ConfigService::getSharedDataDirectory() const
	{
		if ( hasItem ( "paths", "sharedir" ) )
		{
			static std::string path ( static_cast<std::string> ( getValue ( "paths", "sharedir" ) ) + "/" );
			return path;
		}
		else
		{
#ifdef __APPLE__
			static std::string path ( getBundleResourceDirPath() );
			return path;
#else
			return mSharedDataDir;
#endif
		}

	}

	const std::string& ConfigService::getSharedConfigDirectory() const
	{
#ifdef __APPLE__
		static std::string path ( getSharedDataDirectory() + "/etc/ember/" );
		return path;
#else
		return mEtcDir;
#endif
	}

	const std::string& ConfigService::getEmberDataDirectory() const
	{
		if ( hasItem ( "paths", "datadir" ) )
		{
			static std::string path ( static_cast<std::string> ( getValue ( "paths", "datadir" ) ) + "/" );
			return path;
		}
		else
		{
//#ifdef __APPLE__
//			return getBundleResourceDirPath();
//#elif __WIN32__
//			return baseDir;
//#else
//			return BR_EMBER_DATADIR("/games/ember/");
//#endif
			return getHomeDirectory();
		}

	}

	const std::string& ConfigService::getEmberMediaDirectory() const
	{
		static std::string path;
		//look for a media channel key in the config, and if found use that, else use the version of ember as a standard path
		if ( hasItem ( "wfut", "channel" ) )
		{
			path = getEmberDataDirectory() + "/" + static_cast<std::string> ( getValue ( "wfut", "channel" ) ) + "/";
		}
		else
		{
			path = getEmberDataDirectory() + "/ember-media-" + std::string ( VERSION ) + "/";
		}
		return path;
	}


	const std::string& ConfigService::getUserMediaDirectory() const
	{
		static std::string path ( getHomeDirectory() + "/user-media/" );
		return path;
	}

	const std::string& ConfigService::getSharedMediaDirectory() const
	{
		static std::string path ( getSharedDataDirectory() + "/media/shared/" );
		return path;
	}

} // namespace Ember
