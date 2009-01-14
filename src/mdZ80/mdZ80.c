/***************************************************************************
 * mdZ80: Gens Z80 Emulator                                                *
 * mdZ80.c: Main Z80 emulation functions.                                  *
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

#include "mdZ80.h"

// C includes.
#include <string.h>


#define Z80_RUNNING	0x01
#define Z80_HALTED	0x02
#define Z80_FAULTED	0x10


/**
 * z80_Get_PC(): Get the Z80 program counter.
 * @param z80 Pointer to Z80 context.
 * @return Z80 program counter, or -1 (0xFFFFFFFF) if the Z80 is running.
 */
unsigned int z80_Get_PC(Z80_CONTEXT *z80)
{
	if (z80->Status & Z80_RUNNING)
		return -1;
	
	// Subtract the BasePC from PC to get the actual Z80 program counter.
	return (z80->PC.d - z80->BasePC);
}


/**
 * z80_Set_PC(): Set the Z80 program counter.
 * @param z80 Pointer to Z80 context.
 * @param PC New program counter.
 */
void z80_Set_PC(Z80_CONTEXT *z80, unsigned int PC)
{
	if (z80->Status & Z80_RUNNING)
		return;
	
	// TODO: 32-bit specific code will break on 64-bit!
	PC &= 0xFFFF;
	unsigned int newPC = (unsigned int)(z80->Fetch[PC >> 8]);
	z80->BasePC = newPC;
	z80->PC.d = newPC + PC;
}


/**
 * z80_Reset(): Reset the Z80 CPU.
 * @param z80 Pointer to Z80 context.
 */
void z80_Reset(Z80_CONTEXT *z80)
{
	// Save the Z80 Cycle Count.
	unsigned int cycleCnt = z80->CycleCnt;
	
	// Clear the Z80 struct up to CycleSup.
	memset(z80, 0x00, 23*4);
	
	// Restore the Z80 Cycle Count.
	z80->CycleCnt = cycleCnt;
	
	// Initialize the program counter.
	z80_Set_PC(z80, 0);
	
	// Initialize the index and flag registers.
	z80->IX.d = 0xFFFF;
	z80->IY.d = 0xFFFF;
	z80->AF.d = 0x4000;
}
