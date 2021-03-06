#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _WIN32
#include "libgsft/w32u/w32u_libc.h"
#endif

// C includes.
#include <stdio.h>
#include <string.h>

// C++ includes.
#include <string>
using std::string;

#include "cd_sys.hpp"
#include "cd_file.h"
#include "emulator/gens.hpp"
#include "emulator/g_main.hpp"
#include "emulator/g_mcd.hpp"
#include "lc89510.h"
#include "gens_core/cpu/68k/star_68k.h"
#include "gens_core/mem/mem_m68k.h"
#include "gens_core/mem/mem_s68k.h"
#include "util/file/save.hpp"
#include "gens_core/misc/misc.h"

// Audio Handler.
#include "audio/audio.h"

// CD-ROM drive access
#ifdef GENS_CDROM
#include "cd_aspi.hpp"
#endif /* GENS_CDROM */

// Define GENS_NO_CD_AUDIO_DELAY to disable the delay between
// the time the SegaCD game requests playing an Audio CD track
// and the time the track begins to play.
//#define GENS_NO_CD_AUDIO_DELAY
#undef GENS_NO_CD_AUDIO_DELAY

int File_Add_Delay = 0;

int CDDA_Enable;

int CD_Audio_Buffer_L[8192];
int CD_Audio_Buffer_R[8192];
int CD_Audio_Buffer_Read_Pos = 0;
int CD_Audio_Buffer_Write_Pos = 2000;
int CD_Audio_Starting;

int CD_Present;
int CD_Load_System;
int CD_Timer_Counter = 0;

int CDD_Complete;

int track_number;		// Used for the Get_Track_Adr function

unsigned int CD_timer_st;	// Used for CD timer
unsigned int CD_LBA_st;		// Used for CD timer

// GENS Re-Recording
char played_tracks_linear[101] = {0};

_scd SCD;


#ifdef DEBUG_CD
FILE *debug_SCD_file;
#endif


// CDERR_TRAY_OPEN == KEY_NOTREADY [ASPI]
#define CDERR_TRAY_OPEN 2
/**
 * checkTrayOpen(): Check if the CD tray is open.
 * @return true if the tray is open; false if the tray is closed.
 */
static inline bool checkTrayOpen(void)
{
	if (SCD.Status_CDD != TRAY_OPEN)
		return false;
	
	// CD tray is open.
	CDD.Status = SCD.Status_CDD;
	
	CDD.Minute = 0;
	CDD.Seconde = 0;
	CDD.Frame = 0;
	CDD.Ext = 0;
	
	CDD_Complete = 1;
	
	return true;
}


// CDERR_NO_DISC == KEY_MEDIUMERR [ASPI]
#define CDERR_NO_DISC 3
/**
 * checkCDPresent(): Check if a CD is present.
 * @return true if a CD is present; false if a CD is not present.
 */
static inline bool checkCDPresent(void)
{
	if (CD_Present)
		return true;
	
	// CD is not present.
	SCD.Status_CDD = NOCD;
	CDD.Status = SCD.Status_CDD;
	
	CDD.Minute = 0;
	CDD.Seconde = 0;
	CDD.Frame = 0;
	CDD.Ext = 0;
	
	CDD_Complete = 1;
	
	return false;
}


void MSB2DWORD(unsigned int *d, const unsigned char *b)
{
	*d = (b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}


int MSF_to_LBA(const _msf* MSF)
{
	return (MSF->M * 60 * 75) + (MSF->S * 75) + MSF->F - 150;
}


void LBA_to_MSF(int lba, _msf* MSF)
{
	if (lba < -150)
		lba = 0;
	else
		lba += 150;
	
	MSF->M = lba / (60 * 75);
	MSF->S = (lba / 75) % 60;
	MSF->F = lba % 75;
}


// Modif N. -- extracted function
static inline int MSF_to_Ordered(_msf *MSF)
{
	//return MSF_to_LBA(MSF);
	return (MSF->M << 16) + (MSF->S << 8) + MSF->F;
}


unsigned int MSF_to_Track(_msf * MSF)
{
	// Modif N. -- changed to give better results (nothing goes silent) if the tracks are out of order
	int i, Start, Cur = 0, BestIndex = -1;
	unsigned int BestValue = ~0;
	
	Start = MSF_to_Ordered(MSF);
	
	for (i = SCD.TOC.First_Track; i <= (SCD.TOC.Last_Track + 1); i++)
	{
		if (i > SCD.TOC.First_Track)
			Cur = MSF_to_Ordered(&SCD.TOC.Tracks[i - SCD.TOC.First_Track].MSF);
		
#ifdef DEBUG_CD
		//fprintf(debug_SCD_file, " i = %d  start = %.8X  cur = %.8X\n", i, Start, Cur);
#endif
		if (Start >= Cur && (unsigned int)(Start - Cur) < BestValue)
		{
			BestIndex = i;
			BestValue = Start - Cur;
		}
	}
	
	if (BestIndex != -1)
		i = BestIndex;
	else
		return SCD.TOC.Last_Track;
	
	if (i > SCD.TOC.Last_Track)
		return SCD.TOC.Last_Track;
	if (i < SCD.TOC.First_Track)
		i = SCD.TOC.First_Track;
	
	return (unsigned int)i;
}


unsigned int LBA_to_Track(int lba)
{
	_msf MSF;
	
	LBA_to_MSF(lba, &MSF);
	return MSF_to_Track(&MSF);
}


void Track_to_MSF(int track, _msf * MSF)
{
	if (track < SCD.TOC.First_Track)
		track = SCD.TOC.First_Track;
	else if (track > SCD.TOC.Last_Track)
		track = SCD.TOC.Last_Track;
	
	MSF->M = SCD.TOC.Tracks[track - SCD.TOC.First_Track].MSF.M;
	MSF->S = SCD.TOC.Tracks[track - SCD.TOC.First_Track].MSF.S;
	MSF->F = SCD.TOC.Tracks[track - SCD.TOC.First_Track].MSF.F;
}


int Track_to_LBA(int track)
{
	_msf MSF;
	
	Track_to_MSF(track, &MSF);
	return MSF_to_LBA(&MSF);
}


void Flush_CD_Command(void)
{
	CDD_Complete = 0;
}


void Check_CD_Command(void)
{
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "CHECK CD COMMAND\n");
#endif
	
	// Check CDD
	
	if (CDD_Complete)
	{
		CDD_Complete = 0;
		
#ifdef DEBUG_CD
		fprintf(debug_SCD_file, "CDD exporting status\n");
		fprintf(debug_SCD_file,
			"Status=%.4X, Minute=%.4X, Seconde=%.4X, Frame=%.4X, Ext=%.4X\n",
			CDD.Status, CDD.Minute, CDD.Seconde, CDD.Frame, CDD.Ext);
#endif
		
		CDD_Export_Status();
		
		if (Int_Mask_S68K & 0x10)
			sub68k_interrupt(4, -1);
	}
	
	// Check CDC
	if (SCD.Status_CDC & 1)	// CDC is reading data ...
	{
#ifdef DEBUG_CD
		fprintf(debug_SCD_file, "Send a read command\n");
#endif
		
		// DATA ?
		if (SCD.TOC.Tracks[SCD.Cur_Track - SCD.TOC.First_Track].Type)
			CDD.Control |= 0x0100;
		else
			CDD.Control &= ~0x0100;	// AUDIO
		
		if (File_Add_Delay == 0)
		{
			// TODO: Create a struct of function pointers for different CD-ROM readers.
#ifdef GENS_CDROM
			if (CD_Load_System == CDROM_)
				ASPI_Read_One_LBA_CDC();
			else
#endif
				FILE_Read_One_LBA_CDC();
		}
		else
			File_Add_Delay--;
	}
	
	if (SCD.Status_CDD == FAST_FOW)
	{
		SCD.Cur_LBA += 10;
		CDC_Update_Header ();
	}
	else if (SCD.Status_CDD == FAST_REV)
	{
		SCD.Cur_LBA -= 10;
		if (SCD.Cur_LBA < -150)
			SCD.Cur_LBA = -150;
		CDC_Update_Header ();
	}
}


void Init_CD_Driver(void)
{
#ifdef DEBUG_CD
	debug_SCD_file = fopen("SCD.log", "w");
#endif
	
	// TODO: Create classes for different CD-ROM readers.
#ifdef GENS_CDROM
	// ASPI support
	ASPI_Init();
	
	// TODO: Show error message if ASPI can't be initialized.
	/*
	if (ASPI_Init() == 0)
	{
		GensUI::msgBox("No CD Drive or ASPI not correctly initialised\nTry to upgrade your ASPI drivers", "ASPI Error", MSGBOX_ICON_WARNING);
	}
	*/
#endif	
	// FILE ISO support
	FILE_Init();
	
	return;
}


void End_CD_Driver(void)
{
#ifdef DEBUG_CD
	fclose(debug_SCD_file);
#endif
	
	// ASPI support
#ifdef GENS_CDROM
	ASPI_End();
#endif
	
	// FILE ISO support
	FILE_End();
}


/**
 * Reset_CD(): Reset the emulated CD-ROM drive.
 * @param buf ???
 * @param iso_name Filename of the ISO image to load, or NULL to use ASPI.
 * @return 0 on success; non-zero on error.
 */
int Reset_CD(char *buf, const char *iso_name)
{
	// TODO: Return an error code if an error occurred.
	
	memset(CD_Audio_Buffer_L, 0x00, sizeof(CD_Audio_Buffer_L));
	memset(CD_Audio_Buffer_R, 0x00, sizeof(CD_Audio_Buffer_R));
	
#ifdef GENS_CDROM
	if (iso_name == NULL)
	{
		CD_Load_System = CDROM_;
		ASPI_Reset_Drive(buf);
		ASPI_Lock(1);
		return 0;
	}
	else
	{
#endif
		CD_Load_System = FILE_ISO;
		Load_ISO(buf, iso_name);
		CD_Present = 1;
		return 0;
#ifdef GENS_CDROM
	}
#endif
}


void Stop_CD(void)
{
#ifdef GENS_CDROM
	if (CD_Load_System == CDROM_)
	{
		ASPI_Lock(0);
		ASPI_Stop_Play_Scan(0, NULL);
	}
	else
#endif
		Unload_ISO();
}


void Change_CD(void)
{
	if (SCD.Status_CDD == TRAY_OPEN)
		Close_Tray_CDD_cC();
	else
		Open_Tray_CDD_cD();
}


int Get_Status_CDD_c0(void)
{
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "Status command : Cur LBA = %d\n", SCD.Cur_LBA);
#endif

	// Clear immediate status
	if ((CDD.Status & 0x0F00) == 0x0200)
		CDD.Status = (SCD.Status_CDD & 0xFF00) | (CDD.Status & 0x00FF);
	else if ((CDD.Status & 0x0F00) == 0x0700)
		CDD.Status = (SCD.Status_CDD & 0xFF00) | (CDD.Status & 0x00FF);
	else if ((CDD.Status & 0x0F00) == 0x0E00)
		CDD.Status = (SCD.Status_CDD & 0xFF00) | (CDD.Status & 0x00FF);
	
	/*
	CDD.Status = (SCD.Status_CDD & 0xFF00) | (CDD.Status & 0x00FF);
	CDD.Status = SCD.Status_CDD;
	
	CDD.Minute = 0;
	CDD.Seconde = 0;
	CDD.Frame = 0;
	CDD.Ext = 0;
	*/
	
	CDD_Complete = 1;
	
	return 0;
}


int Stop_CDD_c1(void)
{
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	
	SCD.Status_CDC &= ~1;	// Stop CDC read
	
#ifdef GENS_CDROM
	if (CD_Load_System == CDROM_)
	{
		return ASPI_Stop_Play_Scan(1, ASPI_Stop_CDD_c1_COMP);
	}
	else
#endif
	{
		if (CD_Present)
			SCD.Status_CDD = STOPPED;
		else
			SCD.Status_CDD = NOCD;
		CDD.Status = 0x0000;
		
		CDD.Control |= 0x0100;	// Data bit set because stopped	
		
		CDD.Minute = 0;
		CDD.Seconde = 0;
		CDD.Frame = 0;
		CDD.Ext = 0;
	}
	
	CDD_Complete = 1;
	return 0;
}


int Get_Pos_CDD_c20(void)
{
	_msf MSF;
	
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "command 200 : Cur LBA = %d\n", SCD.Cur_LBA);
#endif
	
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	
	CDD.Status &= 0xFF;
	if (!CD_Present)
	{
		SCD.Status_CDD = NOCD;
		CDD.Status |= SCD.Status_CDD;
	}
	/*
	else if (!(CDC.CTRL.B.B0 & 0x80))
		CDD.Status |= SCD.Status_CDD; //aggiunta
	*/
	CDD.Status |= SCD.Status_CDD;
	
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "Status CDD = %.4X  Status = %.4X\n",
		SCD.Status_CDD, CDD.Status);
#endif
	
	LBA_to_MSF(SCD.Cur_LBA, &MSF);
	
	CDD.Minute = INT_TO_BCDW(MSF.M);
	CDD.Seconde = INT_TO_BCDW(MSF.S);
	CDD.Frame = INT_TO_BCDW(MSF.F);
	CDD.Ext = 0;
	
	CDD_Complete = 1;
	return 0;
}


int Get_Track_Pos_CDD_c21(void)
{
	int elapsed_time;
	_msf MSF;
	
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "command 201 : Cur LBA = %d", SCD.Cur_LBA);
#endif
	
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	
	CDD.Status &= 0xFF;
	if (!CD_Present)
	{
		SCD.Status_CDD = NOCD;
		CDD.Status |= SCD.Status_CDD;
	}
	/*
	else if (!(CDC.CTRL.B.B0 & 0x80))
		CDD.Status |= SCD.Status_CDD;
	*/
	CDD.Status |= SCD.Status_CDD;
	
	elapsed_time = SCD.Cur_LBA - Track_to_LBA(LBA_to_Track(SCD.Cur_LBA));
	LBA_to_MSF (elapsed_time - 150, &MSF);
	
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "   elapsed = %d\n", elapsed_time);
#endif
	
	CDD.Minute = INT_TO_BCDW(MSF.M);
	CDD.Seconde = INT_TO_BCDW(MSF.S);
	CDD.Frame = INT_TO_BCDW(MSF.F);
	CDD.Ext = 0;
	
	CDD_Complete = 1;
	return 0;
}


int Get_Current_Track_CDD_c22(void)
{
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "Status CDD = %.4X  Status = %.4X\n",
		SCD.Status_CDD, CDD.Status);
#endif
	
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	
	CDD.Status &= 0xFF;
	if (!CD_Present)
	{
		SCD.Status_CDD = NOCD;
		CDD.Status |= SCD.Status_CDD;
	}
	/*
	else if (!(CDC.CTRL.B.B0 & 0x80))
		CDD.Status |= SCD.Status_CDD;
	*/
	CDD.Status |= SCD.Status_CDD;
	
	SCD.Cur_Track = LBA_to_Track(SCD.Cur_LBA);
	played_tracks_linear[SCD.Cur_Track - SCD.TOC.First_Track] = 1;
	
	if (SCD.Cur_Track == 100)
		CDD.Minute = 0x0A02;
	else
		CDD.Minute = INT_TO_BCDW(SCD.Cur_Track);
	CDD.Seconde = 0;
	CDD.Frame = 0;
	CDD.Ext = 0;
	
	CDD_Complete = 1;
	return 0;
}


int Get_Total_Length_CDD_c23(void)
{
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	
	CDD.Status &= 0xFF;
	if (!CD_Present)
	{
		SCD.Status_CDD = NOCD;
		CDD.Status |= SCD.Status_CDD;
	}
	/*
	else if (!(CDC.CTRL.B.B0 & 0x80))
		CDD.Status |= SCD.Status_CDD;
	*/
	CDD.Status |= SCD.Status_CDD;
	
	CDD.Minute = INT_TO_BCDW(SCD.TOC.Tracks[SCD.TOC.Last_Track - SCD.TOC.First_Track + 1].MSF.M);
	CDD.Seconde = INT_TO_BCDW(SCD.TOC.Tracks[SCD.TOC.Last_Track - SCD.TOC.First_Track + 1].MSF.S);
	CDD.Frame = INT_TO_BCDW(SCD.TOC.Tracks[SCD.TOC.Last_Track - SCD.TOC.First_Track + 1].MSF.F);
	CDD.Ext = 0;
	
	CDD_Complete = 1;
	return 0;
}


int Get_First_Last_Track_CDD_c24(void)
{
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	
	CDD.Status &= 0xFF;
	if (!CD_Present)
	{
		SCD.Status_CDD = NOCD;
		CDD.Status |= SCD.Status_CDD;
	}
	/*
	else if (!(CDC.CTRL.B.B0 & 0x80))
		CDD.Status |= SCD.Status_CDD;
	*/
	CDD.Status |= SCD.Status_CDD;
	
	CDD.Minute = INT_TO_BCDW(SCD.TOC.First_Track);
	CDD.Seconde = INT_TO_BCDW(SCD.TOC.Last_Track);
	CDD.Frame = 0;
	CDD.Ext = 0;
	
	CDD_Complete = 1;
	return 0;
}


int Get_Track_Adr_CDD_c25(void)
{
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	
	// track number in TC4 & TC5
	track_number = (CDD.Trans_Comm[4] & 0xF) + (CDD.Trans_Comm[5] & 0xF) * 10;
	
	CDD.Status &= 0xFF;
	if (!CD_Present)
	{
		SCD.Status_CDD = NOCD;
		CDD.Status |= SCD.Status_CDD;
	}
	/*
	else if (!(CDC.CTRL.B.B0 & 0x80))
		CDD.Status |= SCD.Status_CDD;
	*/
	CDD.Status |= SCD.Status_CDD;
	
	if (track_number > SCD.TOC.Last_Track)
		track_number = SCD.TOC.Last_Track;
	else if (track_number < SCD.TOC.First_Track)
		track_number = SCD.TOC.First_Track;
	
	CDD.Minute = INT_TO_BCDW(SCD.TOC.Tracks[track_number - SCD.TOC.First_Track].MSF.M);
	CDD.Seconde = INT_TO_BCDW(SCD.TOC.Tracks[track_number - SCD.TOC.First_Track].MSF.S);
	CDD.Frame = INT_TO_BCDW (SCD.TOC.Tracks[track_number - SCD.TOC.First_Track].MSF.F);
	CDD.Ext = track_number % 10;
	
	if (SCD.TOC.Tracks[track_number - SCD.TOC.First_Track].Type)
		CDD.Frame |= 0x0800;
	
	CDD_Complete = 1;
	return 0;
}


int Play_CDD_c3(void)
{
	_msf MSF;
	int delay, new_lba;
	
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	if (!checkCDPresent())
		return CDERR_NO_DISC;
	
	if (CD_Load_System == CDROM_) //aggiunta
	{
		SCD.Status_CDC &= ~1;	// Stop read data with CDC
#ifdef GENS_CDROM
		Wait_Read_Complete();
#endif
	}
	
	// MSF of the track to play in TC buffer
	
	MSF.M = (CDD.Trans_Comm[2] & 0xF) + (CDD.Trans_Comm[3] & 0xF) * 10;
	MSF.S = (CDD.Trans_Comm[4] & 0xF) + (CDD.Trans_Comm[5] & 0xF) * 10;
	MSF.F = (CDD.Trans_Comm[6] & 0xF) + (CDD.Trans_Comm[7] & 0xF) * 10;
	
	SCD.Cur_Track = MSF_to_Track(&MSF);
	played_tracks_linear[SCD.Cur_Track - SCD.TOC.First_Track] = 1;
	
	new_lba = MSF_to_LBA(&MSF);
#ifdef GENS_NO_CD_AUDIO_DELAY
	delay = 0;		//upth mod - for consistency given varying track lengths
#else
	delay = new_lba - SCD.Cur_LBA;
	if (delay < 0)
		delay = -delay;
	delay >>= 12;
#endif /* GENS_NO_CD_AUDIO_DELAY */
	
	SCD.Cur_LBA = new_lba;
	CDC_Update_Header();
	
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "Read : Cur LBA = %d, M=%d, S=%d, F=%d\n",
		SCD.Cur_LBA, MSF.M, MSF.S, MSF.F);
#endif
	
#ifdef GENS_CDROM
	if (CD_Load_System == CDROM_)
	{
		delay += 20;
#ifdef GENS_NO_CD_AUDIO_DELAY
		delay >>= 2;
		ASPI_Seek(SCD.Cur_LBA, 1, ASPI_Fast_Seek_COMP); //aggiunta
#endif /* GENS_NO_CD_AUDIO_DELAY */
		ASPI_Flush_Cache_CDC();
	}
	else
#endif /* GENS_CDROM */
	if (SCD.Status_CDD != PLAYING)
	{
		delay += 20;
#ifdef GENS_NO_CD_AUDIO_DELAY
		delay >>= 2; // Modif N. -- added for consistency
#endif /* GENS_NO_CD_AUDIO_DELAY */
	}
	
	SCD.Status_CDD = PLAYING;
	CDD.Status = 0x0102;
	//CDD.Status = COMM_OK;
	
	if (File_Add_Delay == 0)
		File_Add_Delay = delay;
	
	if (SCD.TOC.Tracks[SCD.Cur_Track - SCD.TOC.First_Track].Type)
	{
		// DATA
		CDD.Control |= 0x0100;
	}
	else
	{
		// AUDIO
		CDD.Control &= ~0x0100;
		CD_Audio_Starting = 1;
		if (CD_Load_System == FILE_ISO)
			FILE_Play_CD_LBA(1);
		played_tracks_linear[SCD.Cur_Track - SCD.TOC.First_Track] = 1;
	}
	
	if (SCD.Cur_Track == 100)
		CDD.Minute = 0x0A02;
	else
		CDD.Minute = INT_TO_BCDW(SCD.Cur_Track);
	CDD.Seconde = 0;
	CDD.Frame = 0;
	CDD.Ext = 0;
	
	// Read data with CDC
	SCD.Status_CDC |= 1;
	
	CDD_Complete = 1;
	return 0;
}


int Seek_CDD_c4(void)
{
	_msf MSF;
	
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	if (!checkCDPresent())
		return CDERR_NO_DISC;
	
	// MSF to seek in TC buffer
	MSF.M = (CDD.Trans_Comm[2] & 0xF) + (CDD.Trans_Comm[3] & 0xF) * 10;
	MSF.S = (CDD.Trans_Comm[4] & 0xF) + (CDD.Trans_Comm[5] & 0xF) * 10;
	MSF.F = (CDD.Trans_Comm[6] & 0xF) + (CDD.Trans_Comm[7] & 0xF) * 10;
	
	SCD.Cur_Track = MSF_to_Track(&MSF);
	played_tracks_linear[SCD.Cur_Track - SCD.TOC.First_Track] = 1;
	SCD.Cur_LBA = MSF_to_LBA(&MSF);
	CDC_Update_Header();
	
	// Stop CDC read
	SCD.Status_CDC &= ~1;
	
#ifdef GENS_CDROM
	if (CD_Load_System == CDROM_)
	{
		ASPI_Seek (SCD.Cur_LBA, 1, ASPI_Seek_CDD_c4_COMP);
#ifdef GENS_OS_LINUX
		// For some reason, this code isn't in Gens Rerecording.
		// To be safe, keep it for the Linux version.
		SCD.Status_CDD = READY;
		CDD.Status = 0x0200;
		
		// DATA ?
		if (SCD.TOC.Tracks[SCD.Cur_Track - SCD.TOC.First_Track].Type)
			CDD.Control |= 0x0100;
		else
			CDD.Control &= 0xFEFF;	// AUDIO
		
		CDD.Minute = 0;
		CDD.Seconde = 0;
		CDD.Frame = 0;
		CDD.Ext = 0;
#endif /* GENS_OS_LINUX */
	}
	else
#endif /* GENS_CDROM */
	{
		SCD.Status_CDD = READY;
		CDD.Status = 0x0200;
		
		// DATA ?
		if (SCD.TOC.Tracks[SCD.Cur_Track - SCD.TOC.First_Track].Type)
			CDD.Control |= 0x0100;
		else
			CDD.Control &= 0xFEFF;	// AUDIO
		
		CDD.Minute = 0;
		CDD.Seconde = 0;
		CDD.Frame = 0;
		CDD.Ext = 0;
	}
	
	CDD_Complete = 1;
	return 0;
}


int Pause_CDD_c6(void)
{
	// Stop CDC read to start a new one if raw data
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	if (!checkCDPresent())
		return CDERR_NO_DISC;
	
	// Stop CDC read to start a new one if raw data
	SCD.Status_CDC &= ~1;
	
	SCD.Status_CDD = READY;
	CDD.Status = SCD.Status_CDD;
	
	// Data bit set because stopped
	CDD.Control |= 0x0100;
	
	CDD.Minute = 0;
	CDD.Seconde = 0;
	CDD.Frame = 0;
	CDD.Ext = 0;
	
	CDD_Complete = 1;
	return 0;
}


int Resume_CDD_c7(void)
{
#ifdef DEBUG_CD
	_msf MSF;
#endif /* DEBUG_CD */
	
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	if (!checkCDPresent())
		return CDERR_NO_DISC;
	
#ifdef GENS_CDROM
	if (CD_Load_System == CDROM_)
	{
		// Stop read data with CDC
		SCD.Status_CDC &= ~1;
		Wait_Read_Complete ();
	}
#endif /* GENS_CDROM */
	else //if(CD_Load_System == FILE_CUE) // Modif N. -- added
		SCD.Status_CDC &= ~1;
	
	SCD.Cur_Track = LBA_to_Track(SCD.Cur_LBA);
	played_tracks_linear[SCD.Cur_Track - SCD.TOC.First_Track] = 1;
	
#ifdef DEBUG_CD
	LBA_to_MSF(SCD.Cur_LBA, &MSF);
	fprintf(debug_SCD_file, "Resume read : Cur LBA = %d, M=%d, S=%d, F=%d\n",
		SCD.Cur_LBA, MSF.M, MSF.S, MSF.F);
#endif /* DEBUG_CD */
	
	SCD.Status_CDD = PLAYING;
	CDD.Status = 0x0102;
	
#ifdef GENS_CDROM
	if (CD_Load_System == CDROM_)
	{
		ASPI_Seek(SCD.Cur_LBA, 1, ASPI_Fast_Seek_COMP);
		ASPI_Flush_Cache_CDC();
	}
#endif /* GENS_CDROM */
	
	if (SCD.TOC.Tracks[SCD.Cur_Track - SCD.TOC.First_Track].Type)
	{
		// DATA
		CDD.Control |= 0x0100;
	}
	else
	{
		// AUDIO
		CDD.Control &= ~0x0100;
		CD_Audio_Starting = 1;
		if (CD_Load_System == FILE_ISO)
			FILE_Play_CD_LBA(1);
	}
	
	if (SCD.Cur_Track == 100)
		CDD.Minute = 0x0A02;
	else
		CDD.Minute = INT_TO_BCDW (SCD.Cur_Track);
	CDD.Seconde = 0;
	CDD.Frame = 0;
	CDD.Ext = 0;
	
	// Read data with CDC
	SCD.Status_CDC |= 1;
	
	CDD_Complete = 1;
	return 0;
}


int Fast_Foward_CDD_c8(void)
{
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	if (!checkCDPresent())
		return CDERR_NO_DISC;
	
	// Stop CDC read
	SCD.Status_CDC &= ~1;
	
	SCD.Status_CDD = FAST_FOW;
	CDD.Status = SCD.Status_CDD | 2;
	
	CDD.Minute = INT_TO_BCDW(SCD.Cur_Track);
	CDD.Seconde = 0;
	CDD.Frame = 0;
	CDD.Ext = 0;
	
	CDD_Complete = 1;	
	return 0;
}


int Fast_Rewind_CDD_c9(void)
{
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	if (!checkCDPresent())
		return CDERR_NO_DISC;
	
	// Stop CDC read
	SCD.Status_CDC &= ~1;
	
	SCD.Status_CDD = FAST_REV;
	CDD.Status = SCD.Status_CDD | 2;
	
	CDD.Minute = INT_TO_BCDW(SCD.Cur_Track);
	CDD.Seconde = 0;
	CDD.Frame = 0;
	CDD.Ext = 0;
	
	CDD_Complete = 1;
	return 0;
}


int Close_Tray_CDD_cC(void)
{
	audio_clear_sound_buffer();
	
	// Stop CDC read
	SCD.Status_CDC &= ~1;
	
#ifdef GENS_CDROM
	if (CD_Load_System == CDROM_)
	{
		ASPI_Lock(0);
		ASPI_Star_Stop_Unit(CLOSE_TRAY, 0, 0, ASPI_Close_Tray_CDD_cC_COMP);
		
		Reload_SegaCD(NULL);
		
		if (CD_Present)
			SCD.Status_CDD = STOPPED;
		else
			SCD.Status_CDD = NOCD;
		CDD.Status = 0x0000;
		
		CDD.Minute = 0;
		CDD.Seconde = 0;
		CDD.Frame = 0;
		CDD.Ext = 0;
	}
	else
#endif /* GENS_CDROM */
	{
		string new_iso = Savestate::SelectCDImage(Rom_Dir);
		if (!new_iso.empty())
		{
			Reload_SegaCD(new_iso.c_str());
			CD_Present = 1;
			SCD.Status_CDD = STOPPED;
		}
		
		CDD.Status = 0x0000;
		
		CDD.Minute = 0;
		CDD.Seconde = 0;
		CDD.Frame = 0;
		CDD.Ext = 0;
	}
		
	CDD_Complete = 1;
	return 0;
}


int Open_Tray_CDD_cD(void)
{
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	
	// Stop CDC read
	SCD.Status_CDC &= ~1;
	
#ifdef GENS_CDROM
	if (CD_Load_System == CDROM_)
	{
		audio_clear_sound_buffer();
		
		ASPI_Lock(0);
		ASPI_Star_Stop_Unit(OPEN_TRAY, 0, 0, ASPI_Open_Tray_CDD_cD_COMP);
		
		ASPI_Lock(1);
		return 0;
	}
	else
	{
#endif
		Unload_ISO();
		CD_Present = 0;
		
		SCD.Status_CDD = TRAY_OPEN;
		CDD.Status = 0x0E00;
		
		CDD.Minute = 0;
		CDD.Seconde = 0;
		CDD.Frame = 0;
		CDD.Ext = 0;
#ifdef GENS_CDROM
	}
#endif
	
	CDD_Complete = 1;
	return 0;
}


int CDD_cA(void)
{
	if (checkTrayOpen())
		return CDERR_TRAY_OPEN;
	if (!checkCDPresent())
		return CDERR_NO_DISC;
	
	SCD.Status_CDC &= ~1;
	
	SCD.Status_CDD = READY;
	CDD.Status = SCD.Status_CDD;
	
	CDD.Minute = 0;
	CDD.Seconde = INT_TO_BCDW(1);
	CDD.Frame = INT_TO_BCDW(1);
	CDD.Ext = 0;
	
	CDD_Complete = 1;
	return 0;
}


int CDD_Def(void)
{
	CDD.Status = SCD.Status_CDD;
	
	CDD.Minute = 0;
	CDD.Seconde = 0;
	CDD.Frame = 0;
	CDD.Ext = 0;
	
	return 0;
}




/***************************
 *   Others CD functions   *
 **************************/


// TODO: Port new Write_CD_Audio() asm code from Gens Rerecording.


void Write_CD_Audio(short *Buf, int rate, int channel, int length)
{
	unsigned int length_src, length_dst;
	unsigned int pos_src, pas_src;
	unsigned int tmp;
	
	if (rate == 0)
		return;
	if (audio_get_sound_rate() == 0)
		return;
	
	if (CD_Audio_Starting)
	{
		CD_Audio_Starting = 0;
		memset(CD_Audio_Buffer_L, 0x00, sizeof(CD_Audio_Buffer_L));
		memset(CD_Audio_Buffer_R, 0x00, sizeof(CD_Audio_Buffer_R));
		CD_Audio_Buffer_Write_Pos = (CD_Audio_Buffer_Read_Pos + 2000) & 0xFFF;
	}
	
	length_src = rate / 75;			// 75th of a second
	length_dst = audio_get_sound_rate() / 75;	// 75th of a second
	
	pas_src = (length_src << 16) / length_dst;
	pos_src = 0;
	
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "\n*********  Write Pos = %d    ",
		CD_Audio_Buffer_Write_Pos);
#endif
	
	if (channel == 2)
	{
		while (length_dst > 0)
		{
			length_dst--;
			tmp = pos_src >> 15;	/* the same as "(pos_src >> 16) * 2", but faster */
			CD_Audio_Buffer_L[CD_Audio_Buffer_Write_Pos] = Buf[tmp];
			CD_Audio_Buffer_R[CD_Audio_Buffer_Write_Pos] = Buf[tmp + 1];
			CD_Audio_Buffer_Write_Pos++;
			CD_Audio_Buffer_Write_Pos &= 0xFFF;
			pos_src += pas_src;
		}
	}
	else
	{
		while (length_dst > 0)
		{
			length_dst--;
			tmp = pos_src >> 16;
			CD_Audio_Buffer_L[CD_Audio_Buffer_Write_Pos] = Buf[tmp];
			CD_Audio_Buffer_R[CD_Audio_Buffer_Write_Pos] = Buf[tmp];
			CD_Audio_Buffer_Write_Pos++;
			CD_Audio_Buffer_Write_Pos &= 0xFFF;
			pos_src += pas_src;
		}
	}
	
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "Write Pos 2 = %d\n\n", CD_Audio_Buffer_Write_Pos);
#endif
}


void Update_CD_Audio(int **buf, int length)
{
	int *Buf_L, *Buf_R;
	int diff;
	
	Buf_L = buf[0];
	Buf_R = buf[1];
	
	if (CDD.Control & 0x0100)
		return;
	if (!(SCD.Status_CDC & 1))
		return;
	if (CD_Audio_Starting)
		return;
	
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "\n*********  Read Pos Normal = %d     ",
		CD_Audio_Buffer_Read_Pos);
#endif
	
	if (CD_Audio_Buffer_Write_Pos < CD_Audio_Buffer_Read_Pos)
		diff = CD_Audio_Buffer_Write_Pos + (4096) - CD_Audio_Buffer_Read_Pos;
	else
		diff = CD_Audio_Buffer_Write_Pos - CD_Audio_Buffer_Read_Pos;
	
	if (diff < 500)
		CD_Audio_Buffer_Read_Pos -= 2000;
	else if (diff > 3500)
		CD_Audio_Buffer_Read_Pos += 2000;
#ifdef DEBUG_CD
	else
		fprintf(debug_SCD_file, " pas de modifs   ");
#endif
	
	CD_Audio_Buffer_Read_Pos &= 0xFFF;
	
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "Read Pos = %d   ", CD_Audio_Buffer_Read_Pos);
#endif
	
	if (CDDA_Enable)
	{
		int i;
		for (length--, i = 0; length > 0; length--, i++)
		{
			Buf_L[i] += CD_Audio_Buffer_L[CD_Audio_Buffer_Read_Pos];
			Buf_R[i] += CD_Audio_Buffer_R[CD_Audio_Buffer_Read_Pos];
			CD_Audio_Buffer_Read_Pos++;
			CD_Audio_Buffer_Read_Pos &= 0xFFF;
		}
	}
	else
	{
		CD_Audio_Buffer_Read_Pos += length;
		CD_Audio_Buffer_Read_Pos &= 0xFFF;
	}
	
#ifdef DEBUG_CD
	fprintf(debug_SCD_file, "Read Pos 2 = %d\n\n", CD_Audio_Buffer_Read_Pos);
#endif
}
