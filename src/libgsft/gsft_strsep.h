/*******************************************************************************
 * libgsft: Common functions.                                                  *
 * gsft_strsep.h: strsep() implementation for older systems.                   *
 *                                                                             *
 * MDP implementation Copyright (c) 2008-2009 by David Korth                   *
 *                                                                             *
 * Original implementation derived from the GNU C Library.                     *
 * Copyright (C) 1992, 93, 96, 97, 98, 99, 2004 Free Software Foundation, Inc. *
 *                                                                             *
 * This program is free software; you can redistribute it and/or modify it     *
 * under the terms of the GNU General Public License as published by the       *
 * Free Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                                  *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but         *
 * WITHOUT ANY WARRANTY; without even the implied warranty of                  *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU General Public License for more details.                                *
 *                                                                             *
 * You should have received a copy of the GNU General Public License along     *
 * with this program; if not, write to the Free Software Foundation, Inc.,     *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.               *
 *******************************************************************************/

#ifndef __GSFT_STRSEP_H
#define __GSFT_STRSEP_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRSEP

/* strsep() is defined. */
#include <string.h>

#else /* !HAVE_STRSEP */

/* strsep() is not defined. */

#ifdef __cplusplus
extern "C" {
#endif

char *strsep(char **stringp, const char *delim);

#ifdef __cplusplus
}
#endif

#endif /* HAVE_STRSEP */

#endif /* __GSFT_STRTOK_R_H */