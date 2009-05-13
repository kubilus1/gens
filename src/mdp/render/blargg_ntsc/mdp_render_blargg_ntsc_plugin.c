/***************************************************************************
 * MDP: Blargg's NTSC renderer. (Plugin Data File)                         *
 *                                                                         *
 * Copyright (c) 2006 by Shay Green                                        *
 * MDP version Copyright (c) 2008-2009 by David Korth                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU Lesser General Public License as published   *
 * by the Free Software Foundation; either version 2.1 of the License, or  *
 * (at your option) any later version.                                     *
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

#include "mdp_render_blargg_ntsc.h"
#include "md_ntsc.hpp"

#include "mdp/mdp.h"
#include "mdp/mdp_cpuflags.h"

static mdp_desc_t mdp_desc =
{
	.name = "Blargg's NTSC Renderer",
	.author_mdp = "David Korth",
	.author_orig = "Shay Green",
	.description = "Blargg's NTSC Renderer for the Sega Genesis.",
	.website = "http://www.fly.net/~ant/libs/ntsc.html",
	.license = MDP_LICENSE_LGPL_21,
	
	.icon = NULL,
	.iconLength = 0
};

mdp_render_t mdp_render =
{
	.interfaceVersion = MDP_RENDER_INTERFACE_VERSION,
	.scale = 2,
	
	.blit = mdp_md_ntsc_blit,
	.tag = "Blargg NTSC",
	
	.flags = MDP_RENDER_FLAG_RGB565
};

static mdp_func_t mdp_func =
{
	.init = mdp_render_blargg_ntsc_init,
	.end = mdp_render_blargg_ntsc_end
};

mdp_t mdp =
{
	// Plugin version information.
	.interfaceVersion = MDP_INTERFACE_VERSION,
	.pluginVersion = MDP_VERSION(0, 1, 2),
	
	// CPU flags.
	.cpuFlagsSupported = 0,
	.cpuFlagsRequired = 0,
	
	// UUID: 78a37c47-ecbe-41a4-b446-732a84899840
	.uuid = {0x78, 0xA3, 0x7C, 0x47,
		 0xEC, 0xBE,
		 0x41, 0xA4,
		 0xB4, 0x46,
		 0x73, 0x2A, 0x84, 0x89, 0x98, 0x40},
	
	// Description.
	.desc = &mdp_desc,
	
	// Functions.
	.func = &mdp_func
};
