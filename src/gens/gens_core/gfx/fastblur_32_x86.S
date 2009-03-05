/***************************************************************************
 * Gens: Fast Blur function. (32-bit color, x86 asm version.)              *
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

#if defined(__linux__) && defined(__ELF__)
	/* Mark the stack as non-executable on ELF. */
	.section .note.GNU-stack, "", %progbits
#endif

/* Symbol redefines for ELF. */
#if defined(__ELF__)
	#define _Fast_Blur_32_x86	Fast_Blur_32_x86
	#define _Fast_Blur_32_x86_mmx	Fast_Blur_32_x86_mmx
#endif

#define MD_SCREEN_SIZE	(336*240)
#define MASK_DIV2_32	$0x007F7F7F

/* External symbol redefines for ELF. */
#if defined(__ELF__)
	#define _MD_Screen32	MD_Screen32
	#define _bppMD		bppMD
#endif

/* External symbols. */
.extern _MD_Screen32
.extern _bppMD

#if defined(_WIN32)
.section .rdata
#else
.section .rodata
#endif

/* 64-bit mask used for the MMX fast blur function. */
MASK_DIV2_32_MMX:	.long	0x007F7F7F, 0x007F7F7F

.section .text

/*********************************************
 * void Fast_Blur_32_x86()                   *
 * 32-bit color Fast Blur function, non-MMX. *
 *********************************************/
.globl _Fast_Blur_32_x86
_Fast_Blur_32_x86:
	
	/* Save registers for later. */
	pushl	%ecx
	pushl	%edx
	pushl	%ebx
	pushl	%esi
	
	/* Start at the beginning of the actual display data. */
	movl	$(_MD_Screen32 + (8*4)), %esi
	movl	$(MD_SCREEN_SIZE - 8), %ecx
	xorl	%ebx, %ebx
	jmp	0f
	
.balign 16
0: /* .Loop */
		movl	(%esi), %eax		/* Get the current pixel. */
		shrl	$1, %eax		/* Reduce pixel brightness by 50%. */
		andl	MASK_DIV2_32, %eax	/* Apply color mask. */
		addl	%eax, %ebx		/* Blur the pixel with the previous pixel. */
		movl	%ebx, -4(%esi)		/* Write the new pixel. */
		movl	%eax, %ebx		/* Store the current pixel. */
		
		/* Next pixel. */
		addl	$4, %esi
		decl	%ecx
		jnz	0b
	
	/* Restore registers. */
	popl	%esi
	popl	%ebx
	popl	%edx
	popl	%ecx
	ret

.balign 64

/************************************************************
 * void Fast_Blur_32_x86_mmx()                              *
 * 32-bit color Fast Blur function, using MMX instructions. *
 ************************************************************/
.globl _Fast_Blur_32_x86_mmx
_Fast_Blur_32_x86_mmx:
	
	/* Save registers for later. */
	pushl	%ecx
	pushl	%ebx
	pushl	%esi
	
	/* Start at the beginning of the actual display data. */
	movl	$(_MD_Screen32 + (8*4)), %esi
	movl	$((MD_SCREEN_SIZE - 8) / 2), %ecx
	
	/* Load the 32-bit color mask. */
	movq	(MASK_DIV2_32_MMX), %mm7
	jmp	0f

.balign 16
0: /* .Loop */
		/* Get source pixels. */
		movq	 (%esi), %mm0
		movq	4(%esi), %mm1
		
		/* Blur source pixels. */
		psrld	$1, %mm0
		psrld	$1, %mm1
		pand	%mm7, %mm0
		pand	%mm7, %mm1
		paddd	%mm1, %mm0
		
		/* Put destination pixels. */
		movq	%mm0, (%esi)
		
		/* Next group of pixels. */
		addl	$8, %esi
		decl	%ecx
		jnz	0b
	
	/* Restore registers. */
	popl	%esi
	popl	%ebx
	popl	%ecx
	ret