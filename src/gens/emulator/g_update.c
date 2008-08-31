/**
 * Gens: Update_Emulation functions.
 */

#include "gens.h"
#include "gens_core/mem/mem_m68k.h"
#include "sdllayer/g_sdldraw.h"
#include "sdllayer/g_sdlsound.h"
#include "sdllayer/g_sdlinput.h"

#ifndef NULL
#define NULL 0
#endif

// TODO: Get rid of this.
void Sleep(int i);

int Update_Emulation(void)
{
	static int Over_Time = 0;
	int current_div;

	if (Frame_Skip != -1)
	{
		if (Sound_Enable)
		{
			Write_Sound_Buffer(NULL);
		}

		Update_Controllers();

		if (Frame_Number++ < Frame_Skip)
		{
			Update_Frame_Fast();
		}
		else
		{
			Frame_Number = 0;
			Update_Frame();
			Flip();
		}
	}
	else
	{
		if (Sound_Enable)
		{
			// This does auto-frame skip in a fairly dodgy way -
			// only updating the frame when we have 'lots' in
			// the audio buffer. Hence the audio is a couple of
			// cycles ahead of the graphics.
			
			Write_Sound_Buffer(NULL);
			while (!Lots_In_Audio_Buffer())
			{
				Update_Frame_Fast();
				Write_Sound_Buffer(NULL);
			}

			Update_Controllers();
			Update_Frame();
			Flip();
		} //If sound is enabled
		
		else
		{
			if (CPU_Mode)
				current_div = 20;
			else
				current_div = 16 + (Over_Time ^= 1);

			New_Time = GetTickCount();
			Used_Time += (New_Time - Last_Time);
			Frame_Number = Used_Time / current_div;
			Used_Time %= current_div;
			Last_Time = New_Time;

			if (Frame_Number > 8) Frame_Number = 8;

			for (; Frame_Number > 1; Frame_Number--)
			{
				Update_Controllers();
				Update_Frame_Fast();
			}

			if (Frame_Number)
			{
				Update_Controllers();
				Update_Frame();
				Flip();
			}
			else {Sleep(Sleep_Time);}
		} //If sound is not enabled
		
	}

	return 1;
}


int Update_Emulation_One(void)
{
	Update_Controllers();
	Update_Frame();
	Flip();

	return 1;
}


#if 0
int Update_Emulation_Netplay(int player, int num_player)
{
	static int Over_Time = 0;
	int current_div;

	if (CPU_Mode) current_div = 20;
	else current_div = 16 + (Over_Time ^= 1);

	New_Time = GetTickCount();
	Used_Time += (New_Time - Last_Time);
	Frame_Number = Used_Time / current_div;
	Used_Time %= current_div;
	Last_Time = New_Time;

	if (Frame_Number > 6) Frame_Number = 6;

	for (; Frame_Number > 1; Frame_Number--)
{
		if (Sound_Enable)
{
			if (WP == Get_Current_Seg()) WP = (WP - 1) & (Sound_Segs - 1);
			Write_Sound_Buffer(NULL);
			WP = (WP + 1) & (Sound_Segs - 1);
}

		Scan_Player_Net(player);
		if (Kaillera_Error != -1) Kaillera_Error = Kaillera_Modify_Play_Values((void *) (Kaillera_Keys), 2);
		//Kaillera_Error = Kaillera_Modify_Play_Values((void *) (Kaillera_Keys), 2);
		Update_Controllers_Net(num_player);
		Update_Frame_Fast();
}

	if (Frame_Number)
{
		if (Sound_Enable)
{
			if (WP == Get_Current_Seg()) WP = (WP - 1) & (Sound_Segs - 1);
			Write_Sound_Buffer(NULL);
			WP = (WP + 1) & (Sound_Segs - 1);
}

		Scan_Player_Net(player);
		if (Kaillera_Error != -1) Kaillera_Error = Kaillera_Modify_Play_Values((void *) (Kaillera_Keys), 2);
		//Kaillera_Error = Kaillera_Modify_Play_Values((void *) (Kaillera_Keys), 2);
		Update_Controllers_Net(num_player);
		Update_Frame();
		Flip();
}
	return 1;
}
#endif
