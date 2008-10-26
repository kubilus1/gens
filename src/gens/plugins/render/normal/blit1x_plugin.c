/***************************************************************************
 * Gens: 1x renderer. (Plugin Data File)                                   *
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

#include "plugins/plugin.h"
#include <string.h>
#include <stdint.h>

#include "blit1x.hpp"

static GensPlugin_Desc_t GP_Desc =
{
	.name = "Normal Renderer",
	.author = "David Korth",
	.description = "Normal 1x renderer."
};

static GensPlugin_Render_t GP_Render =
{
	.blit = Blit1x,
	.scale = 1,
	.tag = "Normal"
};

GensPlugin_t GP_Render_1x =
{
	.interfaceVersion = GENSPLUGIN_VERSION,
	.pluginVersion = GP_VERSION(0, 0, 1),
	.type = GENSPLUGIN_RENDER,
	.desc = &GP_Desc,
	.plugin_t = (void*)&GP_Render
};
