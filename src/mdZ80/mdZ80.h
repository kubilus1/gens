/**********************************************************/
/*                                                        */
/* Z80 emulator 0.99                                      */
/* Copyright 2002 Stéphane Dallongeville                  */
/* Used for the genesis emulation in Gens                 */
/*                                                        */
/**********************************************************/

#ifndef __MDZ80_H
#define __MDZ80_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************/
/* Compiler dependant defines */
/******************************/

#ifndef UINT8
#define UINT8   unsigned char
#endif

#ifndef INT8
#define INT8    signed char
#endif

#ifndef UINT16
#define UINT16  unsigned short
#endif

#ifndef INT16
#define INT16   signed short
#endif

#ifndef UINT32
#define UINT32  unsigned int
#endif

#ifndef INT32
#define INT32   signed int
#endif

#ifdef _WIN32
 	#define FASTCALL	__fastcall
#else
	#define __fastcall	__attribute__ ((regparm(2)))
	#define FASTCALL	__attribute__ ((regparm(2)))
#endif


/****************************/
/* Structures & definitions */
/****************************/


typedef UINT8 FASTCALL Z80_RB(UINT32 adr);
typedef UINT16 FASTCALL Z80_RW(UINT32 adr);
typedef void FASTCALL Z80_WB(UINT32 adr, UINT8 data);
typedef void FASTCALL Z80_WW(UINT32 adr, UINT16 data);


struct _Z80_context
{
	union
	{
		struct
		{
			UINT8 A;
			UINT8 F;
			UINT8 x;
			UINT8 FXY;
		} b;
		struct
		{
			UINT16 AF;
			UINT16 FXYW;
		} w;
		UINT32 d;
	} AF;
	union
	{
		struct
		{
			UINT8 C;
			UINT8 B;
			UINT16 x;
		} b;
		struct
		{
			UINT16 BC;
			UINT16 x;
		} w;
		UINT32 d;
	} BC;
	union
	{
		struct
		{
			UINT8 E;
			UINT8 D;
			UINT16 x;
		} b;
		struct
		{
			UINT16 DE;
			UINT16 x;
		} w;
		UINT32 d;
	} DE;
	union
	{
		struct
		{
			UINT8 L;
			UINT8 H;
			UINT16 x;
		} b;
		struct
		{
			UINT16 HL;
			UINT16 x;
		} w;
		UINT32 d;
	} HL;
	union
	{
		struct
		{
			UINT8 IXL;
			UINT8 IXH;
			UINT16 x;
		} b;
		struct
		{
			UINT16 IX;
			UINT16 x;
		} w;
		UINT32 d;
	} IX;
	union
	{
		struct
		{
			UINT8 IYL;
			UINT8 IYH;
			UINT16 x;
		} b;
		struct
		{
			UINT16 IY;
			UINT16 x;
		} w;
		UINT32 d;
	} IY;
	union
	{
		struct
		{
			UINT8 PCL;
			UINT8 PCH;
			UINT16 x;
		} b;
		struct
		{
			UINT16 PC;
			UINT16 x;
		} w;
		UINT32 d;
	} PC;	// PC == BasePC + Z80 PC
	union
	{
		struct
		{
			UINT8 SPL;
			UINT8 SPH;
			UINT16 x;
		} b;
		struct
		{
			UINT16 SP;
			UINT16 x;
		} w;
		UINT32 d;
	} SP;
	union
	{
		struct
		{
			UINT8 A2;
			UINT8 F2;
			UINT8 x;
			UINT8 FXY2;
		} b;
		struct
		{
			UINT16 AF2;
			UINT16 FXYW2;
		} w;
		UINT32 d;
	} AF2;
	union
	{
		struct
		{
			UINT8 C2;
			UINT8 B2;
			UINT16 x;
		} b;
		struct
		{
			UINT16 BC2;
			UINT16 x;
		} w;
		UINT32 d;
	} BC2;
	union
	{
		struct
		{
			UINT8 E2;
			UINT8 D2;
			UINT16 x;
		} b;
		struct
		{
			UINT16 DE2;
			UINT16 x;
		} w;
		UINT32 d;
	} DE2;
	union
	{
		struct
		{
			UINT8 L2;
			UINT8 H2;
			UINT16 x;
		} b;
		struct
		{
			UINT16 HL2;
			UINT16 x;
		} w;
		UINT32 d;
	} HL2;
	union
	{
		struct
		{
			UINT8 IFF1;
			UINT8 IFF2;
			UINT16 x;
		} b;
		struct
		{
			UINT16 IFF;
			UINT16 x;
		} w;
		UINT32 d;
	} IFF;
	union
	{
		struct
		{
			UINT8 R1;
			UINT8 R2;
			UINT16 x;
		} b;
		struct
		{
			UINT16 R;
			UINT16 x;
		} w;
		UINT32 d;
	} R;

	UINT8 I;
	UINT8 IM;
	UINT8 IntVect;
	UINT8 IntLine;

	UINT32 Status;
	UINT32 BasePC;	// Pointer to x86 memory location where Z80 RAM starts.
	UINT32 TmpSav0;
	UINT32 TmpSav1;

	UINT32 CycleCnt;
	UINT32 CycleTD;
	UINT32 CycleIO;
	UINT32 CycleSup;

	Z80_RB *ReadB[0x100];
	Z80_WB *WriteB[0x100];
	UINT8 *Fetch[0x100];
	Z80_RW *ReadW[0x100];
	Z80_WW *WriteW[0x100];

	Z80_RB *IN_C;
	Z80_WB *OUT_C;
	UINT32 RetIC;
	UINT32 IntAckC;
};

typedef struct _Z80_context Z80_CONTEXT;


/*************************/
/* Publics Z80 variables */
/*************************/


extern Z80_CONTEXT M_Z80;


/**
 * Z80 functions (asm)
 */


//UINT32 FASTCALL z80_Init(Z80_CONTEXT *z80);
//UINT32 FASTCALL z80_Reset(Z80_CONTEXT *z80);

UINT32 z80_Add_ReadB(Z80_CONTEXT *z80, UINT32 low_adr, UINT32 high_adr, Z80_RB *Func);
UINT32 z80_Add_ReadW(Z80_CONTEXT *z80, UINT32 low_adr, UINT32 high_adr, Z80_RW *Func);
UINT32 z80_Add_WriteB(Z80_CONTEXT *z80, UINT32 low_adr, UINT32 high_adr, Z80_WB *Func);
UINT32 z80_Add_WriteW(Z80_CONTEXT *z80, UINT32 low_adr, UINT32 high_adr, Z80_WW *Func);
UINT32 z80_Add_Fetch(Z80_CONTEXT *z80, UINT32 low_adr, UINT32 high_adr, UINT8 *Region);

//UINT32 FASTCALL z80_Read_Odo(Z80_CONTEXT *z80);
//void FASTCALL z80_Clear_Odo(Z80_CONTEXT *z80);
//void FASTCALL z80_Set_Odo(Z80_CONTEXT *z80, UINT32 Odo);
//void FASTCALL z80_Add_Cycles(Z80_CONTEXT *z80, UINT32 cycles);

UINT32 FASTCALL z80_Exec(Z80_CONTEXT *z80, int odo);


/**
 * Z80 functions (ported to C)
 */


void mdZ80_init(Z80_CONTEXT *z80);
void mdZ80_reset(Z80_CONTEXT *z80);


unsigned int mdZ80_get_PC(Z80_CONTEXT *z80);
unsigned int mdZ80_get_AF(Z80_CONTEXT *z80);
unsigned int mdZ80_get_AF2(Z80_CONTEXT *z80);
void mdZ80_set_PC(Z80_CONTEXT *z80, unsigned int PC);
void mdZ80_set_AF(Z80_CONTEXT *z80, UINT32 AF);
void mdZ80_set_AF2(Z80_CONTEXT *z80, UINT32 AF2);

unsigned int mdZ80_read_odo(Z80_CONTEXT *z80);
static inline void mdZ80_clear_odo(Z80_CONTEXT *z80)
{
	z80->CycleCnt = 0;
}
void mdZ80_set_odo(Z80_CONTEXT *z80, unsigned int odo);
void mdZ80_add_cycles(Z80_CONTEXT *z80, UINT32 cycles);

//UINT32 FASTCALL mdZ80_Exec(Z80_CONTEXT *z80, int odo);

void mdZ80_nmi(Z80_CONTEXT *z80);
void mdZ80_interrupt(Z80_CONTEXT *z80, unsigned char vector);


/**
 * Default Read/Write functions.
 * TODO: Port these to C.
 * This requires the Read/Write function prototype to be changed to non-fastcall.
 */
extern unsigned char mdZ80_def_mem[0x10000];
unsigned char FASTCALL mdZ80_def_ReadB(UINT32 address);
unsigned char FASTCALL mdZ80_def_In(UINT32 address);
unsigned short FASTCALL mdZ80_def_ReadW(UINT32 address);
void FASTCALL mdZ80_def_WriteB(UINT32 address, unsigned char data);
void FASTCALL mdZ80_def_Out(UINT32 address, unsigned char data);
void FASTCALL mdZ80_def_WriteW(UINT32 address, unsigned short data);


#ifdef __cplusplus
}
#endif

#endif /* __MDZ80_H */
