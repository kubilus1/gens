/***************************************************************************
 * Gens: Video Drawing - Text Drawing.                                     *
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

#ifndef GENS_VDRAW_TEXT_HPP
#define GENS_VDRAW_TEXT_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "macros/bool_m.h"

// Constants for MsgStyle and FpsStyle.
// These match the #define's in gens_core/misc/misc.h.
#define STYLE_EMU_MODE		0x01
#define STYLE_COLOR_WHITE	0x00
#define STYLE_COLOR_BLUE	0x02
#define STYLE_COLOR_GREEN	0x04
#define STYLE_COLOR_RED		0x06
#define STYLE_TRANSPARENT	0x08
#define STYLE_DOUBLESIZE	0x10

// VDraw styles.
typedef struct
{
	uint8_t		style;
	uint8_t		color;		// STYLE_COLOR_* values
	uint32_t	dot_color;	// Actual RGB color value.
	BOOL		double_size;
	BOOL		transparent;
} vdraw_style_t;

void draw_text(void *screen, const int fullW, const int w, const int h,
	       const char *msg, const vdraw_style_t *style,
	       const BOOL adjustForScreenSize);

void calc_text_style(vdraw_style_t *style);
void calc_transparency_mask(void);

#ifdef __cplusplus
}
#endif

#endif /* GENS_VDRAW_TEXT_HPP */