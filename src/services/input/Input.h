//
// C++ Interface: Input
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
#ifndef EMBERINPUT_H
#define EMBERINPUT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "framework/ConsoleObject.h"
#include "framework/Singleton.h"

#include <sigc++/slot.h>
#include <sigc++/signal.h>

#include <SDL_keysym.h>

#include <set>
#include <list>
#include <unordered_map>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
typedef unsigned int uint32_t;
#endif

struct SDL_KeyboardEvent;
struct SDL_keysym;
struct SDL_Surface;

namespace varconf
{
class Variable;
}

namespace Ember
{

class IInputAdapter;
class IWindowProvider;
class InputCommandMapper;
class ConfigListenerContainer;
class MainLoopController;

typedef std::set<SDLKey> KeysSet;
typedef std::list<IInputAdapter*> IInputAdapterStore;

/**
 * @brief Struct for a mouse movement.
 * @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 */
struct MouseMotion
{
	/**
	 * @brief The horizontal position of the mouse in pixels.
	 */
	int xPosition;

	/**
	 * @brief The vertical position of the mouse in pixels.
	 */
	int yPosition;

	/**
	 * @brief The relative horizontal position of the mouse.
	 * This is between 0..1
	 */
	float xRelativeMovement;

	/**
	 * @brief The relative vertical position of the mouse.
	 * This is between 0..1
	 */
	float yRelativeMovement;

	/**
	 * @brief The relative horizontal movement in pixels.
	 * The amount of pixels the cursor has moved since our last sampling point.
	 */
	int xRelativeMovementInPixels;

	/**
	 * @brief The relative vertical movement in pixels.
	 * The amount of pixels the cursor has moved since our last sampling point.
	 */
	int yRelativeMovementInPixels;

	/**
	 @brief The time since last sampling point.
	 Expressed as full seconds.
	 */
	float timeSinceLastMovement;

};

/**
 * @brief Expresses the position of the mouse, both in terms of pixels and relative.
 * 
 * Positions are from the upper left corner.
 * @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 */
struct MousePosition
{
	/**
	 * @brief The horizontal position of the mouse in pixels.
	 */
	int xPixelPosition;
	/**
	 * @brief The vertical position of the mouse in pixels.
	 */
	int yPixelPosition;

	/**
	 * @brief The relative horizontal position of the mouse.
	 * 0 is to the left, 1 is to the right.
	 */
	float xRelativePosition;
	/**
	 * @brief The relative vertical position of the mouse.
	 * 0 is at the top, 1 is at the bottom.
	 */
	float yRelativePosition;

};

/**
 * @author Erik Hjortsberg <erik.hjortsberg@gmail.com>
 * 
 * @brief This class takes care of all input and routes it to the correct place in Ember.
 * Right now that means that when in GUI mode, all input will be routed to the registered list of @see IInputAdapter, and when in non-gui mode (ie. movement mode), all input will be routed directly to Ember, where it can be handled by the camera and movement system.
 * 
 * Note that while keyboard input is buffered, mouse input is not.
 * 
 * You can listen to input updates either by listening directly to the events, or by registering an instance of IInputAdapter through the addAdapter and removeAdapter methods.
 * 
 * This class also provides some methods useful for standard windowing and event system integration, such as isApplicationVisible().
 */
class Input: public ConsoleObject, public Singleton<Input>
{
	friend class InputCommandMapper;

public:

	/**
	 * @brief Command for binding keys to commands.
	 */
	static const std::string BINDCOMMAND;

	/**
	 * @brief Command for unbinding keys to commands.
	 */
	static const std::string UNBINDCOMMAND;

	enum MouseButton
	{
		MouseButtonLeft, MouseButtonRight, MouseButtonMiddle, MouseWheelUp, MouseWheelDown,
	};

	/**
	 * @brief Describes different input modes.
	 */
	enum InputMode
	{
		/**
		 * @brief In gui mode, the mouse will move the cursor and allow interaction with the GUI system
		 */
		IM_GUI,

		/**
		 * @brief In movement mode, the mouse will move the camera and the keys will move the player. Interaction with the gui is not possible.
		 */
		IM_MOVEMENT
	};

	Input();

	virtual ~Input();

	/**
	 * @brief Initializes the input object. Call this before you want to receive input.
	 * @param window The target window to attach the input system to.
	 */
	void attach(IWindowProvider* windowProvider);

	/**
	 * @brief This will detach the input system from the window.
	 *
	 * It's safe to call it multiple times. Second call will be ignored.
	 */
	void detach();

	/**
	 * @brief Call this when application setup has completed and the user should start interacting with the application.
	 *
	 * This will catch the mouse if that's enabled, amongst other things.
	 */
	void startInteraction();

	/**
	 * @brief Starts processing all input for a frame.
	 * Call this once every frame.
	 * @param evt 
	 */
	void processInput();

	/**
	 * @brief Checks whether the application is visible and nor minimized.
	 * @return True if the application is in normal shown mode, false if it's minimized.
	 */
	bool isApplicationVisible();

	/**
	 * @brief Checks whether the application is currently in focus.
	 * @return True if the application is currently in focus.
	 */
	bool isApplicationFocused();

	/**
	 * @brief Sets the main loop controller.
	 *
	 * @param mainLoopController The main loop controller.
	 */
	void setMainLoopController(MainLoopController* mainLoopController);

	/**
	 * @brief Gets the main loop controller.
	 *
	 * @return The main loop controller.
	 */
	MainLoopController* getMainLoopController() const;

	/** @brief Emitted when a key has been pressed in movement mode.
	 * @param the key event
	 * @param true if the application is in gui mode
	 */
	sigc::signal<void, const SDL_keysym&, Input::InputMode> EventKeyPressed;

	/** @brief Emitted when a key has been released in movement mode.
	 * @param the key event
	 * @param true if the application is in gui mode
	 */
	sigc::signal<void, const SDL_keysym&, Input::InputMode> EventKeyReleased;

	/** @brief Emitted when the mouse has moved.
	 * Note that when in non-gui mode, the x and y position for the mouse will always be the same for consecutive signals although the relative position will have changed.
	 * @param the mouse motion
	 * @param true if ember is in gui mode
	 */
	sigc::signal<void, const MouseMotion&, InputMode> EventMouseMoved;

	/**
	 * @brief Emitted when a mouse button is pressed.
	 * @param the mouse button
	 * @param true if ember is in gui mode
	 */
	sigc::signal<void, MouseButton, InputMode> EventMouseButtonPressed;

	/**
	 * @brief Emitted when a mouse button is released.
	 * @param the mouse button
	 * @param true if ember is in gui mode
	 */
	sigc::signal<void, MouseButton, InputMode> EventMouseButtonReleased;

	/**
	 * @brief Emitted when the input mode has been changed.
	 * @param the new input mode
	 */
	sigc::signal<void, InputMode> EventChangedInputMode;

	/**
	 * @brief Emitted when the user has changed the window focus, which usually means Alt-Tab switching.
	 * If the application is in mouse grab mode we should probably release it.
	 */
	sigc::signal<void> EventWindowFocusChange;

	/**
	 @brief Emitted when the window is minimized or un-mininized.
	 @param True if the window is active, false it it's minimized.
	 */
	sigc::signal<void, bool> EventWindowActive;

	/**
	 * @brief Emitted when the size of the main window has changed.
	 */
	sigc::signal<void, int, int> EventSizeChanged;

	/**
	 * @brief Returns true if the supplied key is down.
	 * @param  key The key to check for.
	 * @return True if the key is down.
	 */
	const bool isKeyDown(const SDLKey& key) const;

	/**
	 * @brief Sets the window geometry. Call this whenever the size of the window has changed.
	 * @param width The new width, in pixels.
	 * @param heigh The new height, in pixels.
	 */
	void setGeometry(int width, int heigh);

	/**
	 * @brief Sets the new input mode.
	 * @param mode The new mode.
	 */
	void setInputMode(InputMode mode);

	/**
	 * @brief Gets the current input mode.
	 * @return The current input mode. 
	 */
	InputMode getInputMode() const;

	/**
	 * @brief Toggles between different input modes, returning the new mode.
	 * @return The new input mode.
	 */
	InputMode toggleInputMode();

	/**
	 * @brief Adds an adaptor to which input event will be sent.
	 *  Note that event will be sent to adapters added later first, allowing them to decide whether events should be sent to previous added adapters. This allows later added adapters to override current behaviour.
	 * @see removeAdapter
	 * @param adaptor An adapter instance to add. Note that ownership isn't transferred. Note that if the adapter already has been added, this will add a duplicate entry.
	 */
	void addAdapter(IInputAdapter* adapter);

	/**
	 * @brief Remove an adaptor from the list of adaptors.
	 * @param adaptor The adapter to remove. If the adapter isn't present already nothing will happen.
	 */
	void removeAdapter(IInputAdapter* adapter);

	/**
	 * @brief Reimplements the ConsoleObject::runCommand method
	 * @param command 
	 * @param args 
	 */
	virtual void runCommand(const std::string &command, const std::string &args);

	/**
	 * @brief Suppress all further event handling of the current event. Call this inside event handling methods to prevent further event handling.
	 */
	void suppressFurtherHandlingOfCurrentEvent();

	/**
	 * @brief Gets whether the movement mode is enabled, at which all mouse right click events will toggle between movement mode and mouse mode.
	 * @return True if movement mode is enabled.
	 */
	bool getMovementModeEnabled() const;

	/**
	 * @brief Sets whether the movement mode is enabled, at which all mouse right click events will toggle between movement mode and mouse mode.
	 * @param value Whether to enable movement mode or not.
	 */
	void setMovementModeEnabled(bool value);

	/**
	 * @brief Writes the supplied text to the system clipboard.
	 * This works on all platform, by using the scrap.cpp code found in framework.
	 * @param text The text to write.
	 */
	void writeToClipboard(const std::string& text);

	/**
	 * @brief Gets the current mouse position.
	 * @return The current mouse position.
	 */
	const MousePosition& getMousePosition() const;

	/**
	 * @brief Lets the application sleep a little.
	 * @param milliseconds How long to sleep, in milliseconds.
	 */
	void sleep(unsigned int milliseconds);

	/**
	 * @brief Registers a command mapper.
	 * @param mapper The mapper instance to register.
	 */
	void registerCommandMapper(InputCommandMapper* mapper);

	/**
	 * @brief Deregisters a command mapper.
	 * @param mapper The mapper to deregister.
	 */
	void deregisterCommandMapper(InputCommandMapper* mapper);

	/**
	 * @brief Enabled or disables mouse grabbing.
	 * @param enabled True if the mouse should be grabbed.
	 */
	void setMouseGrab(bool enabled);

private:

	typedef std::unordered_map<std::string, InputCommandMapper*> InputCommandMapperStore;

	/**
	 * @brief Polls all input for the mouse.
	 * Call this each frame.
	 * @param secondsSinceLast In whole seconds, the time since the last polling.
	 */
	void pollMouse(float secondsSinceLast);

	/**
	 * @brief Polls all needed events from the system.
	 * Call this each frame.
	 * @param secondsSinceLast In whole seconds, the time since the last polling.
	 */
	void pollEvents(float secondsSinceLast);

	void keyChanged(const SDL_KeyboardEvent &keyEvent);

	void keyPressed(const SDL_KeyboardEvent &keyEvent);
	void keyReleased(const SDL_KeyboardEvent &keyEvent);

	/**
	 * @brief Bind the ability for Ember to catch the mouse to the input:catchmouse key.
	 * @param section
	 * @param key
	 * @param variable
	 */
	void Config_CatchMouse(const std::string& section, const std::string& key, varconf::Variable& variable);

	/**
	 * @brief Allows to invert relative mouse position.
	 * @param section
	 * @param key
	 * @param variable
	 */
	void Config_InvertCamera(const std::string& section, const std::string& key, varconf::Variable& variable);

	/**
	 * @brief Gets the text in the clipboard and pastes it to the gui system.
	 */
	void pasteFromClipboard();

	/**
	 * @brief The current input mode.
	 */
	InputMode mCurrentInputMode;

	/**
	 @brief Keys which should not be injected as chars, ie. enter, backspace etc.
	 */
	KeysSet mNonCharKeys;

	/**
	 @brief A set of the keys that are currently pressed.
	 */
	KeysSet mKeysPressed;

	/**
	 @brief Saves the last mouse state.
	 */
	unsigned int mMouseState;

	/**
	 @brief The amount of time since the last right mouse click.
	 Used for detecting double clicks.
	 */
	float mTimeSinceLastRightMouseClick;

	/**
	 @brief Store the last tick count, to use for looking up how much time has elapsed since our last event polling.
	 */
	uint32_t mLastTick;

	/**
	 * @brief The last positions of the mouse.
	 */
	MousePosition mMousePosition;

	/**
	 @brief A store of adapters to which input event will be sent, in order.
	 */
	IInputAdapterStore mAdapters;

	/**
	 * @brief A store of InputCommandMappers with their state as the key.
	 */
	InputCommandMapperStore mInputCommandMappers;

	/**
	 * @brief Whether no more event should be processed.
	 * This is checked within the event handling code only.
	 */
	bool mSuppressForCurrentEvent;

	/**
	 * @brief Whether movement mode is enabled or not.
	 */
	bool mMovementModeEnabled;

	/**
	 * @brief Keeps track of configuration changes.
	 */
	ConfigListenerContainer* mConfigListenerContainer;

	/**
	 * @brief True if mouse grabbing has been requested.
	 *
	 * If this is true, the mouse will be grabbed next time the application has mouse focus.
	 * The main reason for not grabbing the mouse directly is that the application window might not have focus, and grabbing it then will only confuse the user.
	 */
	bool mMouseGrabbingRequested;

	/**
	 * @brief Whether the mouse is grabbed.
	 */
	bool mMouseGrab;

	/**
	 * @brief The main loop controller.
	 *
	 * This mainly allows us to request that the application is shut down.
	 */
	MainLoopController* mMainLoopController;

	/**
	 * @brief The Window Provider, which provides the communication interface between the window and the input system.
	 */
	IWindowProvider* mWindowProvider;

	/**
	 @brief The dimensions of the window.
	 */
	float mScreenWidth, mScreenHeight;

	/**
	 The icon shown in the top of the window.
	 */
	SDL_Surface* mIconSurface;

	/**
	 * @brief Whether to invert relative movement.
	 * 
	 * It can be 1(normal) or -1(inverted).
	 */
	int mInvertMouse;

};

}

#endif
