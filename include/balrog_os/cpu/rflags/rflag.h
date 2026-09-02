#pragma once

/*
    RFLAGS
    Bits	63..32	31	30	29	28	27	26	25	24	23	22	21	20	19	18	17	16	15	14	13..12	11	10	9	8	7	6	5	4	3	2	1	0
    Drapeaux	-	-	-	-	-	-	-	-	-	-	-	ID	VIP	VIF	AC	VM	RF	0	NT	IOPL	OF	DF	IF	TF	SF	ZF	0	AF	0	PF	1	CF
                0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0
                    |           |   |           |   |           |   |           |   |           |   |           |   |           |   |           |

    Set trap flag and 1 flag (reserved)
*/

/* IDENTIFICATION FLAG (CPUID) */
#define RFLAG_ID        0x200000
/* VIRTUAL INTERRRUPT PENDING */
#define RFLAG_VIP       0x100000
/* VIRTUAL INTERRUPT FLAG */
#define RFLAG_VIF       0x080000
/* ALIGNMENT CHECK */
#define RFLAG_AC        0x040000
/* VIRTUAL-8086 mode FLAG*/
#define RFLAG_VM        0x020000
/* RESUME FLAG */
#define RFLAG_RF        0x010000
/* NESTED TASK FLAG */
#define RFLAG_NT        0x004000
/* I/O protection level 3 (RING MODE 3 = User mode) */
#define RFLAG_IOPL_3    0x003000
/* I/O protection level 2 (RING MODE 2 = server mode) */
#define RFLAG_IOPL_2    0x002000
/* I/O protection level 1 (RING MODE 1 = driver mode) */
#define RFLAG_IOPL_1    0x001000
/* I/O protection level 0 (RING MODE 0 = Kernel mode) */
#define RFLAG_IOPL_0    0x000000
/* OVERFLOW FLAG */
#define RFLAG_OF        0x000800
/* DIRECTION FLAG */
#define RFLAG_DF        0x000400
/* INTERRUPT FLAG */
#define RFLAG_IF        0x000200
/* TRAP FLAG */
#define RFLAG_TF        0x000100
/* SIGN FLAG */
#define RFLAG_SF        0x000080
/* ZERO FLAG */
#define RFLAG_ZF        0x000040
/* ADJUST FLAG */
#define RFLAG_AF        0x000010
/* PARITY FLAG */
#define RFLAG_PF        0x000004
/* RESERVED */
#define RFLAG_1         0x000002
/* CARRY FLAG */
#define RFLAG_CF        0x000001

/*
	A mask to get the IOPL from the RFLAGS
*/
#define IOPL_MASK	0x003000

/**
 * @brief A macro that return the IOPL contained into the RFLAGS register
 * 
 */
#define GET_IOPL(rflag)	(rflag & IOPL_MASK)