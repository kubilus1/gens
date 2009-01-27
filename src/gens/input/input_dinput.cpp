/***************************************************************************
 * Gens: Input Handler - DirectInput 5 Backend.                            *
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

#include "input.h"
#include "input_dinput.hpp"
#include "input_win32_keys.h"

#include "emulator/g_main.hpp"
#include "ui/gens_ui.hpp"
#include "gens/gens_window.hpp"

#include "controller_config/controller_config_window.hpp"

#include <windows.h>
#include <dinput.h>
//#include <mmsystem.h>

// Wine's headers are missing these definitions...
#ifndef IID_IDirectInputDevice2
#ifdef UNICODE
#define IID_IDirectInputDevice2 IID_IDirectInputDevice2W
#else
#define IID_IDirectInputDevice2 IID_IDirectInputDevice2A
#endif /* UNICODE */
#endif /* IID_IDirectInputDevice2 */

#ifndef IDirectInputDevice2
#ifdef UNICODE
#define IDirectInputDevice2 IDirectInputDevice2W
#else
#define IDirectInputDevice2 IDirectInputDevice2A
#endif /* UNICODE */
#endif /* IDirectInputDevice2 */

#ifndef IDirectInputDevice2Vtbl
#ifdef UNICODE
#define IDirectInputDevice2Vtbl IDirectInputDevice2WVtbl
#else
#define IDirectInputDevice2Vtbl IDirectInputDevice2AVtbl
#endif /* UNICODE */
#endif /* IDirectInputDevice2Vtbl */

#define MAX_JOYS 8

// DirectInput versions.
static const unsigned short DIRECTINPUT_VERSION_5 = 0x0500;
static const unsigned short DIRECTINPUT_VERSION_3 = 0x0300;

// DirectInput variables
static LPDIRECTINPUT lpDI = NULL;
static LPDIRECTINPUTDEVICE lpDIDKeyboard = NULL;
static LPDIRECTINPUTDEVICE lpDIDMouse = NULL;
static IDirectInputDevice2 *input_dinput_joy_id[MAX_JOYS];
static DIJOYSTATE input_dinput_joy_state[MAX_JOYS];

// DirectInput version
static unsigned short input_dinput_version = 0;

// DirectInput Keys array
static unsigned char input_dinput_keys[256];

// Joysticks
static BOOL input_dinput_joystick_initialized;
static BOOL input_dinput_joystick_error;
static int input_dinput_num_joysticks;	// Number of joysticks connected
static BOOL input_dinput_init_joysticks_enum(LPCDIDEVICEINSTANCE lpDIIJoy, LPVOID pvRef);
int input_dinput_set_cooperative_level_joysticks(HWND hWnd);

// Miscellaneous DirectInput functions.
static inline void input_dinput_restore_input(void);

// Default keymap.
const input_keymap_t input_dinput_keymap_default[8] =
{
	// Player 1
	{DIK_UP, DIK_DOWN, DIK_LEFT, DIK_RIGHT,
	DIK_RETURN, DIK_A, DIK_S, DIK_D,
	DIK_RSHIFT, DIK_Z, DIK_X, DIK_C},
	
	// Players 1B, 1C, 1D
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	
	// Player 2
	{DIK_Y, DIK_H, DIK_G, DIK_J,
	DIK_U, DIK_K, DIK_L, DIK_M,
	DIK_T, DIK_I, DIK_O, DIK_P},
	
	// Players 2B, 2C, 2D
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// Axis values.
static const unsigned char input_dinput_joy_axis_values[2][6] =
{
	// axis value < -10,000
	{0x03, 0x01, 0x07, 0x05, 0x0B, 0x09},
	
	// axis value > 10,000
	{0x04, 0x02, 0x08, 0x06, 0x0C, 0x0A},
};

#define MOD_NONE 0
#define VK_NONE 0
#define ID_NONE 0


/**
 * input_dinput_init(): Initialize the DirectInput subsystem.
 * @return 0 on success; non-zero on error.
 */
int input_dinput_init(void)
{
	int i;
	HRESULT rval;
	
	// Attempt to initialize DirectInput 5.
	input_dinput_version = 0;
	rval = DirectInputCreate(ghInstance, DIRECTINPUT_VERSION_5, &lpDI, NULL);
	if (rval == DI_OK)
	{
		// DirectInput 5 initialized.
		fprintf(stderr, "Input_DInput(): Initialized DirectInput 5.\n");
		input_dinput_version = DIRECTINPUT_VERSION_5;
	}
	else
	{
		// Attempt to initialize DirectInput 3.
		rval = DirectInputCreate(ghInstance, DIRECTINPUT_VERSION_3, &lpDI, NULL);
		if (rval == DI_OK)
		{
			// DirectInput 3 initialized.
			fprintf(stderr, "Input_DInput(): Initialized DirectInput 3.\n");
			input_dinput_version = DIRECTINPUT_VERSION_3;
		}
		else
		{
			// DirectInput could not be initialized.
			fprintf(stderr, "Input_DInput(): Could not initialize DirectInput 3 or DirectInput 5.\n");
			
			GensUI::msgBox("input_dinput_init(): DirectInputCreate() failed.\n\nYou must have DirectX 3 or later.",
				       "DirectInput Error", GensUI::MSGBOX_ICON_ERROR);
			return -1;
		}
	}
	
	input_dinput_joystick_initialized = false;
	input_dinput_joystick_error = false;
	input_dinput_num_joysticks = 0;
	memset(input_dinput_joy_id, 0x00, sizeof(input_dinput_joy_id));
	
	//rval = lpDI->CreateDevice(GUID_SysMouse, &lpDIDMouse, NULL);
	lpDIDMouse = NULL;
	rval = lpDI->CreateDevice(GUID_SysKeyboard, &lpDIDKeyboard, NULL);
	if (rval != DI_OK)
	{
		GensUI::msgBox("input_dinput_init(): CreateDevice() failed.", "DirectInput Error", GensUI::MSGBOX_ICON_ERROR);
		
		// TODO: Use cross-platform error numbers, not just DirectInput return values.
		return -2;
	}
	
	// Set the cooperative level.
	input_dinput_set_cooperative_level(NULL);
	
	//rval = lpDIDMouse->SetDataFormat(&c_dfDIMouse);
	rval = lpDIDKeyboard->SetDataFormat(&c_dfDIKeyboard);
	if (rval != DI_OK)
	{
		GensUI::msgBox("Input_DInput(): SetDataFormat() failed.", "DirectInput Error", GensUI::MSGBOX_ICON_ERROR);
		
		// TODO: Use cross-platform error numbers, not just DirectInput return values.
		return -1;
	}
	
	//rval = lpDIDMouse->Acquire();
	for(i = 0; i < 10; i++)
	{
		rval = lpDIDKeyboard->Acquire();
		if (rval == DI_OK)
			break;
		GensUI::sleep(10);
	}
	
	// Clear the DirectInput arrays.
	memset(input_dinput_keys, 0x00, sizeof(input_dinput_keys));
	memset(input_dinput_joy_id, 0x00, sizeof(input_dinput_joy_id));
	memset(input_dinput_joy_state, 0x00, sizeof(input_dinput_joy_state));
	
	// DirectInput initialized.
	return 0;
}


/**
 * input_dinput_end(): Shut down the DirectInput subsystem.
 * @return 0 on success; non-zero on error.
 */
int input_dinput_end(void)
{
	// If any joysticks were opened, close them.
	if (!lpDI)
		return 0;
	
	if (lpDIDMouse)
	{
		lpDIDMouse->Release();
		lpDIDMouse = NULL;
	}
	
	if (lpDIDKeyboard)
	{
		lpDIDKeyboard->Release();
		lpDIDKeyboard = NULL;
	}
	
	for (int i = 0; i < MAX_JOYS; i++)
	{
		if (input_dinput_joy_id[i])
		{
			input_dinput_joy_id[i]->Unacquire();
			input_dinput_joy_id[i]->Release();
			input_dinput_joy_id[i] = NULL;
		}
	}
	
	input_dinput_num_joysticks = 0;
	lpDI->Release();
	lpDI = NULL;
	
	// DirectInput shut down.
	return 0;
}


/**
 * input_dinput_init_joysticks(): Initialize joysticks.
 * @param hWnd Window handle where the joysticks should be acquired initially.
 * @return 0 on success; non-zero on error.
 */
int input_dinput_init_joysticks(HWND hWnd)
{
	// TODO: Check if joysticks work with DirectInput 3.
	if (input_dinput_joystick_error)
		return -1;
	
	if (input_dinput_joystick_initialized)
	{
		// Joysticks are already initialized.
		// Set the cooperative level.
		// TODO: If set to DISCL_FOREGROUND, run setCooperativeLevel_Joysticks().
		// TODO: If set to DISCL_BACKGROUND, don't run setCooperativeLevel_Joysticks().
		// Currently hard-coded for DISCL_BACKGROUND.
		//setCooperativeLevel_Joysticks(hWnd);
		return 0;
	}
	
	// Joysticks are being initialized.
	input_dinput_joystick_initialized = true;
	
	HRESULT rval;
	rval = lpDI->EnumDevices(DIDEVTYPE_JOYSTICK,
				 (LPDIENUMDEVICESCALLBACK)(&input_dinput_init_joysticks_enum),
				 hWnd, DIEDFL_ATTACHEDONLY);
	if (rval != DI_OK)
	{
		input_dinput_joystick_error = true;
		GensUI::msgBox("input_dinput_init_joysticks(): EnumDevices() failed.",
			       "DirectInput Error", GensUI::MSGBOX_ICON_ERROR);
		// TODO: Error handling.
		return -2;
	}
}


static BOOL input_dinput_init_joysticks_enum(LPCDIDEVICEINSTANCE lpDIIJoy, LPVOID pvRef)
{
	HRESULT rval;
	LPDIRECTINPUTDEVICE	lpDIJoy;
	DIPROPRANGE diprg;
	int i;
 
	if (input_dinput_num_joysticks >= MAX_JOYS)
		return DIENUM_STOP;
		
	input_dinput_joy_id[input_dinput_num_joysticks] = NULL;
	
	rval = lpDI->CreateDevice(lpDIIJoy->guidInstance, &lpDIJoy, NULL);
	if (rval != DI_OK)
	{
		GensUI::msgBox("IDirectInput::CreateDevice() FAILED", "Joystick Error", GensUI::MSGBOX_ICON_ERROR);
		return(DIENUM_CONTINUE);
	}
	
	rval = lpDIJoy->QueryInterface(IID_IDirectInputDevice2, (void **)&input_dinput_joy_id[input_dinput_num_joysticks]);
	lpDIJoy->Release();
	if (rval != DI_OK)
	{
		GensUI::msgBox("IDirectInputDevice2::QueryInterface() FAILED", "Joystick Error", GensUI::MSGBOX_ICON_ERROR);
		input_dinput_joy_id[input_dinput_num_joysticks] = NULL;
		return(DIENUM_CONTINUE);
	}
	
	rval = input_dinput_joy_id[input_dinput_num_joysticks]->SetDataFormat(&c_dfDIJoystick);
	if (rval != DI_OK)
	{
		GensUI::msgBox("IDirectInputDevice::SetDataFormat() FAILED", "Joystick Error", GensUI::MSGBOX_ICON_ERROR);
		input_dinput_joy_id[input_dinput_num_joysticks]->Release();
		input_dinput_joy_id[input_dinput_num_joysticks] = NULL;
		return(DIENUM_CONTINUE);
	}
	
	// TODO: Add an option to specify DISCL_FOREGROUND so the joysticks only work when the Gens window is active.
	rval = input_dinput_joy_id[input_dinput_num_joysticks]->SetCooperativeLevel((HWND)pvRef, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	
	if (rval != DI_OK)
	{
		GensUI::msgBox("IDirectInputDevice::SetCooperativeLevel() FAILED", "Joystick Error", GensUI::MSGBOX_ICON_ERROR);
		input_dinput_joy_id[input_dinput_num_joysticks]->Release();
		input_dinput_joy_id[input_dinput_num_joysticks] = NULL;
		return(DIENUM_CONTINUE);
	}
	
	diprg.diph.dwSize = sizeof(diprg); 
	diprg.diph.dwHeaderSize = sizeof(diprg.diph); 
	diprg.diph.dwObj = DIJOFS_X;
	diprg.diph.dwHow = DIPH_BYOFFSET;
	diprg.lMin = -1000; 
	diprg.lMax = +1000;
 
	rval = input_dinput_joy_id[input_dinput_num_joysticks]->SetProperty(DIPROP_RANGE, &diprg.diph);
	if ((rval != DI_OK) && (rval != DI_PROPNOEFFECT))
	{
		GensUI::msgBox("IDirectInputDevice::SetProperty() (X-Axis) FAILED", "Joystick Error", GensUI::MSGBOX_ICON_ERROR);
	}
	
	diprg.diph.dwSize = sizeof(diprg); 
	diprg.diph.dwHeaderSize = sizeof(diprg.diph); 
	diprg.diph.dwObj = DIJOFS_Y;
	diprg.diph.dwHow = DIPH_BYOFFSET;
	diprg.lMin = -1000; 
	diprg.lMax = +1000;
 
	rval = input_dinput_joy_id[input_dinput_num_joysticks]->SetProperty(DIPROP_RANGE, &diprg.diph);
	if ((rval != DI_OK) && (rval != DI_PROPNOEFFECT))
	{
		GensUI::msgBox("IDirectInputDevice::SetProperty() (Y-Axis) FAILED", "Joystick Error", GensUI::MSGBOX_ICON_ERROR);
	}
	
	for(i = 0; i < 10; i++)
	{
		rval = input_dinput_joy_id[input_dinput_num_joysticks]->Acquire();
		if (rval == DI_OK)
			break;
		GensUI::sleep(10);
	}
	
	input_dinput_num_joysticks++;
	
	return(DIENUM_CONTINUE);
}


/**
 * input_dinput_restore_input(): Restore DirectInput.
 */
static inline void input_dinput_restore_input(void)
{
	//lpDIDMouse->Acquire();
	lpDIDKeyboard->Acquire();
}


/**
 * input_dinput_joy_exists(): Check if the specified joystick exists.
 * @param joyNum Joystick number.
 * @return true if the joystick exists; false if it does not exist.
 */
BOOL input_dinput_joy_exists(int joyNum)
{
	if (joyNum < 0 || joyNum >= MAX_JOYS)
		return FALSE;
	
	if (input_dinput_joy_id[joyNum])
		return TRUE;
	
	// Joystick does not exist.
	return FALSE;
}


/**
 * input_dinput_get_key(): Get a key. (Used for controller configuration.)
 * @return Key value.
 */
unsigned int input_dinput_get_key(void)
{
	int i, j, joyIndex;
	
	bool prevReady = false;
	
	BOOL prevDiKeys[256];
	BOOL prevJoyKeys[256];
	
	BOOL curDiKeys[256];
	BOOL curJoyKeys[256];
	
	while (true)
	{
		// Compute the current state of all buttons.
		input_dinput_update();
		
		// Current state of DirectInput keys
		for (i = 0; i < 256; i++)
			curDiKeys[i] = (input_dinput_keys[i] & 0x80);
		
		// Current state of recognized buttons on joypad
		joyIndex = 0;
		for (i = 0; i < input_dinput_num_joysticks; i++)
		{
			if (!input_dinput_joy_exists(i))
				continue;
			
			curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].lY < -500);
			curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].lY > +500);
			curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].lX < -500);
			curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].lX > +500);
			
			for (j = 0; j < 4; j++)
			{
				curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].rgdwPOV[j] == 0);
				curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].rgdwPOV[j] == 9000);
				curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].rgdwPOV[j] == 18000);
				curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].rgdwPOV[j] == 27000);
			}
			
			for (j = 0; j < 32; j++)
			{
				curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].rgbButtons[j]);
			}
			
			curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].lRx < 0x3FFF);
			curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].lRx > 0xBFFF);
			curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].lRy < 0x3FFF);
			curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].lRy > 0xBFFF);
			curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].lZ < 0x3FFF);
			curJoyKeys[joyIndex++] = (input_dinput_joy_state[i].lZ > 0xBFFF);
		}
		
		// Compare buttons against the previous state to determine
		// what is pressed now that wasn't already pressed before.
		
		if (prevReady)
		{
			// Check for new DirectInput key presses.
			for (i = 1; i < 255; i++)
			{
				if (curDiKeys[i] && !prevDiKeys[i])
					return i;
			}
			
			// Check for new recognized joypad button presses.
			for (int index = 0; index < joyIndex; index++)
			{
				if (curJoyKeys[index] && !prevJoyKeys[index])
				{
					int joyIndex2 = 0;
					for (i = 0; i < input_dinput_num_joysticks; i++)
					{
						if (!input_dinput_joy_exists(i))
							continue;
						
						if (index == joyIndex2++)
						{
							return (0x1000 + (0x100 * i) + 0x1);
						}
						if (index == joyIndex2++)
						{
							return (0x1000 + (0x100 * i) + 0x2);
						}
						if (index == joyIndex2++)
						{
							return (0x1000 + (0x100 * i) + 0x3);
						}
						if (index == joyIndex2++)
						{
							return (0x1000 + (0x100 * i) + 0x4);
						}
						
						for (j = 0; j < 4; j++)
						{
							if (index == joyIndex2++)
							{
								return (0x1080 + (0x100 * i) + (0x10 * j) + 0x1);
							}
							if (index == joyIndex2++)
							{
								return (0x1080 + (0x100 * i) + (0x10 * j) + 0x2);
							}
							if (index == joyIndex2++)
							{
								return (0x1080 + (0x100 * i) + (0x10 * j) + 0x3);
							}
							if (index == joyIndex2++)
							{
								return (0x1080 + (0x100 * i) + (0x10 * j) + 0x4);
							}
						}
						
						for (j = 0; j < 32; j++)
						{
							if (index == joyIndex2++)
							{
								return (0x1010 + (0x100 * i) + j);
							}
						}
						
						if (index == joyIndex2++)
						{
							return (0x1000 + (0x100 * i) + 0x5);
						}
						if (index == joyIndex2++)
						{
							return (0x1000 + (0x100 * i) + 0x6);
						}
						if (index == joyIndex2++)
						{
							return (0x1000 + (0x100 * i) + 0x7);
						}
						if (index == joyIndex2++)
						{
							return (0x1000 + (0x100 * i) + 0x8);
						}
					}
				}
			}
		}
		
		// Update previous state.
		memcpy(prevDiKeys, curDiKeys, sizeof(prevDiKeys));
		memcpy(prevJoyKeys, curJoyKeys, sizeof(prevJoyKeys));
		prevReady = true;
	}
}


/**
 * input_dinput_update(): Update the input subsystem.
 * @return 0 on success; non-zero on error.
 */
int input_dinput_update(void)
{
	//DIMOUSESTATE MouseState;
	HRESULT rval;
	
	rval = lpDIDKeyboard->GetDeviceState(256, &input_dinput_keys);
	
	// HACK because DirectInput is totally wacky about recognizing the PAUSE/BREAK key
	// still not perfect with this, but at least it goes above a 25% success rate
	if(GetAsyncKeyState(VK_PAUSE)) // normally this should have & 0x8000, but apparently this key is too special for that to work
		input_dinput_keys[0xC5] |= 0x80;
	
	if ((rval == DIERR_INPUTLOST) | (rval == DIERR_NOTACQUIRED))
		input_dinput_restore_input();
	
	for (int i = 0; i < input_dinput_num_joysticks; i++)
	{
		if (input_dinput_joy_id[i])
		{
			input_dinput_joy_id[i]->Poll();
			rval = input_dinput_joy_id[i]->GetDeviceState(sizeof(input_dinput_joy_state[i]), &input_dinput_joy_state[i]);
			if (rval != DI_OK) input_dinput_joy_id[i]->Acquire();
		}
	}
	
	return 0;
	
	// TODO: Figure out what to do with the rest of this function.
	
	//rval = lpDIDMouse->GetDeviceState(sizeof(MouseState), &MouseState);
	
	//if ((rval == DIERR_INPUTLOST) | (rval == DIERR_NOTACQUIRED))
	//	Restore_Input();

	//MouseX = MouseState.lX;
	//MouseY = MouseState.lY;
	
	// TODO: This is from Gens/Rerecording's hotkey remapping system.
#if 0
	int numInputButtons = GetNumHotkeys();
	for(int i = 0; i < numInputButtons; i++)
	{
		InputButton& button = s_inputButtons[i];

		BOOL pressed = button.diKey ? Check_Key_Pressed(button.diKey) : FALSE;

		if(button.virtKey || button.modifiers)
		{
			bool pressed2 = button.virtKey ? !!(GetAsyncKeyState(button.virtKey) & 0x8000) : true;

			pressed2 &= !(button.modifiers & MOD_CONTROL) == !(GetAsyncKeyState(VK_CONTROL) & 0x8000);
			pressed2 &= !(button.modifiers & MOD_SHIFT) == !(GetAsyncKeyState(VK_SHIFT) & 0x8000);
			pressed2 &= !(button.modifiers & MOD_ALT) == !(GetAsyncKeyState(VK_MENU) & 0x8000);
			pressed2 &= !(button.modifiers & MOD_WIN) == !((GetAsyncKeyState(VK_LWIN)|GetAsyncKeyState(VK_RWIN)) & 0x8000);

			if(!button.diKey)
				pressed = TRUE;

			if(!pressed2)
				pressed = FALSE;
		}

		if (button.alias)
			*button.alias = pressed;

		BOOL oldPressed = button.heldNow;
		button.heldNow = pressed;

		if (pressed && !oldPressed && button.eventID && !button.ShouldUseAccelerator())
			SendMessage(Gens_hWnd, WM_COMMAND, button.eventID, 0);
	}
#endif
}


/**
 * input_dinput_check_key_pressed(): Checks if the specified key is pressed.
 * @param key Key to check.
 * @return TRUE if the key is pressed; FALSE if the key is not pressed.
 */
BOOL input_dinput_check_key_pressed(unsigned int key)
{
	// If the key value is <256, it's a keyboard key.
	if (key < 0x100)
		return ((input_dinput_keys[key] & 0x80) ? TRUE : FALSE);
	
	// Joystick "key" check.
	
	// Determine which joystick we're looking for.
	int joyNum = ((key >> 8) & 0xF);
	
	// Check that this joystick exists.
	if (!input_dinput_joy_exists(joyNum))
		return FALSE;
	
	// Joystick exists. Check the state.
	if (key & 0x80)
	{
		// Joystick POV
		int value = input_dinput_joy_state[joyNum].rgdwPOV[(key >> 4) & 3];
		if (value == -1)
			return FALSE;
		
		switch (key & 0xF)
		{
			case 1:
				if (value >= 29250 || value <= 6750)
					return TRUE;
				break;
			case 2:
				if (value >= 2250 && value <= 15750)
					return TRUE;
				break;
			case 3:
				if (value >= 11250 && value <= 24750)
					return TRUE;
				break;
			case 4:
				if (value >= 20250 && value <= 33750)
					return TRUE;
				break;
		}
	}
	else if (key & 0x70)
	{
		// Joystick buttons
		if (input_dinput_joy_state[joyNum].rgbButtons[(key & 0xFF) - 0x10])
			return TRUE;
	}
	else
	{
		// Joystick axes
		switch (key & 0xF)
		{
			case 1:
				if (input_dinput_joy_state[joyNum].lY < -500)
					return TRUE;
				break;
			case 2:
				if (input_dinput_joy_state[joyNum].lY > +500)
					return TRUE;
				break;
			case 3:
				if (input_dinput_joy_state[joyNum].lX < -500)
					return TRUE;
				break;
			case 4:
				if (input_dinput_joy_state[joyNum].lX > +500)
					return TRUE;
				break;
			case 5:
				if (input_dinput_joy_state[joyNum].lRx < 0x3FFF)
					return TRUE;
				break;
			case 6:
				if (input_dinput_joy_state[joyNum].lRx > 0xBFFF)
					return TRUE;
				break;
			case 7:
				if (input_dinput_joy_state[joyNum].lRy < 0x3FFF)
					return TRUE;
				break;
			case 8:
				if (input_dinput_joy_state[joyNum].lRy > 0xBFFF)
					return TRUE;
				break;
			case 9:
				if (input_dinput_joy_state[joyNum].lZ < 0x3FFF)
					return TRUE;
				break;
			case 10:
				if (input_dinput_joy_state[joyNum].lZ > 0xBFFF)
					return TRUE;
				break;
		}
	}
	
	// Key is not pressed.
	return FALSE;
}


/**
 * input_dinput_set_cooperative_level(): Sets the cooperative level.
 * @param hWnd Window to set the cooperative level on.
 * @return 0 on success; non-zero on error.
 */
int input_dinput_set_cooperative_level(HWND hWnd)
{
	// If no hWnd was specified, use the Gens window.
	if (!hWnd)
		hWnd = Gens_hWnd;
	
	if (!hWnd || !lpDIDKeyboard /*|| lpDIDMouse*/)
		return -1;
	
	HRESULT rval;
	//rval = lpDIDMouse->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	rval = lpDIDKeyboard->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (rval != DI_OK)
	{
		fprintf(stderr, "%s(): lpDIDKeyboard->SetCooperativeLevel() failed.\n", __func__);
		// TODO: Error handling code.
	}
	else
	{
		fprintf(stderr, "%s(): lpDIDKeyboard->SetCooperativeLevel() succeeded.\n", __func__);
	}
	
	return 0;
}


/**
 * input_dinput_set_cooperative_level_joysticks(): Sets the cooperative level on joysticks.
 * @param hWnd Window to set the cooperative level on.
 * @return 0 on success; non-zero on error.
 */
int input_dinput_set_cooperative_level_joysticks(HWND hWnd)
{
	// If no hWnd was specified, use the Gens window.
	if (!hWnd)
		hWnd = Gens_hWnd;
	
	HRESULT rval;
	for (int i = 0; i < MAX_JOYS; i++)
	{
		if (!input_dinput_joy_id[i])
			continue;
		
		rval = input_dinput_joy_id[i]->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
		
		if (rval != DI_OK)
		{
			fprintf(stderr, "%s(): SetCooperativeLevel() failed on joystick %d.\n", __func__, i);
			input_dinput_joy_id[input_dinput_num_joysticks]->Release();
			input_dinput_joy_id[input_dinput_num_joysticks] = NULL;
		}
		else
		{
			fprintf(stderr, "%s(): SetCooperativeLevel() succeeded on joystick %d.\n", __func__, i);
		}
	}
	
	return 0;
}
