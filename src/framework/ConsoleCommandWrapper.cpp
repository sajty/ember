//
// C++ Implementation: ConsoleCommandWrapper
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ConsoleCommandWrapper.h"
#include "ConsoleBackend.h"
#include "framework/LoggingInstance.h"

namespace Ember {


ConsoleCommandWrapper::ConsoleCommandWrapper(std::string command, ConsoleObject *object, std::string description, bool suppressLogging)
: mCommand(command), mInverseCommand(""), mDescription(description), mObject(object), mSuppressLogging(suppressLogging)
{
	if (mCommand.size() > 0 && mCommand[0] == '+') {
		mInverseCommand = std::string("-") + std::string(mCommand).erase(0, 1);
	}
	ConsoleBackend& consoleBackend = ConsoleBackend::getSingleton();
	consoleBackend.registerCommand(mCommand, object, mDescription, suppressLogging);
	if (mInverseCommand != "") {
		consoleBackend.registerCommand(mInverseCommand, object, std::string("Releases the command ") + mCommand, suppressLogging);
	}
}

ConsoleCommandWrapper::~ConsoleCommandWrapper()
{
	ConsoleBackend& consoleBackend = ConsoleBackend::getSingleton();
	consoleBackend.deregisterCommand(mCommand, mSuppressLogging);
	if (mInverseCommand != "") {
		consoleBackend.deregisterCommand(mInverseCommand, mSuppressLogging);
	}
}


}
