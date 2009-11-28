/***************************************************************************
 * Gens: RAR Decompressor. (Win32)                                         *
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

#include "md_rar_win32.h"

#include "emulator/g_main.hpp"
#include "ui/gens_ui.hpp"

// Win32 includes.
#include "libgsft/w32u/w32u.h"
#include "libgsft/w32u/w32u_libc.h"
#include "libgsft/w32u/w32u_priv.h"
#include <winnls.h>

// C includes.
#include <unistd.h>
#include <stdlib.h>

// MDP includes.
#include "mdp/mdp_error.h"
#include "mdp/mdp_dlopen.h"

// libgsft includes.
#include "libgsft/gsft_strdup.h"
#include "libgsft/gsft_unused.h"


// RAR (Win32) decompressor functions.
static int decompressor_rar_win32_detect_format(FILE *zF);
static int decompressor_rar_win32_get_file_info(FILE *zF, const char* filename,
						mdp_z_entry_t** z_entry_out);
static size_t decompressor_rar_win32_get_file(FILE *zF, const char* filename,
						mdp_z_entry_t *z_entry,
						void *buf, const size_t size);

// RAR (Win32) decompressor struct.
const decompressor_t decompressor_rar_win32 =
{
	.detect_format	= decompressor_rar_win32_detect_format,
	.get_file_info	= decompressor_rar_win32_get_file_info,
	.get_file	= decompressor_rar_win32_get_file
};


// RAR load state.
typedef struct _RarState_t
{
	uint8_t	*buf;	// Buffer.
	size_t	size;	// Size of buffer.
	size_t	pos;	// Current position.
} RarState_t;

// UnRAR.dll
#include "unrar.h"
static void *hUnrarDll = NULL;
static int unrar_refcnt = 0;
MAKE_STFUNCPTR(RAROpenArchiveEx);
MAKE_STFUNCPTR(RARCloseArchive);
MAKE_STFUNCPTR(RARReadHeaderEx);
MAKE_STFUNCPTR(RARProcessFile);
MAKE_STFUNCPTR(RARSetCallback);
MAKE_STFUNCPTR(RARGetDllVersion);

#define InitFuncPtr_unrar(hDll, fn) p##fn = (typeof(p##fn))mdp_dlsym((hDll), #fn)

static int unrar_dll_init(void);
static int unrar_dll_end(void);

/**
 * unrar_dll_init(): Initialize UnRAR.dll.
 * @return 0 on success; non-zero on error.
 */
static int unrar_dll_init(void)
{
	if (unrar_refcnt++ != 0)
		return 0;
	
	// Load the DLL.
	pSetCurrentDirectoryU(PathNames.Gens_EXE_Path);
	hUnrarDll = mdp_dlopen(Misc_Filenames.RAR_Binary);
	if (!hUnrarDll)
	{
		// DLL could not be loaded.
		// TODO: Come up with a new MDP error code for "DLL not found."
		return -MDP_ERR_Z_EXE_NOT_FOUND;
	}
	
	// Load the function pointers.
	InitFuncPtr_unrar(hUnrarDll, RAROpenArchiveEx);
	InitFuncPtr_unrar(hUnrarDll, RARCloseArchive);
	InitFuncPtr_unrar(hUnrarDll, RARReadHeaderEx);
	InitFuncPtr_unrar(hUnrarDll, RARProcessFile);
	InitFuncPtr_unrar(hUnrarDll, RARSetCallback);
	InitFuncPtr_unrar(hUnrarDll, RARGetDllVersion);
	
	// Check if any of the function pointers are NULL.
	if (!pRAROpenArchiveEx || !pRARCloseArchive ||
	    !pRARReadHeaderEx  || !pRARProcessFile ||
	    !pRARSetCallback   || !pRARGetDllVersion)
	{
		// NULL pointers found. That's bad.
		// TODO: Come up with a new MDP error code for "Incorrect DLL."
		unrar_dll_end();
		return -MDP_ERR_Z_EXE_NOT_FOUND;
	}
	
	// UnRAR.dll loaded successfully.
	return 0;
}

/**
 * unrar_dll_end(): Shut down UnRAR.dll.
 * @return 0 on success; non-zero on error.
 */
static int unrar_dll_end(void)
{
	if (unrar_refcnt <= 0)
		return 0;
	
	unrar_refcnt--;
	if (unrar_refcnt != 0)
		return 0;
	
	// Unload the DLL.
	mdp_dlclose(hUnrarDll);
	hUnrarDll = NULL;
	
	// Clear the function pointers.
	pRAROpenArchiveEx	= NULL;
	pRARCloseArchive	= NULL;
	pRARReadHeaderEx	= NULL;
	pRARProcessFile		= NULL;
	pRARSetCallback		= NULL;
	pRARGetDllVersion	= NULL;
	
	return 0;
}


/**
 * decompressor_rar_win32_detect_format(): Detect if this file can be handled by this decompressor.
 * @param zF Open file handle.
 * @return Non-zero if this file can be handled; 0 if it can't be.
 */
int decompressor_rar_win32_detect_format(FILE *zF)
{
	// Magic Number for RAR:
	// First four bytes: "Rar!"
	static const unsigned char rar_magic[] = {'R', 'a', 'r', '!'};
	
	unsigned char buf[sizeof(rar_magic)];
	fseek(zF, 0, SEEK_SET);
	fread(buf, sizeof(unsigned char), sizeof(rar_magic), zF);
	return (memcmp(buf, rar_magic, sizeof(rar_magic)) == 0);
}


/**
 * decompressor_rar_win32_get_file_info(): Get information about all files in the archive.
 * @param zF		[in] Open file handle.
 * @param filename	[in] Filename of the archive.
 * @param z_entry_out	[out] Pointer to mdp_z_entry_t*, which will contain an allocated mdp_z_entry_t.
 * @return MDP error code.
 */
int decompressor_rar_win32_get_file_info(FILE *zF, const char* filename, mdp_z_entry_t** z_entry_out)
{
	// Initialize UnRAR.dll.
	int ret = unrar_dll_init();
	if (ret != 0)
	{
		// Error initializing UnRAR.dll.
		return ret;
	}
	
	HANDLE hRar;
	char *filenameA = NULL;
	wchar_t *filenameW = NULL;
	BOOL doUnicode = (isSendMessageUnicode && pMultiByteToWideChar && pWideCharToMultiByte);
	
	// Open the RAR file.
	struct RAROpenArchiveDataEx rar_open;
	rar_open.OpenMode = RAR_OM_LIST;
	rar_open.CmtBuf = NULL;
	rar_open.CmtBufSize = 0;
	
	if (doUnicode)
	{
		// Unicode mode.
		filenameW = w32u_mbstowcs(filename);
		rar_open.ArcName = NULL;
		rar_open.ArcNameW = filenameW;
	}
	else
	{
		// ANSI mode.
		filenameA = strdup(filename);
		rar_open.ArcName = filenameA;
		rar_open.ArcNameW = NULL;
	}
	
	hRar = pRAROpenArchiveEx(&rar_open);
	if (!hRar)
	{
		// Error opening the RAR file.
		free(filenameW);
		return -MDP_ERR_Z_CANT_OPEN_ARCHIVE;
	}
	
	// File list pointers.
	mdp_z_entry_t *z_entry_head = NULL;
	mdp_z_entry_t *z_entry_end = NULL;
	
	// Process the archive.
	struct RARHeaderDataEx rar_header;
	char utf8_buf[1024*4];
	while ((ret = pRARReadHeaderEx(hRar, &rar_header)) == 0)
	{
		// Allocate memory for the file.
		mdp_z_entry_t *z_entry_cur = (mdp_z_entry_t*)malloc(sizeof(*z_entry_cur));
		
		// Store the ROM file information.
		// TODO: What do we do if rar_header.UnpSizeHigh is set (indicating >4 GB)?
		z_entry_cur->filesize = rar_header.UnpSize;
		z_entry_cur->next = NULL;
		
		// Check which filename should be used.
		if (doUnicode)
		{
			// Use the Unicode filename. (rar_header.FileNameW)
			pWideCharToMultiByte(CP_UTF8, 0, rar_header.FileNameW, -1,
						utf8_buf, sizeof(utf8_buf), NULL, NULL);
			z_entry_cur->filename = strdup(utf8_buf);
		}
		else
		{
			// Use the ANSI filename. (rar_header.FileName)
			z_entry_cur->filename = strdup(rar_header.FileName);
		}
		
		if (!z_entry_head)
		{
			// List hasn't been created yet. Create it.
			z_entry_head = z_entry_cur;
			z_entry_end = z_entry_cur;
		}
		else
		{
			// Append the file list entry to the end of the list.
			z_entry_end->next = z_entry_cur;
			z_entry_end = z_entry_cur;
		}
		
		// Go to the next file.
		if (pRARProcessFile(hRar, RAR_SKIP, NULL, NULL) != 0)
			break;
	}
	
	// Close the RAR file.
	pRARCloseArchive(hRar);
	free(filenameA);
	free(filenameW);
	
	// Shut down UnRAR.dll.
	unrar_dll_end();
	
	// Return the list of files.
	*z_entry_out = z_entry_head;
	return MDP_ERR_OK;
}


/**
 * decompressor_rar_win32_callback(): UnRAR.dll callback function.
 * @param msg
 * @param UserData
 * @param P1
 * @param P2
 */
static int CALLBACK decompressor_rar_win32_callback(UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2)
{
	switch (msg)
	{
		default:
		case UCM_CHANGEVOLUME:
		case UCM_NEEDPASSWORD:
			// Unhandled message.
			// TODO: Support at least UCM_NEEDPASSWORD.
			return -1;
		
		case UCM_PROCESSDATA:
		{
			// Process data.
			RarState_t *pRarState = (RarState_t*)UserData;
			
			const uint8_t *buf = (const uint8_t*)P1;
			size_t size = (size_t)P2;
			
			if ((pRarState->pos + size) > pRarState->size)
			{
				// Overflow!
				size_t size_diff = (pRarState->size - pRarState->pos);
				memcpy(&pRarState->buf[pRarState->pos], buf, size_diff);
				pRarState->pos += size_diff;
				return -1;
			}
			
			// Copy the data.
			memcpy(&pRarState->buf[pRarState->pos], buf, size);
			pRarState->pos += size;
			break;
		}
	}
	
	return 0;
}


/**
 * decompressor_rar_win32_get_file(): Get a file from the archive.
 * @param zF Open file handle. (Unused in the RAR handler.)
 * @param filename Filename of the archive.
 * @param z_entry Pointer to mdp_z_entry_t element to get from the archive.
 * @param buf Buffer to read the file into.
 * @param size Size of buf (in bytes).
 * @return Number of bytes read, or 0 on error.
 */
size_t decompressor_rar_win32_get_file(FILE *zF, const char *filename,
					mdp_z_entry_t *z_entry,
					void *buf, const size_t size)
{
	int ret = unrar_dll_init();
	if (ret != 0)
	{
		// Error initializing UnRAR.dll.
		return ret;
	}
	
	HANDLE hRar;
	char *filenameA = NULL;
	wchar_t *filenameW = NULL;
	BOOL doUnicode = (isSendMessageUnicode && pMultiByteToWideChar && pWideCharToMultiByte && p_wcsicmp);
	
	// Open the RAR file.
	struct RAROpenArchiveDataEx rar_open;
	rar_open.OpenMode = RAR_OM_EXTRACT;
	rar_open.CmtBuf = NULL;
	rar_open.CmtBufSize = 0;
	
	if (doUnicode)
	{
		// Unicode mode.
		filenameW = w32u_mbstowcs(filename);
		rar_open.ArcName = NULL;
		rar_open.ArcNameW = filenameW;
	}
	else
	{
		// ANSI mode.
		filenameA = strdup(filename);
		rar_open.ArcName = filenameA;
		rar_open.ArcNameW = NULL;
	}
	
	hRar = pRAROpenArchiveEx(&rar_open);
	if (!hRar)
	{
		// Error opening the RAR file.
		free(filenameW);
		return -MDP_ERR_Z_CANT_OPEN_ARCHIVE;
	}
	
	// If we're using Unicode, convert the selected filename to Unicode.
	wchar_t *z_filenameW = NULL;
	if (doUnicode)
		z_filenameW = w32u_mbstowcs(z_entry->filename);
	
	// Search for the file.
	struct RARHeaderDataEx rar_header;
	size_t success = 0;	// 0 == not successful; positive == size read
	int cmp;
	while ((ret = pRARReadHeaderEx(hRar, &rar_header)) == 0)
	{
		if (doUnicode)
		{
			// Unicode mode.
			cmp = p_wcsicmp(z_filenameW, rar_header.FileNameW);
		}
		else
		{
			// ANSI mode.
			cmp = _stricmp(z_entry->filename, rar_header.FileName);
		}
		
		if (cmp != 0)
		{
			// Not a match. Skip the file.
			if (pRARProcessFile(hRar, RAR_SKIP, NULL, NULL) != 0)
				break;
			continue;
		}
		
		// Found the file.
		
		// Create the RAR state.
		RarState_t rar_state;
		rar_state.buf = (uint8_t*)buf;
		rar_state.size = size;
		rar_state.pos = 0;
		
		// Set up the RAR callback.
		pRARSetCallback(hRar, &decompressor_rar_win32_callback, (LPARAM)&rar_state);
		
		// Process the file.
		// Possible errors:
		// - 0: Success.
		// - ERAR_UNKNOWN: Read the maximum amount of data for the ubuffer.
		// - Others: Read error; abort. (TODO: Show an error message.)
		ret = pRARProcessFile(hRar, RAR_TEST, NULL, NULL);
		// TODO: md_rar.cpp returns the filesize processed on error.
		// This just returns 0.
		if (ret != 0 && ret != ERAR_UNKNOWN)
			break;
		
		// File processed.
		success = rar_state.pos;
		break;
	}
	
	// Close the RAR file.
	pRARCloseArchive(hRar);
	free(filenameA);
	free(filenameW);
	free(z_filenameW);
	
	// Shut down UnRAR.dll.
	unrar_dll_end();
	
	return success;
}
