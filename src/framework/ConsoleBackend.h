/*
 Copyright (C) 2002  Martin Pollard, Simon
 Copyright (C) 2005 Erik Hjortsberg <erik.hjortsberg@gmail.com>

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

#ifndef CONSOLEBACKEND_H
#define CONSOLEBACKEND_H

// Included headers from the current project
#include "ConsoleObject.h"

// Included custom library headers

#include <sigc++/signal.h>

// Included system headers
#include <list>
#include <unordered_map>
#include <map>
#include <set>
#include <string>

#include "framework/Singleton.h"

namespace Ember {

class CommandHistory;

/**
 * @brief Backend for the console. This act very much like a message hub, receiving incoming console commands and then dispatching these to the registered listener.
 *
 * The backend keeps track of all console commands, and all the listeners. Whenever a console command is executed, the listeners are notified in turn, allowing them to act on the console command.
 * It also allows for some nifty command completion functionality by creating internal lookup maps of all console commands.
 *
 * Any other component which wishes to interact with the console should preferrably do this through the ConsoleCommandWrapper class. While it's possible to instead use the ConsoleObject class directly, the ConsoleCommandWrapper provides an easier way.
 *
 * @see Ember::ConsoleCommandWrapper
 * @see Ember::ConsoleObject
 *
 */

class ConsoleBackend : public ConsoleObject, public Singleton<ConsoleBackend>
{
    public:

    //----------------------------------------------------------------------
    // Constructors

    /**
     * Creates a new ConsoleBackend using default values.
     */
	ConsoleBackend(void);

    /**
     * Copy constructor.
     */
    ConsoleBackend( const ConsoleBackend &source );


    /**
     * Assignment operator.
     */
    ConsoleBackend &operator= ( const ConsoleBackend &source );


    //----------------------------------------------------------------------
    // Destructor

    /**
     * Deletes a ConsoleBackend instance.
     */
    virtual ~ConsoleBackend ();


    //----------------------------------------------------------------------
    // Getters

    /**
     * @brief Gets a list of previous console messages.
     * @return A list of the previous console messages, the maximum defined through MAX_MESSAGES.
     */
    const std::list<std::string>& getConsoleMessages() const;

	const std::set< std::string > & getPrefixes(const std::string & prefix) const;

	/**
	 * @brief Gets the command history.
	 * @return The command history.
	 */
	CommandHistory& getHistory();

    //----------------------------------------------------------------------
    // Other public methods

    /**
     * @brief Registers a command with the console
     * @param command The command to register.
     * @param object The object which will receive updated when the console command is activated.
     * @param description A description of the command and what it does.
     */
    void registerCommand(const std::string &command, ConsoleObject *object, const std::string& description = "", bool suppressLogging = false);

    /**
     * @brief Deregisters a command with the console
     * @param command The command to deregister.
     */
    void deregisterCommand(const std::string &command, bool suppressLogging = false);

    /**
     * @brief The method the determines what object the pass the command onto
     * @param command The command string to process.
     * @param addToHistory Optional. Whether to add the command to the history or not. Most commands will be added to the history, but others, such as the movement command issued when the user pressed movement keys, shouldn't be recorded in the history.
     */
    void runCommand(const std::string &command, bool addToHistory = true);

    /**
     * @brief Add a message to the console message queue.
     * @param message The message.
	 * @param tag "Category" of the message, for example "warning", "error", "info", "help", "important", etc...
     */
    void pushMessage(const std::string &message, const std::string& tag = "");


    /**
     * @brief Reacts to listened to commands.
     * @param command The command.
     * @param args The parameters for the command.
     */
    void runCommand(const std::string &command, const std::string &args);


    //------------------------------------
    // Events


    /**
     * This event is raised every time a message is pushed to the console.
     * If true is returned, the message won't be saved in myConsoleMessages
     */
    sigc::signal<bool, const std::string&, const std::string&> GotMessage;

    //======================================================================
    // Protected Methods
    //======================================================================
    protected:

    virtual bool onGotMessage(const std::string &message, const std::string& tag);

    private:

	struct ConsoleObjectEntry
	{
		ConsoleObject* Object;
		std::string Description;
	};

	typedef std::unordered_map<std::string, ConsoleObjectEntry> ConsoleObjectEntryStore;

    // List of ConsoleBackend's console commands
    static const char * const LIST_CONSOLE_COMMANDS;

	static const unsigned int MAX_MESSAGES;


    /**
     * Mapping of registered commands to associated object.
     */
    ConsoleObjectEntryStore mRegisteredCommands;

    /**
     * Current console messages
     */
    std::list< std::string > mConsoleMessages;


    /**
     * @brief Prefix map for commands.
     * This is used for fast lookup for the autocompletion.
     * An example:
     * If the command "set_foo" is registered, these entries will be added to the mPrefixes:
     * s => set_foo
     * se => set_foo
     * set => set_foo
     * set_ => set_foo
     * set_f => set_foo
     * set_fo => set_foo
     * set_foo => set_foo (will this really be added?)
     **/
    std::map< std::string, std::set< std::string >> mPrefixes;

    /**
     * @brief The command history, allowing simple browsing of the command history.
     */
    CommandHistory* mCommandHistory;

};

inline const std::list<std::string>& ConsoleBackend::getConsoleMessages() const
{
	return mConsoleMessages;
}

}

#endif
