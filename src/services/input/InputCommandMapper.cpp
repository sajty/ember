//
// C++ Implementation: InputCommandMapper
//
// Description:
//
//
// Author: Erik Hjortsberg <erik.hjortsberg@gmail.com>, (C) 2006
// Copyright (C) 2001 - 2005 Simon Goodall, University of Southampton
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

#include <algorithm>
#include "InputCommandMapper.h"
#include "framework/ConsoleBackend.h"
#include "services/config/ConfigService.h"
#include "framework/LoggingInstance.h"
#include <OISKeyboard.h>
namespace Ember {


InputCommandMapper::InputCommandMapper(const std::string& state) : mState(state), mEnabled(true), mInput(0)
{
	initKeyMap();
}

InputCommandMapper::InputCommandMapper(const std::string& state, const std::string& configSection) : mState(state), mEnabled(true), mInput(0)
{
	initKeyMap();
	readFromConfigSection(configSection);
}

InputCommandMapper::~InputCommandMapper()
{
	if (mInput) {
		mInput->deregisterCommandMapper(this);
	}
}

void InputCommandMapper::readFromConfigSection(const std::string& sectionName)
{

	//get the mappings from the config service
	ConfigService::SectionMap section = EmberServices::getSingleton().getConfigService().getSection(sectionName);

	for (ConfigService::SectionMap::const_iterator I = section.begin(); I != section.end(); ++I) {
		bindCommand(std::string(I->first), std::string(I->second));
	}

}

void InputCommandMapper::Input_EventKeyPressed(const OIS::KeyEvent& key, Input::InputMode inputMode)
{
	if (mEnabled && isActiveForInputMode(inputMode)) {
		//check if we have any key with a matching command
		KeyMapStore::const_iterator keyI = mKeymap.find(key.key);
		if (keyI != mKeymap.end()) {
			std::pair<KeyCommandStore::iterator, KeyCommandStore::iterator> commandsI = mKeyCommands.equal_range(keyI->second);
			for (KeyCommandStore::iterator I = commandsI.first; I != commandsI.second; ++I) {
				const std::string& command(I->second);
				if (command != "") {
					ConsoleBackend& myBackend = ConsoleBackend::getSingleton();
					myBackend.runCommand(command, false);
				}
			}
		}
	}
}


void InputCommandMapper::Input_EventKeyReleased(const OIS::KeyEvent& key, Input::InputMode inputMode)
{
	if (mEnabled && isActiveForInputMode(inputMode)) {
		//check if we have any key with a matching command
		//only check for commands that start with a "+"
		KeyMapStore::const_iterator keyI = mKeymap.find(key.key);
		if (keyI != mKeymap.end()) {
			std::pair<KeyCommandStore::iterator, KeyCommandStore::iterator> commandsI = mKeyCommands.equal_range(keyI->second);
			for (KeyCommandStore::iterator I = commandsI.first; I != commandsI.second; ++I) {
				const std::string& command(I->second);
				if (command != "") {
					if (command[0] == '+') {
						ConsoleBackend& myBackend = ConsoleBackend::getSingleton();
						//remove the "+" and replace it with "-"
						myBackend.runCommand("-" + std::string(command).erase(0, 1), false);
					}
				}
			}
		}
	}
}

void InputCommandMapper::bindToInput(Input& input)
{
	input.EventKeyPressed.connect(sigc::mem_fun(*this, &InputCommandMapper::Input_EventKeyPressed));
	input.EventKeyReleased.connect(sigc::mem_fun(*this, &InputCommandMapper::Input_EventKeyReleased));
	input.registerCommandMapper(this);
	mInput = &input;
}

bool InputCommandMapper::isActiveForInputMode(Input::InputMode mode) const
{
	//if there's no restriction, return true
	if (mInputModesRestriction.size() == 0) {
		return true;
	} else {
		return std::find(mInputModesRestriction.begin(), mInputModesRestriction.end(), mode) != mInputModesRestriction.end();
	}
}


void InputCommandMapper::bindCommand(const std::string& key, const std::string& command)
{
	S_LOG_INFO("Binding key " << key << " to command " << command << " for state " << getState() << ".");
	mKeyCommands.insert(KeyCommandStore::value_type(key, command));
}

void InputCommandMapper::unbindCommand(const std::string& key)
{
	S_LOG_INFO("Unbinding key " << key << " for state " << getState() << ".");
	mKeyCommands.erase(key);
}

void InputCommandMapper::unbindCommand(const std::string& key, const std::string& command)
{
	std::pair<KeyCommandStore::iterator, KeyCommandStore::iterator> commandsI = mKeyCommands.equal_range(key);
	for (KeyCommandStore::iterator I = commandsI.first; I != commandsI.second; ++I) {
		if (I->second == command) {
			mKeyCommands.erase(I);
		}
	}
}


//const std::string& InputCommandMapper::getCommandForKey(OIS::KeyCode key)
//{
//	KeyMapStore::const_iterator I = mKeymap.find(key);
//	if (I != mKeymap.end()) {
//		KeyCommandStore::const_iterator J = mKeyCommands.find(I->second);
//		if (J != mKeyCommands.end()) {
//			return J->second;
//		}
//	}
//	//if we don't find anything, return an empty string
//	static std::string empty("");
//	return empty;
//}


// std::string Bindings::getBindingForKeysym(const OIS::KeyEvent& key) {
//
//   std::map<int, std::string>::const_iterator I = m_keymap.find(key.sym);
//   if (I == m_keymap.end()) return ""; // un-mapped basic keysym
//   const std::string & plainName = I->second;
//   std::string decoratedName = plainName;
//
//   if (key.mod & KMOD_SHIFT)
//     decoratedName = "shift_" + decoratedName;
//
//   if (key.mod & KMOD_ALT)
//     decoratedName = "alt_" + decoratedName;
//
//   if (key.mod & KMOD_CTRL)
//     decoratedName = "ctrl_" + decoratedName;
//
//   m_bindings->clean(decoratedName);
//   if (m_bindings->findItem("key_bindings", decoratedName))
//     return m_bindings->getItem("key_bindings", decoratedName);
//
//   if (m_bindings->findItem("key_bindings", plainName))
//     return m_bindings->getItem("key_bindings", plainName);
//
//   std::cout << "no binding specified for key " << decoratedName << std::endl;
//   return "";
// }

void InputCommandMapper::initKeyMap() {
	// Assign keys to textual representation
	mKeymap[OIS::KC_BACK] = "backspace";
	mKeymap[OIS::KC_TAB] = "tab";
	mKeymap[OIS::KC_DELETE] = "clear";
	mKeymap[OIS::KC_RETURN] = "return";
	mKeymap[OIS::KC_PAUSE] = "pause";
	mKeymap[OIS::KC_ESCAPE] = "escape";
	mKeymap[OIS::KC_SPACE] = "space";
	//mKeymap[OIS::KC_EXCLAIM] = "exclaim";
	//mKeymap[OIS::KC_QUOTEDBL] = "dbl_quote";
	//mKeymap[OIS::KC_HASH] = "hash";
	//mKeymap[OIS::KC_DOLLAR] = "dollar";
	//mKeymap[OIS::KC_AMPERSAND] = "ampersand";
	//mKeymap[OIS::KC_QUOTE] = "quote";
	mKeymap[OIS::KC_LBRACKET] = "left_paren";
	//mKeymap[OIS::KC_RIGHTPAREN] = "right_paren";
	//mKeymap[OIS::KC_ASTERISK] = "asterisk";
	//mKeymap[OIS::KC_PLUS] = "plus";
	mKeymap[OIS::KC_COMMA] = "comma";
	//mKeymap[OIS::KC_MINUS] = "minus";
	mKeymap[OIS::KC_PERIOD] = "period";
	mKeymap[OIS::KC_SLASH] = "slash";
	mKeymap[OIS::KC_0] = "0";
	mKeymap[OIS::KC_1] = "1";
	mKeymap[OIS::KC_2] = "2";
	mKeymap[OIS::KC_3] = "3";
	mKeymap[OIS::KC_4] = "4";
	mKeymap[OIS::KC_5] = "5";
	mKeymap[OIS::KC_6] = "6";
	mKeymap[OIS::KC_7] = "7";
	mKeymap[OIS::KC_8] = "8";
	mKeymap[OIS::KC_9] = "9";
	mKeymap[OIS::KC_COLON] = "colon";
	mKeymap[OIS::KC_SEMICOLON] = "semi_colon";
	//mKeymap[OIS::KC_LESS] = "less_than";
	mKeymap[OIS::KC_EQUALS] = "equals";
	//mKeymap[OIS::KC_GREATER] = "greater_then";
	//mKeymap[OIS::KC_QUESTION] = "question";
	mKeymap[OIS::KC_AT] = "at";
	mKeymap[OIS::KC_LBRACKET] = "left_brace";
	mKeymap[OIS::KC_BACKSLASH] = "backslash";
	mKeymap[OIS::KC_RBRACKET] = "right_brace";
	//mKeymap[OIS::KC_CARET] = "caret";
	//mKeymap[OIS::KC_UNDERSCORE] = "_";
	//mKeymap[OIS::KC_BACKQUOTE] = "backquote";
	mKeymap[OIS::KC_A] = "a";
	mKeymap[OIS::KC_B] = "b";
	mKeymap[OIS::KC_C] = "c";
	mKeymap[OIS::KC_D] = "d";
	mKeymap[OIS::KC_E] = "e";
	mKeymap[OIS::KC_F] = "f";
	mKeymap[OIS::KC_G] = "g";
	mKeymap[OIS::KC_H] = "h";
	mKeymap[OIS::KC_I] = "i";
	mKeymap[OIS::KC_J] = "j";
	mKeymap[OIS::KC_K] = "k";
	mKeymap[OIS::KC_L] = "l";
	mKeymap[OIS::KC_M] = "m";
	mKeymap[OIS::KC_N] = "n";
	mKeymap[OIS::KC_O] = "o";
	mKeymap[OIS::KC_P] = "p";
	mKeymap[OIS::KC_Q] = "q";
	mKeymap[OIS::KC_R] = "r";
	mKeymap[OIS::KC_S] = "s";
	mKeymap[OIS::KC_T] = "t";
	mKeymap[OIS::KC_U] = "u";
	mKeymap[OIS::KC_V] = "v";
	mKeymap[OIS::KC_W] = "w";
	mKeymap[OIS::KC_X] = "x";
	mKeymap[OIS::KC_Y] = "y";
	mKeymap[OIS::KC_Z] = "z";
	mKeymap[OIS::KC_DELETE] = "delete";
	mKeymap[OIS::KC_NUMPAD0] = "kp_0";
	mKeymap[OIS::KC_NUMPAD1] = "kp_1";
	mKeymap[OIS::KC_NUMPAD2] = "kp_2";
	mKeymap[OIS::KC_NUMPAD3] = "kp_3";
	mKeymap[OIS::KC_NUMPAD4] = "kp_4";
	mKeymap[OIS::KC_NUMPAD5] = "kp_5";
	mKeymap[OIS::KC_NUMPAD6] = "kp_6";
	mKeymap[OIS::KC_NUMPAD7] = "kp_7";
	mKeymap[OIS::KC_NUMPAD8] = "kp_8";
	mKeymap[OIS::KC_NUMPAD9] = "kp_9";
	mKeymap[OIS::KC_NUMPADCOMMA] = "kp_period";
	mKeymap[OIS::KC_DIVIDE] = "kp_divide";
	mKeymap[OIS::KC_MULTIPLY] = "kp_multi";
	mKeymap[OIS::KC_MINUS] = "kp_minus";
	mKeymap[OIS::KC_ADD] = "kp_plus";
	mKeymap[OIS::KC_NUMPADENTER] = "kp_enter";
	mKeymap[OIS::KC_NUMPADEQUALS] = "kp_equals";
	mKeymap[OIS::KC_UP] = "up";
	mKeymap[OIS::KC_DOWN] = "down";
	mKeymap[OIS::KC_RIGHT] = "right";
	mKeymap[OIS::KC_LEFT] = "left";
	mKeymap[OIS::KC_INSERT] = "insert";
	mKeymap[OIS::KC_HOME] = "home";
	mKeymap[OIS::KC_END] = "end";
	mKeymap[OIS::KC_PGUP] = "page_up";
	mKeymap[OIS::KC_PGDOWN] = "page_down";
	mKeymap[OIS::KC_F1] = "f1";
	mKeymap[OIS::KC_F2] = "f2";
	mKeymap[OIS::KC_F3] = "f3";
	mKeymap[OIS::KC_F4] = "f4";
	mKeymap[OIS::KC_F5] = "f5";
	mKeymap[OIS::KC_F6] = "f6";
	mKeymap[OIS::KC_F7] = "f7";
	mKeymap[OIS::KC_F8] = "f8";
	mKeymap[OIS::KC_F9] = "f9";
	mKeymap[OIS::KC_F10] = "f10";
	mKeymap[OIS::KC_F11] = "f11";
	mKeymap[OIS::KC_F12] = "f12";
	mKeymap[OIS::KC_F13] = "f13";
	mKeymap[OIS::KC_F14] = "f14";
	mKeymap[OIS::KC_F15] = "f15";

	mKeymap[OIS::KC_NUMLOCK] = "num";
	mKeymap[OIS::KC_CAPITAL] = "caps";
	mKeymap[OIS::KC_SCROLL] = "srcoll";
	mKeymap[OIS::KC_RSHIFT] = "right_shift";
	mKeymap[OIS::KC_LSHIFT] = "left_shift";
	mKeymap[OIS::KC_RCONTROL] = "right_ctrl";
	mKeymap[OIS::KC_LCONTROL] = "left_ctrl";
	mKeymap[OIS::KC_RMENU] = "right_alt";
	mKeymap[OIS::KC_LMENU] = "left_alt";
	mKeymap[OIS::KC_RMENU] = "right_meta";
	mKeymap[OIS::KC_LMENU] = "left_meta";
	//mKeymap[OIS::KC_LSUPER] = "left_super";
	//mKeymap[OIS::KC_RSUPER] = "right_super";
	//mKeymap[OIS::KC_MODE]= "mode";
	//mKeymap[OIS::KC_COMPOSE] = "compose";
	//mKeymap[OIS::KC_PRINT] = "print";
	mKeymap[OIS::KC_SYSRQ] = "sysreq";
	//mKeymap[OIS::KC_BREAK] = "break";
	//mKeymap[OIS::KC_MENU] = "menu";
	mKeymap[OIS::KC_POWER] = "power";
	//mKeymap[OIS::KC_EURO] = "euro";
	mKeymap[OIS::KC_UNASSIGNED] = "unassigned";
}

}
