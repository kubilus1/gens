/***************************************************************************
 * Gens: [MDP] hq2x renderer.                                              *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville                       *
 * Copyright (c) 2003-2004 by Stéphane Akhoun                              *
 * Copyright (c) 2008 by David Korth                                       *
 * hq2x Copyright (c) 2003 by Maxim Stepin                                 *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mdp_render_hq2x.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// CPU flags
#include "mdp/mdp_cpuflags.h"

// x86 asm versions
#ifdef GENS_X86_ASM
#include "mdp_render_hq2x_x86.h"
#endif /* GENS_X86_ASM */

// MDP Host Services
static MDP_Host_t *mdp_render_hq2x_hostSrv;
int *mdp_render_hq2x_LUT16to32 = NULL;
int *mdp_render_hq2x_RGB16toYUV = NULL;


// TODO: Proper 15-bit color support.
// It "works' right now in 15-bit, but the output is different than with 16-bit.
// My first attempt at 15-bit support resulted in massive failure. :(


/**
 * mdp_render_hq2x_end(): Initialize the hq2x plugin.
 */
void MDP_FNCALL mdp_render_hq2x_init(MDP_Host_t *hostSrv)
{
	// Save the MDP Host Services pointer.
	mdp_render_hq2x_hostSrv = hostSrv;
}


/**
 * mdp_render_hq2x_end(): Shut down the hq2x plugin.
 */
void MDP_FNCALL mdp_render_hq2x_end(void)
{
	// If the LUT16to32 pointer was referenced, unreference it.
	if (mdp_render_hq2x_LUT16to32)
	{
		mdp_render_hq2x_hostSrv->unrefPtr(MDP_PTR_LUT16to32);
		mdp_render_hq2x_LUT16to32 = NULL;
	}
	
	// If the RGB16toYUV pointer was referenced, unreference it.
	if (mdp_render_hq2x_RGB16toYUV)
	{
		mdp_render_hq2x_hostSrv->unrefPtr(MDP_PTR_RGB16toYUV);
		mdp_render_hq2x_RGB16toYUV = NULL;
	}
}


/**
 * mdp_render_hq2x_cpp(): hq2x rendering function.
 * @param renderInfo Render information.
 */
void MDP_FNCALL mdp_render_hq2x_cpp(MDP_Render_Info_t *renderInfo)
{
	if (!renderInfo)
		return;
	
	// Make sure the lookup tables are initialized.
	if (!mdp_render_hq2x_LUT16to32)
		mdp_render_hq2x_LUT16to32 = (int*)(mdp_render_hq2x_hostSrv->refPtr(MDP_PTR_LUT16to32));
	if (!mdp_render_hq2x_RGB16toYUV)
		mdp_render_hq2x_RGB16toYUV = (int*)(mdp_render_hq2x_hostSrv->refPtr(MDP_PTR_RGB16toYUV));
	
#ifdef GENS_X86_ASM
	if (renderInfo->cpuFlags & MDP_CPUFLAG_MMX)
	{
		mdp_render_hq2x_16_x86_mmx(
			    (uint16_t*)renderInfo->destScreen,
			    (uint16_t*)renderInfo->mdScreen,
			    renderInfo->destPitch, renderInfo->srcPitch,
			    renderInfo->width, renderInfo->height);
	}
#else /* !GENS_X86_ASM */
	T_mdp_render_hq2x_cpp(
		    (uint16_t*)renderInfo->destScreen,
		    (uint16_t*)renderInfo->mdScreen,
		    renderInfo->destPitch, renderInfo->srcPitch,
		    renderInfo->width, renderInfo->height);
#endif /* GENS_X86_ASM */
}
