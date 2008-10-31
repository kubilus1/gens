/***************************************************************************
 * Gens: (Win32) Main Window.                                              *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville                       *
 * Copyright (c) 2003-2004 by Stéphane Akhoun                              *
 * Copyright (c) 2008 by David Korth                                       *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef GENS_WIN32_GENS_WINDOW_HPP
#define GENS_WIN32_GENS_WINDOW_HPP

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <stdint.h>

HWND initGens_hWnd(void);
HWND create_gens_window(void);
void create_gens_window_menubar(void);
extern HWND Gens_hWnd;

#if 0
extern HMENU MainMenu;
extern HMENU FileMenu;
extern HMENU FileMenu_ROMHistory;
extern HMENU FileMenu_ChangeState;
extern HMENU GraphicsMenu;
extern HMENU GraphicsMenu_Render;
extern HMENU GraphicsMenu_FrameSkip;
extern HMENU CPUMenu;
extern HMENU CPUMenu_Debug;
extern HMENU CPUMenu_Country;
extern HMENU SoundMenu;
extern HMENU SoundMenu_Rate;
extern HMENU OptionsMenu;
extern HMENU OptionsMenu_SegaCDSRAMSize;
extern HMENU HelpMenu;
#endif

// New menu handler.
HMENU findMenuItem(uint16_t id);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

// Unordered map containing all the menu items.
// Map key is the menu ID.
// TODO: unordered_map is gcc-4.x and later.
// For gcc-3.x, use __gnu_cxx::hash_map.

#include <tr1/unordered_map>
#include <utility>
#include <stdint.h>
typedef std::tr1::unordered_map<uint16_t, HMENU> win32MenuMap;
extern win32MenuMap gensMenuMap;
typedef std::pair<uint16_t, HMENU> win32MenuMapItem;

#endif /* __cplusplus */

#endif /* GENS_WIN32_GENS_WINDOW_HPP */
