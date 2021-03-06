/***************************************************************************
 * Gens: Old Print_Text() function.                                        *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville                       *
 * Copyright (c) 2003-2004 by Stéphane Akhoun                              *
 * Copyright (c) 2008-2010 by David Korth                                  *
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

#ifndef GENS_PRINT_TEXT_HPP
#define GENS_PRINT_TEXT_HPP

#ifdef __cplusplus
extern "C" {
#endif

void Print_Text(const char *str, int Pos_X, int Pos_Y, int Style);

void PrintF_Text(int Pos_X, int Pos_Y, int Style, const char *str, ...)
	__attribute__ ((format (printf, 4, 5)));

// Text style defines.
#define TEXT_EMU_MODE	0x01
#define TEXT_WHITE	0x00
#define TEXT_BLUE	0x02
#define TEXT_GREEN	0x04
#define TEXT_RED	0x06
#define TEXT_TRANS	0x08
#define TEXT_SIZE_X2	0x10

#ifdef __cplusplus
}
#endif

#endif /* GENS_PRINT_TEXT_HPP */
