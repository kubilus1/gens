/***************************************************************************
 * libgsft_w32u: Win32 Unicode Translation Layer.                          *
 * w32u_winnlsW.c: winnls.h translation. (Unicode version)                 *
 *                                                                         *
 * Copyright (c) 2009 by David Korth.                                      *
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

#include "w32u_winnlsW.h"
#include "w32u_winnls.h"

// C includes.
#include <stdlib.h>
#include <string.h>


static WINBASEAPI BOOL WINAPI GetCPInfoExUW(UINT CodePage, DWORD dwFlags, LPCPINFOEXA lpCPInfoEx)
{
	// TODO: Correct argument names.
	// TODO: Unicode conversion.
	return GetCPInfoExA(CodePage, dwFlags, lpCPInfoEx);
}


int WINAPI w32u_winnlsW_init(void)
{
	// TODO: Error handling.
	
	pGetCPInfoExU		= &GetCPInfoExUW;
	
	return 0;
}