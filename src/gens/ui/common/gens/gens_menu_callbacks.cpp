/***************************************************************************
 * Gens: Main Menu callbacks.                                              *
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

#include "gens_menu_callbacks.hpp"
#include "gens_menu.h"

#include "emulator/g_main.hpp"
#include "gens/gens_window.hpp"
#include "gens/gens_window_sync.hpp"
#include "game_genie/game_genie_window_misc.h"
#include "controller_config/controller_config_window_misc.hpp"
#include "bios_misc_files/bios_misc_files_window_misc.hpp"
#include "directory_config/directory_config_window_misc.hpp"
#include "general_options/general_options_window_misc.hpp"
#include "about/about_window.hpp"
#include "color_adjust/color_adjust_window_misc.h"
#include "country_code/country_code_window_misc.h"

#ifdef GENS_OPENGL
#include "opengl_resolution/opengl_resolution_window_misc.h"
#endif /* GENS_OPENGL */

#ifdef GENS_CDROM
#include "select_cdrom/select_cdrom_window_misc.hpp"
#endif /* GENS_CDROM */

#include "emulator/ui_proxy.hpp"
#include "util/file/config_file.hpp"

#include "util/sound/gym.hpp"
#include "util/file/rom.hpp"
#include "gens_core/vdp/vdp_io.h"
#include "util/file/save.hpp"
#include "gens_core/cpu/z80/z80.h"
#include "util/gfx/imageutil.hpp"

// Sega CD
#include "emulator/g_mcd.hpp"

// 32X
#include "gens_core/cpu/sh2/sh2.h"

// CD-ROM drive access
#ifdef GENS_CDROM
#include "segacd/cd_aspi.hpp"
#endif /* GENS_CDROM */

// Debugger
#ifdef GENS_DEBUGGER
#include "debugger/debugger.hpp"
#endif /* GENS_DEBUGGER */

// For some reason, these aren't extern'd anywhere...
extern "C"
{
	void main68k_reset();
	void sub68k_reset();
}

// C includes
#include <cstring>

// C++ includes
#include <string>
using std::string;

static int GensWindow_MenuItemCallback_FileMenu(uint16_t menuID, uint16_t state);
static int GensWindow_MenuItemCallback_GraphicsMenu(uint16_t menuID, uint16_t state);
static int GensWindow_MenuItemCallback_CPUMenu(uint16_t menuID, uint16_t state);
static int GensWindow_MenuItemCallback_SoundMenu(uint16_t menuID, uint16_t state);
static int GensWindow_MenuItemCallback_OptionsMenu(uint16_t menuID, uint16_t state);
static int GensWindow_MenuItemCallback_HelpMenu(uint16_t menuID, uint16_t state);


/**
 * GensWindow_MenuItemCallback(): Menu item callback handler.
 * @param menuID Menu ID.
 * @param state Menu state. (Used for check/radio menu items.)
 * @return Non-zero if the callback was handled; 0 if the callback wasn't handled.
 */
int GensWindow_MenuItemCallback(uint16_t menuID, uint16_t state)
{
	// Determine which menu this menu item is from.
	switch (menuID & 0xF000)
	{
		case IDM_FILE_MENU:
			return GensWindow_MenuItemCallback_FileMenu(menuID, state);
			break;
		case IDM_GRAPHICS_MENU:
			return GensWindow_MenuItemCallback_GraphicsMenu(menuID, state);
			break;
		case IDM_CPU_MENU:
			return GensWindow_MenuItemCallback_CPUMenu(menuID, state);
			break;
		case IDM_SOUND_MENU:
			return GensWindow_MenuItemCallback_SoundMenu(menuID, state);
			break;
		case IDM_OPTIONS_MENU:
			return GensWindow_MenuItemCallback_OptionsMenu(menuID, state);
			break;
		case IDM_HELP_MENU:
			return GensWindow_MenuItemCallback_HelpMenu(menuID, state);
			break;
		default:
			// Menu item not handled.
			return 0;
	}
	
	// Menu item not handled.
	return 0;
}


static int GensWindow_MenuItemCallback_FileMenu(uint16_t menuID, uint16_t state)
{
	string filename;
	
	switch (menuID)
	{
		case IDM_FILE_OPENROM:
			/*
			if ((Check_If_Kaillera_Running()))
				return 0;
			*/
			if (audio->playingGYM())
				Stop_Play_GYM();
			if (ROM::getROM() != -1)
				Sync_Gens_Window();
			break;
		
#ifdef GENS_CDROM
		case IDM_FILE_BOOTCD:
			if (!ASPI_Initialized || !Num_CD_Drive)
			{
				fprintf(stderr, "%s: ASPI not initialized and/or no CD-ROM drive(s) detected.\n", __func__);
				break;
			}
			
			/*
			if (Check_If_Kaillera_Running())
				return 0;
			*/
			if (audio->playingGYM())
				Stop_Play_GYM();
			
			ROM::freeROM(Game); // Don't forget it !
			SegaCD_Started = Init_SegaCD(NULL);
			Sync_Gens_Window();
			break;
#endif /* GENS_CDROM */
		
#ifdef GENS_OS_WIN32
		case IDM_FILE_NETPLAY:
			// Netplay isn't supported yet in Gens/GS.
			// There are only two ways this message could be received:
			// 1. Someone sent the message using another program.
			// 2. Someone used a Win32 API tool to remove the MF_GRAYED style.
			//
			// So, let's give them what they really want: A Rick Roll! :)
			Paused = 1;
			//Pause_Screen();
			audio->clearSoundBuffer();
			ShellExecute(NULL, NULL, "http://www.youtube.com/watch?v=oHg5SJYRHA0", NULL, NULL, SW_MAXIMIZE);
			break;
#endif /* GENS_OS_WIN32 */
		
		case IDM_FILE_CLOSEROM:
			if (audio->soundInitialized())
				audio->clearSoundBuffer();
	
#ifdef GENS_DEBUGGER
			Debug = 0;
#endif /* GENS_DEBUGGER */
	
			/* TODO: NetPlay
			if (Net_Play)
			{
				if (Video.Full_Screen)
					Set_Render(0, -1, 1);
			}
			*/
			
			ROM::freeROM(Game);
			Sync_Gens_Window();
			break;
		
		case IDM_FILE_GAMEGENIE:
			Open_Game_Genie();
			break;
		
		case IDM_FILE_ROMHISTORY_0:
		case IDM_FILE_ROMHISTORY_1:
		case IDM_FILE_ROMHISTORY_2:
		case IDM_FILE_ROMHISTORY_3:
		case IDM_FILE_ROMHISTORY_4:
		case IDM_FILE_ROMHISTORY_5:
		case IDM_FILE_ROMHISTORY_6:
		case IDM_FILE_ROMHISTORY_7:
		case IDM_FILE_ROMHISTORY_8:
		case IDM_FILE_ROMHISTORY_9:
			// ROM History.
			if (audio->playingGYM())
				Stop_Play_GYM();
			
			if (strlen(Recent_Rom[menuID - IDM_FILE_ROMHISTORY]) > 0)
				ROM::openROM(Recent_Rom[menuID - IDM_FILE_ROMHISTORY]);
			
			Sync_Gens_Window();
			break;
		
		case IDM_FILE_LOADSTATE:
			/*
			if (Check_If_Kaillera_Running())
				return 0;
			*/
			filename = Savestate::selectFile(false, State_Dir);
			if (!filename.empty())
				Savestate::loadState(filename.c_str());
			break;
		
		case IDM_FILE_SAVESTATE:
			/*
			if (Check_If_Kaillera_Running())
				return 0;
			*/
			filename = Savestate::selectFile(true, State_Dir);
			if (!filename.empty())
				Savestate::saveState(filename.c_str());
			break;
		
		case IDM_FILE_QUICKLOAD:
			/*
			if (Check_If_Kaillera_Running())
			return 0;
			*/
			filename = Savestate::getStateFilename();
			Savestate::loadState(filename);
			break;
		
		case IDM_FILE_QUICKSAVE:
			/*
			if (Check_If_Kaillera_Running())
			return 0;
			*/
			filename = Savestate::getStateFilename();
			Savestate::saveState(filename.c_str());
			break;
		
		case IDM_FILE_CHANGESTATE_0:
		case IDM_FILE_CHANGESTATE_1:
		case IDM_FILE_CHANGESTATE_2:
		case IDM_FILE_CHANGESTATE_3:
		case IDM_FILE_CHANGESTATE_4:
		case IDM_FILE_CHANGESTATE_5:
		case IDM_FILE_CHANGESTATE_6:
		case IDM_FILE_CHANGESTATE_7:
		case IDM_FILE_CHANGESTATE_8:
		case IDM_FILE_CHANGESTATE_9:
			// Change state.
			Set_Current_State(menuID - IDM_FILE_CHANGESTATE);
			Sync_Gens_Window_FileMenu();
			break;
		
		case IDM_FILE_EXIT:
			close_gens();
			break;
		
		default:
			// Unknown menu item ID.
			return 0;
	}
	
	// Menu item handled.
	return 1;
}


static int GensWindow_MenuItemCallback_GraphicsMenu(uint16_t menuID, uint16_t state)
{
	switch (menuID)
	{
		case IDM_GRAPHICS_FULLSCREEN:
			/*
			if (Full_Screen)
				Set_Render(0, -1, 1);
			else
				Set_Render(1, Render_FS, 1);
			*/
			
			draw->setFullScreen(!draw->fullScreen());
			// TODO: See if draw->setRender() is still needed.
			//draw->setRender(Video.Render_Mode);
			Sync_Gens_Window_GraphicsMenu();
			break;
		
		case IDM_GRAPHICS_VSYNC:
			Change_VSync(!state);
			Sync_Gens_Window_GraphicsMenu();
			break;
		
		case IDM_GRAPHICS_STRETCH:
			Change_Stretch(!state);
			Sync_Gens_Window_GraphicsMenu();
			break;
		
#ifdef GENS_OPENGL
		case IDM_GRAPHICS_OPENGL:
			Change_OpenGL(!state);
			Sync_Gens_Window_GraphicsMenu();
			break;
		
		case IDM_GRAPHICS_OPENGL_FILTER:
			Video.glLinearFilter = !state;
			
			if (Video.glLinearFilter)
				MESSAGE_L("Enabled OpenGL Linear Filter", "Enabled OpenGL Linear Filter", 1500);
			else
				MESSAGE_L("Disabled OpenGL Linear Filter", "Disabled OpenGL Linear Filter", 1500);
			
			break;
		
		case IDM_GRAPHICS_OPENGL_RES_320:
			Set_GL_Resolution(320, 240);
			Sync_Gens_Window_GraphicsMenu();
			break;
		
		case IDM_GRAPHICS_OPENGL_RES_640:
			Set_GL_Resolution(640, 480);
			Sync_Gens_Window_GraphicsMenu();
			break;
		
		case IDM_GRAPHICS_OPENGL_RES_800:
			Set_GL_Resolution(800, 600);
			Sync_Gens_Window_GraphicsMenu();
			break;
		
		case IDM_GRAPHICS_OPENGL_RES_1024:
			Set_GL_Resolution(1024, 768);
			Sync_Gens_Window_GraphicsMenu();
			break;
		
		case IDM_GRAPHICS_OPENGL_RES_CUSTOM:
			Open_OpenGL_Resolution();
			Sync_Gens_Window_GraphicsMenu();
			break;
#endif /* GENS_OPENGL */
		
#ifdef GENS_OS_UNIX
		case IDM_GRAPHICS_BPP_15:
			draw->setBpp(15);
			Sync_Gens_Window_GraphicsMenu();
			MESSAGE_L("Selected 15-bit color depth", "Selected 15-bit color depth", 1500);
			break;
		
		case IDM_GRAPHICS_BPP_16:
			draw->setBpp(16);
			Sync_Gens_Window_GraphicsMenu();
			MESSAGE_L("Selected 16-bit color depth", "Selected 16-bit color depth", 1500);
			break;
		
		case IDM_GRAPHICS_BPP_32:
			draw->setBpp(32);
			Sync_Gens_Window_GraphicsMenu();
			MESSAGE_L("Selected 32-bit color depth", "Selected 32-bit color depth", 1500);
			break;
#endif /* GENS_OS_UNIX */
		
		case IDM_GRAPHICS_COLORADJUST:
			Open_Color_Adjust();
			break;
				
		case IDM_GRAPHICS_SPRITELIMIT:
			// Sprite Limit
			Set_Sprite_Limit(!state);
			Sync_Gens_Window_GraphicsMenu();
			break;
			
		case IDM_GRAPHICS_FRAMESKIP_AUTO:
		case IDM_GRAPHICS_FRAMESKIP_0:
		case IDM_GRAPHICS_FRAMESKIP_1:
		case IDM_GRAPHICS_FRAMESKIP_2:
		case IDM_GRAPHICS_FRAMESKIP_3:
		case IDM_GRAPHICS_FRAMESKIP_4:
		case IDM_GRAPHICS_FRAMESKIP_5:
		case IDM_GRAPHICS_FRAMESKIP_6:
		case IDM_GRAPHICS_FRAMESKIP_7:
		case IDM_GRAPHICS_FRAMESKIP_8:
			// Set the frame skip value.
			Set_Frame_Skip(menuID - IDM_GRAPHICS_FRAMESKIP - 1);
			Sync_Gens_Window_GraphicsMenu();
			break;
		
		case IDM_GRAPHICS_SCREENSHOT:
			audio->clearSoundBuffer();
			ImageUtil::screenShot();
			break;
		
		default:
			if ((menuID & 0xFF00) == IDM_GRAPHICS_RENDER)
			{
				// Render mode change.
				draw->setRender(menuID - IDM_GRAPHICS_RENDER);
				Sync_Gens_Window_GraphicsMenu();
			}
			else
			{
				// Unknown menu item ID.
				return 0;
			}
			break;
	}
	
	// Menu item handled.
	return 1;
}


static int GensWindow_MenuItemCallback_CPUMenu(uint16_t menuID, uint16_t state)
{
	switch (menuID)
	{
		case IDM_CPU_COUNTRY_AUTO:
		case IDM_CPU_COUNTRY_JAPAN_NTSC:
		case IDM_CPU_COUNTRY_USA:
		case IDM_CPU_COUNTRY_EUROPE:
		case IDM_CPU_COUNTRY_JAPAN_PAL:
			Change_Country(menuID - IDM_CPU_COUNTRY - 1);
			Sync_Gens_Window_CPUMenu();
			break;
		
		case IDM_CPU_COUNTRY_ORDER:
			Open_Country_Code();
			break;
		
		case IDM_CPU_HARDRESET:
			system_reset();
			break;
		
		case IDM_CPU_RESET68K:
		case IDM_CPU_RESETMAIN68K:
			/*
			if (Check_If_Kaillera_Running())
				return 0;
			*/
			
			if (!Game)
				break;
			
			Paused = 0;
			main68k_reset();
			if (Genesis_Started || _32X_Started)
			{
				MESSAGE_L("68000 CPU reset", "68000 CPU reset", 1000);
			}
			else if (SegaCD_Started)
			{
				MESSAGE_L("Main 68000 CPU reset", "Main 68000 CPU reset", 1000);
			}
			break;
		
		case IDM_CPU_RESETSUB68K:
			/*
			if (Check_If_Kaillera_Running())
				return 0;
			*/
			
			if (!Game || !SegaCD_Started)
				break;
			
			Paused = 0;
			sub68k_reset();
			MESSAGE_L("Sub 68000 CPU reset", "Sub 68000 CPU reset", 1000);
			break;
		
		case IDM_CPU_RESETMAINSH2:
			/*
			if (Check_If_Kaillera_Running())
				return 0;
			*/
			
			if (!Game || !_32X_Started)
				break;
			
			Paused = 0;
			SH2_Reset(&M_SH2, 1);
			MESSAGE_L("Master SH2 reset", "Master SH2 reset", 1000);
			break;
		
		case IDM_CPU_RESETSUBSH2:
			/*
			if (Check_If_Kaillera_Running())
				return 0;
			*/
			
			if (!Game || !_32X_Started)
				break;
			
			Paused = 0;
			SH2_Reset(&S_SH2, 1);
			MESSAGE_L("Slave SH2 reset", "Slave SH2 reset", 1000);
			break;
		
		case IDM_CPU_RESETZ80:
			/*
			if (Check_If_Kaillera_Running())
				return 0;
			*/
			
			if (!Game)
				break;
			
			z80_Reset(&M_Z80);
			MESSAGE_L("Z80 reset", "Z80 reset", 1000);
			break;
		
		case IDM_CPU_SEGACDPERFECTSYNC:
			Change_SegaCD_PerfectSync(!state);
			Sync_Gens_Window_CPUMenu();
			break;
		
		default:
			if ((menuID & 0xFF00) == IDM_CPU_DEBUG)
			{
				// Debug mode change.
				Change_Debug((menuID - IDM_CPU_DEBUG) + 1);
				Sync_Gens_Window_CPUMenu();
			}
			else
			{
				// Unknown menu item ID.
				return 0;
			}
			break;
	}
	
	// Menu item handled.
	return 1;
}


static int GensWindow_MenuItemCallback_SoundMenu(uint16_t menuID, uint16_t state)
{
	switch (menuID)
	{
		case IDM_SOUND_ENABLE:
			Change_Sound(!state);
			break;
		
		case IDM_SOUND_STEREO:
			Change_Sound_Stereo(!state);
			break;
		
		case IDM_SOUND_Z80:
			Change_Z80(!state);
			break;
		
		case IDM_SOUND_YM2612:
			Change_YM2612(!state);
			break;
		
		case IDM_SOUND_YM2612_IMPROVED:
			Change_YM2612_Improved(!state);
			break;
		
		case IDM_SOUND_DAC:
			Change_DAC(!state);
			break;
		
		case IDM_SOUND_DAC_IMPROVED:
			Change_DAC_Improved(!state);
			break;
		
		case IDM_SOUND_PSG:
			Change_PSG(!state);
			break;
		
		case IDM_SOUND_PSG_IMPROVED:
			Change_PSG_Improved(!state);
			break;
		
		case IDM_SOUND_PCM:
			Change_PCM(!state);
			break;
		
		case IDM_SOUND_PWM:
			Change_PWM(!state);
			break;
		
		case IDM_SOUND_CDDA:
			Change_CDDA(!state);
			break;
		
		case IDM_SOUND_WAVDUMP:
			// Change WAV dump status.
			if (!state)
				audio->startWAVDump();
			else
				audio->stopWAVDump();
			break;
		
		case IDM_SOUND_GYMDUMP:
			// Change GYM dump status.
			if (!state)
				Start_GYM_Dump();
			else
				Stop_GYM_Dump();
			break;
		
		default:
			if ((menuID & 0xFF00) == IDM_SOUND_RATE)
			{
				// Sample rate change.
				Change_Sample_Rate(menuID - IDM_SOUND_RATE);
			}
			else
			{
				// Unknown menu item ID.
				return 0;
			}
			break;
	}
	
	// Synchronize the Sound Menu
	Sync_Gens_Window_SoundMenu();
	
	// Menu item handled.
	return 1;
}


static int GensWindow_MenuItemCallback_OptionsMenu(uint16_t menuID, uint16_t state)
{
	switch (menuID)
	{
		case IDM_OPTIONS_GENERAL:
			Open_General_Options();
			break;
		
		case IDM_OPTIONS_JOYPADS:
			Open_Controller_Config();
			break;
		
		case IDM_OPTIONS_BIOSMISCFILES:
			Open_BIOS_Misc_Files();
			break;
		
		case IDM_OPTIONS_CURRENT_CD_DRIVE:
			Open_Select_CDROM();
			break;
		
		case IDM_OPTIONS_LOADCONFIG:
			Config::loadAs(Game);
			Sync_Gens_Window();
			break;
		
		case IDM_OPTIONS_DIRECTORIES:
			Open_Directory_Config();
			break;
		
		case IDM_OPTIONS_SAVECONFIGAS:
			Config::saveAs();
			break;
		
		default:
			if ((menuID & 0xFF00) == IDM_OPTIONS_SEGACDSRAMSIZE)
			{
				// SegaCD SRAM Size change.
				Change_SegaCD_SRAM_Size(menuID - IDM_OPTIONS_SEGACDSRAMSIZE - 1);
				Sync_Gens_Window_OptionsMenu();
			}
			else
			{
				// Unknown menu item ID.
				return 0;
			}
			break;
	}

	// Menu item handled.
	return 1;
}


static int GensWindow_MenuItemCallback_HelpMenu(uint16_t menuID, uint16_t state)
{
	switch (menuID)
	{
		case IDM_HELP_ABOUT:
			AboutWindow::Instance();
			break;
			
		default:
			// Unknown menu item ID.
			return 0;
	}
	
	// Menu item handled.
	return 1;
}
