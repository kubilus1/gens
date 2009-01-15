/***************************************************************************
 * Gens: Input Handler - Base Code.                                        *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef GENS_INPUT_H
#define GENS_INPUT_H

// OS-specific includes.
#if (defined(GENS_OS_UNIX))
#include "input_sdl_keys.h"
#elif (defined(GENS_OS_WIN32))
#include "input_win32_keys.h"
#else
#error Unsupported operating system.
#endif

// Needed for HWND in input_set_cooperative_level.
#ifdef GENS_OS_WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "macros/bool_m.h"

// Controller key mapping.
typedef struct
{
	unsigned int Start, Mode;
	unsigned int A, B, C;
	unsigned int X, Y, Z;
	unsigned int Up, Down, Left, Right;
} input_keymap_t;

// Input backends.
typedef enum
{
	#ifdef GENS_OS_WIN32
		INPUT_BACKEND_DINPUT,
	#else /* !GENS_OS_WIN32 */
		INPUT_BACKEND_SDL,
	#endif /* GENS_OS_WIN32 */
	INPUT_BACKEND_MAX
} INPUT_BACKEND;

// Input backend function pointers.
typedef struct
{
	int	(*init)(void);
	int	(*end)(void);
	
	// Default keymap. (Must have 8 elements.)
	const input_keymap_t *keymap_default;
	
	// Update the input subsystem.
	int	(*update)(void);
	
	// Check if the specified key is pressed.
	BOOL	(*check_key_pressed)(unsigned int key);
	
	// Get a key. (Used for controller configuration.)
	unsigned int	(*get_key)(void);
	
	// Check if a joystick exists.
	BOOL	(*joy_exists)(int joy_num);
	
#ifdef GENS_OS_WIN32
	// Win32-specific functions.
	int	(*set_cooperative_level)(HWND hWnd);
#endif /* GENS_OS_WIN32 */
} input_backend_t;

int	input_init(INPUT_BACKEND backend);
int	input_end(void);

// Current backend.
extern const input_backend_t *input_cur_backend;
extern INPUT_BACKEND input_cur_backend_id;

// Update the controller bitfields.
void	input_update_controllers(void);

// Function and array pointers.
extern int			(*input_update)(void);
extern unsigned int		(*input_get_key)(void);
extern const input_keymap_t	*input_keymap_default;
#ifdef GENS_OS_WIN32
extern int			(*input_set_cooperative_level)(HWND hWnd);
#endif /* GENS_OS_WIN32 */

// Current keymap.
extern input_keymap_t	input_keymap[8];

#ifdef __cplusplus
}
#endif

#endif