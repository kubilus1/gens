/***************************************************************************
 * Gens: Main Loop. (Unix-specific code)                                   *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville                       *
 * Copyright (c) 2003-2004 by Stéphane Akhoun                              *
 * Copyright (c) 2008-2009 by David Korth                                  *
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

#include "g_main.hpp"
#include "g_main_unix.hpp"

// C includes.
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// C++ includes
#include <string>
#include <list>
using std::string;
using std::list;

#if !defined(GENS_DEBUG)
// Signal handler.
#include "sighandler.h"
#endif

// libgsft includes.
#include "libgsft/gsft_szprintf.h"

#include "md_palette.hpp"
#include "gens_ui.hpp"
#include "g_md.hpp"

// Command line parsing.
#include "parse.hpp"

#include "util/file/config_file.hpp"

// Video, Input.
#include "video/vdraw_sdl.h"
#include "video/vdraw_cpp.hpp"
#include "input/input_sdl.h"

#include "gens/gens_window.h"
#include "gens/gens_window_sync.hpp"

#include "port/timer.h"

// Message logging.
#include "macros/log_msg.h"


// On UNIX, the default save path is prefixed with the user's home directory.
#ifdef GENS_OS_MACOSX
	#define GENS_DEFAULT_SAVE_PATH "/Library/Application Support/Gens/"
#else
	#define GENS_DEFAULT_SAVE_PATH "/.gens/"
#endif


/**
 * get_default_save_path(): Get the default save path.
 * @param buf Buffer to store the default save path in.
 * @param size Size of the buffer.
 */
void get_default_save_path(char *buf, size_t size)
{
	// Prepend the user's home directory to the save path.
	szprintf(buf, size, "%s%s", getenv("HOME"), GENS_DEFAULT_SAVE_PATH);
	
	// Make sure the directory exists.
	// NOTE: 0777 is used to allow for custom umask settings.
	mkdir(buf, 0777);
}


/**
 * main(): Main loop.
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @return Error code.
 */
int main(int argc, char *argv[])
{
	if (geteuid() == 0)
	{
		// Don't run Gens/GS as root!
		static const char gensRootErr[] =
				"Error: " GENS_APPNAME " should not be run as root.\n"
				"Please log in as a regular user.";
		
		fprintf(stderr, "%s\n", gensRootErr);
		
		#ifdef GENS_UI_GTK
			// Check if X is running.
			char *display = getenv("DISPLAY");
			if (display)
			{
				if (gtk_init_check(NULL, NULL))
					GensUI::msgBox(gensRootErr, GENS_APPNAME " - Permissions Error", GensUI::MSGBOX_ICON_ERROR);
			}
		#endif /* GENS_UI_GTK */
		
		return 1;
	}
	
#if !defined(GENS_DEBUG)
	// Install the signal handler.
	gens_sighandler_init();
#endif
	
	// Initialize the timer.
	// TODO: Make this unnecessary.
	init_timer();
	
	// Initialize the PRNG.
	Init_PRNG();
	
	// Initialize the UI.
	Settings.showMenuBar = 1;
	GensUI::init(&argc, &argv);
	
	// Initialize VDraw.
	vdraw_init();
	
	// Initialize input_sdl.
	input_init(INPUT_BACKEND_SDL);
	
	// Initialize the Settings struct.
	if (Init_Settings())
	{
		// Error initializing settings.
		input_end();
		vdraw_end();
		return 1;	// TODO: Replace with a better error code.
	}
	
	// Parse command line arguments.
	Gens_StartupInfo_t *startup = parse_args(argc, argv);
	
	// Recalculate the palettes, in case a command line argument changed a video setting.
	Recalculate_Palettes();
	
	Init_Genesis_Bios();
	
	// Initialize Gens.
	if (!Init())
		return 0;
	
	// not yet finished (? - wryun)
	//initializeConsoleRomsView();
	
	// Create the menu bar.
	gens_window_create_menubar();
	
	// Update the UI.
	GensUI::update();
	
	// Reset the renderer.
	// This should be done before initializing the backend
	// to make sure vdraw_rInfo is initialized.
	vdraw_reset_renderer(true);
	
	// Initialize the video backend.
	if ((int)vdraw_cur_backend_id < 0)
	{
		// No backend saved in the configuration file. Use the default.
		vdraw_backend_init((VDRAW_BACKEND)0);
	}
	else
	{
		// Backend saved in the configuration file. Use that backend.
		vdraw_backend_init(vdraw_cur_backend_id);
	}
	
	// Check the startup mode.
	check_startup_mode(startup);
	free(startup);
	
	// Synchronize the Gens window.
	Sync_Gens_Window();
	
	// Show the Gens window.
	if (!vdraw_get_fullscreen())
		gtk_widget_show(gens_window);
	
	// Run the Gens Main Loop.
	GensMainLoop();
	
	// Save the configuration file.
#ifndef PACKAGE_NAME
#error PACKAGE_NAME not defined!
#endif
	string cfg_filename = string(PathNames.Gens_Save_Path) + PACKAGE_NAME + ".cfg";
	Config::save(cfg_filename);
	
	End_All();
	
#if !defined(GENS_DEBUG)
	// Shut down the signal handler.
	gens_sighandler_end();
#endif
	
	return 0;
}
