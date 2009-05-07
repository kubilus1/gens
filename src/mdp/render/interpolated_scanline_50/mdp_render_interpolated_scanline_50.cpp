/***************************************************************************
 * MDP: Interpolated 50% Scanline renderer.                                *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mdp_render_interpolated_scanline_50.hpp"
#include "mdp_render_interpolated_scanline_50_plugin.h"

// MDP includes.
#include "mdp/mdp_stdint.h"
#include "mdp/mdp_cpuflags.h"
#include "mdp/mdp_error.h"

// Mask constants.
#define MASK_DIV2_15		((uint16_t)(0x3DEF))
#define MASK_DIV2_16		((uint16_t)(0x7BEF))
#define MASK_DIV2_32		((uint32_t)(0x7F7F7F7F))

#define BLEND(a, b, mask) ((((a) >> 1) & mask) + (((b) >> 1) & mask))

// MDP Host Services.
static mdp_host_t *mdp_render_interpolated_scanline_50_host_srv = NULL;


/**
 * mdp_render_interpolated_scanline_50_init(): Initialize the Interpolated 50% Scanline rendering plugin.
 * @return MDP error code.
 */
int MDP_FNCALL mdp_render_interpolated_scanline_50_init(mdp_host_t *host_srv)
{
	// Save the MDP Host Services pointer.
	mdp_render_interpolated_scanline_50_host_srv = host_srv;
	
	// Register the renderer.
	return mdp_render_interpolated_scanline_50_host_srv->renderer_register(&mdp, &mdp_render);
}


/**
 * mdp_render_interpolated_scanline_50_end(): Shut down the Interpolated 50% Scanline rendering plugin.
 * @return MDP error code.
 */
int MDP_FNCALL mdp_render_interpolated_scanline_50_end(void)
{
	if (!mdp_render_interpolated_scanline_50_host_srv)
		return MDP_ERR_OK;
	
	// Unregister the renderer.
	mdp_render_interpolated_scanline_50_host_srv->renderer_unregister(&mdp, &mdp_render);
	
	// Plugin is shut down.
	return MDP_ERR_OK;
}


/**
 * T_mdp_render_interpolated_scanline_50_cpp: Blits the image to the screen, 2x size, interpolation with 50% scanlines.
 * @param destScreen Pointer to the destination screen buffer.
 * @param mdScreen Pointer to the MD screen buffer.
 * @param destPitch Pitch of destScreen.
 * @param srcPitch Pitch of mdScreen.
 * @param width Width of the image.
 * @param height Height of the image.
 * @param mask Mask for the interpolation data.
 */
template<typename pixel>
static inline void T_mdp_render_interpolated_scanline_50_cpp(pixel *destScreen, pixel *mdScreen,
							     int destPitch, int srcPitch,
							     int width, int height, pixel mask)
{
	destPitch /= sizeof(pixel);
	srcPitch /= sizeof(pixel);
	
	// TODO: Figure out why the interpolation function is using the line
	// below the source line instead of using the current source line.
	for (int y = 0; y < height; y++)
	{
		pixel *SrcLine = &mdScreen[y * srcPitch];
		pixel *DstLine1 = &destScreen[(y * 2) * destPitch];
		pixel *DstLine2 = &destScreen[((y * 2) + 1) * destPitch];
		
		for (int x = 0; x < width; x++)
		{
			pixel C = *(SrcLine);
			pixel R = *(SrcLine + 1);
			pixel D = *(SrcLine + srcPitch);
			pixel DR = *(SrcLine + srcPitch + 1);
			
			*DstLine1++ = C;
			*DstLine1++ = BLEND(C, R, mask);
			*DstLine2++ = (BLEND(C, D, mask) >> 1) & mask;
			*DstLine2++ = ((BLEND(BLEND(C, R, mask), BLEND(D, DR, mask), mask)) >> 1) & mask;
			
			SrcLine++;
		}
	}
}


int MDP_FNCALL mdp_render_interpolated_scanline_50_cpp(mdp_render_info_t *render_info)
{
	if (!render_info)
		return -MDP_ERR_RENDER_INVALID_RENDERINFO;
	
	if ((render_info->vmodeFlags & MDP_RENDER_VMODE_BPP) == MDP_RENDER_VMODE_BPP_16)
	{
		T_mdp_render_interpolated_scanline_50_cpp(
			    (uint16_t*)render_info->destScreen,
			    (uint16_t*)render_info->mdScreen,
			    render_info->destPitch, render_info->srcPitch,
			    render_info->width, render_info->height,
			    ((render_info->vmodeFlags & MDP_RENDER_VMODE_RGB_MODE) ? MASK_DIV2_16 : MASK_DIV2_15));
	}
	else
	{
		T_mdp_render_interpolated_scanline_50_cpp(
			    (uint32_t*)render_info->destScreen,
			    (uint32_t*)render_info->mdScreen,
			    render_info->destPitch, render_info->srcPitch,
			    render_info->width, render_info->height,
			    MASK_DIV2_32);
	}
	
	return MDP_ERR_OK;
}
