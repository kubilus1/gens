/*****************************************************************************
 * Gens: [MDP] Super Eagle renderer. (Plugin Data File)                      *
 *                                                                           *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville                         *
 * Copyright (c) 2003-2004 by Stéphane Akhoun                                *
 * Copyright (c) 2008 by David Korth                                         *
 * Super Eagle Copyright (c) by Derek Liauw Kie Fa and Robert J. Ohannessian *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify it   *
 * under the terms of the GNU Lesser General Public License as published     *
 * by the Free Software Foundation; either version 2.1 of the License, or    *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty of                *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License along   *
 * with this program; if not, write to the Free Software Foundation, Inc.,   *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 *****************************************************************************/

#include <stdint.h>
#include <string.h>

#include "mdp/mdp.h"
#include "mdp/mdp_cpuflags.h"

#include "mdp_render_super_eagle.h"

static MDP_Desc_t MDP_Desc =
{
	.name = "Super Eagle Renderer",
	.author_mdp = "David Korth",
	.author_orig = "Derek Liauw Kie Fa and Robert J. Ohannessian",
	.description = "Super Eagle renderer.",
	.website = NULL,
	.license = MDP_LICENSE_LGPL_21
};

MDP_Render_t mdp_render_t =
{
	.interfaceVersion = MDP_RENDER_INTERFACE_VERSION,
	.blit = mdp_render_super_eagle_cpp,
	.scale = 2,
	.flags = MDP_RENDER_FLAG_SRC16DST32,
	.tag = "Super Eagle"
};

static MDP_Func_t MDP_Func =
{
	.init = mdp_render_super_eagle_init,
	.end = mdp_render_super_eagle_end
};

MDP_t mdp =
{
	// Plugin version information.
	.interfaceVersion = MDP_INTERFACE_VERSION,
	.pluginVersion = MDP_VERSION(1, 0, 0),
	
	// CPU flags.
	.cpuFlagsSupported = MDP_CPUFLAG_MMX,
	.cpuFlagsRequired = MDP_CPUFLAG_MMX,
	
	// UUID: d26bcd88-7906-48e2-86c5-c74558ae4462
	.uuid = {0xD2, 0x6B, 0xCD, 0x88,
		 0x79, 0x06,
		 0x48, 0xE2,
		 0x86, 0xC5,
		 0xC7, 0x45, 0x58, 0xAE, 0x44, 0x62},
	
	// Description.
	.desc = &MDP_Desc,
	
	// Functions.
	.func = &MDP_Func
};