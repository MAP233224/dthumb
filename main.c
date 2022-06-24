/*

Target CPU: ARM946E-S (Nintendo DS main CPU)
Target Architecture: ARMv5TE
ARM version: 5
THUMB version: 2
Documentation: https://www.intel.com/content/dam/support/us/en/programmable/support-resources/bulk-container/pdfs/literature/third-party/ddi0100e-arm-arm.pdf

*/


/* Alphabetical list of ARM instructions (number of variants) */
/*
ADC         Add with Carry
ADD         Add
AND         Logical AND
B           Branch
BL          Branch and Link
BIC         Bit Clear
BKPT        Breakpoint
BLX (2)     Branch with Link and Exchange
BX          Branch and Exchange
CDP         Coprocessor Data Processing
CLZ         Count Leading Zeros
CMN         Compare Negative
CMP         Compare
EOR         Logical Exclusive OR
LDC         Load Coprocessor
LDM (3)     Load Multiple
LDR         Load Register
LDRB        Load Register Byte
LDRBT       Load Register Byte with Translation
LDRH        Load Register Halfword
LDRSB       Load Register Signed Byte
LDRSH       Load Register Signed Halfword
LDRT        Load Register with Translation
MCR         Move to Coprocessor from ARM Register
MLA         Multiply Accumulate
MOV         Move
MRC         Move to ARM Register from Coprocessor
MRS         Move PSR to General-purpose Register
MSR         Move to Status Register from ARM Register
MUL         Multiply
MVN         Move Negative
ORR         Logical OR
RSB         Reverse Substract
RSC         Reverse Substract with Carry
SBC         Substract with Carry
SMLAL       Signed Multply Accumulate Long
SMULL       Signed Multply Long
STC         Store Coprocessor
STM (2)     Store Multiple
STR         Store Register
STRB        Store Register Byte
STRBT       Store Register Byte with Translation
STRH        Store Register Halfword
STRT        Store Register with Translation
SUB         Substract
SWI         Software Interrupt
SWP         Swap
SWPB        Swap Byte
TEQ         Test Equivalence
TST         Test
UMLAL       Unsigned Multply Accumulate Long
UMULL       Unsigned Multply Long

//DSP enhanced (ARMv5TE exclusive)

LDRD        Load Register Dual
MCRR        Move to Coprocessor from Registers
MRRC        Move to Registers from Coprocessor
PLD         Preload Data
QADD        Saturating signed Add
QDADD       ;Performs a saturated integer doubling of one operand followed by a saturated integer addition with the other operand
QDSUB       ;Performs a saturated integer doubling of one operand followed by a saturated integer substraction from the other operand
QSUB        Saturating signed Subtraction
SMLA        Signed Multiply Accumulate
SMLAL       Signed Multiply Accumulate Long
SMLAW       Signed Multiply Accumulate Word
SMUL        Signed Multiply
SMULW       Signed Multiply Word
STRD        Store Register Dual
*/

/* Alphabetical list of THUMB instructions (number of variants) */
/*
ADC         Add with Carry
ADD (7)     Add
AND         Logical AND
ASR (2)     Arithmetic Shift Right
B (2)       Branch
BIC         Bit Clear
BKPT        Breakpoint
BL          Branch with Link
BLX (2)     Branch with Link and Exchange
BX          Branch and Exchange
CMN         Compare Negative
CMP (3)     Compare
EOR         Logical Exclusive OR
LDMIA       Load Multiple Increment After
LDR (4)     Load Register
LDRB (2)    Load Register Byte
LDRH (2)    Load Register Halfword
LDRSB       Load Register Signed Byte
LDRSH       Load Register Signed Halfword
LSL (2)     Logical Shift Left
LSR (2)     Logical Shift Right
MOV (3)     Move
MUL         Multiply
MVN         Move NOT
NEG         Negate
ORR         Logical OR
POP         Pop Multiple Registers
PUSH        Push Multiple Registers
ROR         Rotate Right Register
SBC         Substract with Carry
STMIA       Store Multiple Increment After
STR (3)     Store Register
STRB (2)    Store Register Byte
STRH (2)    Store Register Halfword
SUB (4)     Substract
SWI         Software Interrupt
TST         Test
*/

/* INCLUDES */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* MACROS */

//#define _CRT_SECURE_NO_WARNINGS 1
#define PATH_LENGTH (256)
#define RANGE_LENGTH (18)
#define STRING_LENGTH (64)
#define CONDITIONS_MAX (16)

#define BITS(x, b, n) ((x >> b) & ((1 << n) - 1)) //retrieves n bits from x starting at bit b
#define SIGNEX32_BITS(x, b, n) ((BITS(x,b,n) ^ (1<<(n-1))) - (1<<(n-1))) //convert n-bit value to signed 32 bits
#define SIGNEX32_VAL(x, n) ((x ^ (1<<(n-1))) - (1<<(n-1))) //convert n-bit value to signed 32 bits
#define ROR(x, n) ((x>>n)|(x<<(32-n))) //rotate right 32-bit value x by n bits

/* TYPEDEFS */

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef struct {
    int start; //starting address
    int end; //end address
}FILERANGE;

typedef enum {
    ARMv4T, //ARM v4, THUMB v1
    ARMv5TE, //ARM v5, THUMB v2
    ARMv6, //ARM v6, THUMB v3
}ARMARCH; //only 32-bit legacy architectures with THUMB support

typedef enum {
    EQ, //equal, Z set
    NE, //not equal, Z clear
    CS, //carry set, C set (or HS, unsigned higher or same)
    CC, //carry clear, C clear (or LO, unsigned lower)
    MI, //minus/negative, N set
    PL, //plus/positive/zero, N clear
    VS, //overflow, V set
    VC, //no overflow, V clear
    HI, //unsigned higher, C set and Z clear
    LS, //unsigned lower or same, C clear or Z set
    GE, //signed greater than or equal, N==V
    LT, //signed less than, N!=V
    GT, //signed greater than, Z==0 and N==V
    LE, //signed less than or equal, Z==1 or N!=V
    AL, //unconditional, only with IT instructions
    NV  //unconditional, usually undefined
}CONDITION;

/* GLOBALS */

//todo: maybe put "2" instead of "nv" (or nothing) in the last one
const u8 Conditions[CONDITIONS_MAX][3] = { "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le", "", "" }; //last two are "al" and "nv", but never displayed

const u8 IT_xyz_0[CONDITIONS_MAX][4] = { //if then block suffixes
    "", //doesn't exist
    "ttt",
    "tt",
    "tte",
    "t",
    "tet",
    "te",
    "tee",
    "", //ommitted all
    "eee",
    "ee",
    "eet",
    "e",
    "ete",
    "et",
    "ett"
};

const u8 IT_xyz_1[CONDITIONS_MAX][4] = { //inverse of the one above
    "", //doesn't exist
    "eee",
    "ee",
    "eet",
    "e",
    "ete",
    "et",
    "ett",
    "", //ommitted all
    "tee",
    "te",
    "tet",
    "t",
    "tte",
    "tt",
    "ttt"
};

const u8 AddressingModes[4][3] = {
    "da", //Decrement after
    "ia", //Increment after
    "db", //Decrement before
    "ib"  //Increment before
};

const u8 DataProcessing_thumb[16][4] = {
    "and",
    "eor",
    "lsl",
    "lsr",
    "asr",
    "adc",
    "sbc",
    "ror",
    "tst", //no s
    "rsb",
    "cmp", //no s
    "cmn", //no s
    "orr",
    "mul",
    "bic",
    "mvn"
};

const u8 DataProcessing_arm[16][4] = {
    "and",
    "eor",
    "sub",
    "rsb",
    "add",
    "adc",
    "sbc",
    "rsc",
    "tst", //no s
    "teq", //no s
    "cmp", //no s
    "cmn", //no s
    "orr",
    "mov", //only 1 source operand
    "bic",
    "mvn"  //only 1 source operand
};

const u8 MSR_cxsf[16][5] = {
    "",
    "c",
    "x",
    "xc",
    "s",
    "sc",
    "sx",
    "sxc",
    "f",
    "fc",
    "fx",
    "fxc",
    "fs",
    "fsc",
    "fsx",
    "fsxc"
};

const u8 DSP_AddSub[4][6] = { "qadd", "qsub", "qdadd", "qdsub" };
const u8 DSP_Multiplies[4][6] = { "smla", "", "smlal", "smul" }; //slot 1 empty, decided elsehow
const u8 MultiplyLong[4][6] = { "umull", "umlal", "smull", "smlal" };
const u8 MovAddSubImmediate[4][4] = { "mov", "cmp", "add", "sub" }; //cmp won't be used
const u8 LoadStoreRegister[8][6] = { "str", "strh", "strb", "ldrsb", "ldr", "ldrh", "ldrb", "ldrsh" };
const u8 CPS_effect[2][3] = { "ie", "id" };
const u8 CPS_flags[8][5] = { "none", "f", "i", "if", "a", "af", "ai", "aif" };
const u8 SignZeroExtend[4][5] = { "sxth", "sxtb", "uxth", "uxtb" };
const u8 Shifters[4][4] = { "lsl", "lsr", "asr", "ror" }; //+rrx

u32 debug_na_count = 0;

/* FUNCTIONS */

static void SubstituteSubString(u8 dst[STRING_LENGTH], u32 index, const u8* sub, u32 size) {
    /* Insert sub string of length size (< STRING_LENGTH) at dst[index] */
    u8 tmp[STRING_LENGTH] = { 0 }; //zinit
    memcpy(&tmp, sub, size); //init
    memcpy(&tmp[size], &dst[index + size + 1], STRING_LENGTH - index - size - 1); //save
    memcpy(&dst[index], tmp, STRING_LENGTH - index - 1); //restore
}

static void CheckSpecialRegister(u8* str, int size) {
    /* Replace special registers by their common names */
    //(r12->ip), r13->sp, r14->lr, r15->pc
    //todo: check for size != -1
    for (int i = 0; i < size; i++) //when using a variable string size (return value of sprintf), 10% faster than constant size of STRING_LENGTH 64
    {
        if (str[i] == 'r')
        {
            switch (*(u16*)&str[i + 1])
            {
            case 0x3331: //"13"
            {
                SubstituteSubString(str, i, "sp", sizeof("sp") - 1);
                break;
            }
            case 0x3431: //"14"
            {
                SubstituteSubString(str, i, "lr", sizeof("lr") - 1);
                break;
            }
            case 0x3531: //"15"
            {
                SubstituteSubString(str, i, "pc", sizeof("pc") - 1);
                break;
            }
            }
        }
    }
}

static u32 FormatStringRegisterList_thumb(u8 str[STRING_LENGTH], u16 reg, const u8 pclr[3]) {
    /**/
    //todo for later: maybe group consecutive registers together with a -
    u32 bits = 0;
    u32 pos = 0; //position in the str array
    for (u32 i = 0; i < 9; i++) //9 instead of 16 because of clever grouping
    {
        if (BITS(reg, i, 1))
        {
            bits++;
            u8 tmp[4] = { 0 };
            if (i == 8) sprintf(tmp, pclr, i); //pc or lr
            else sprintf(tmp, "r%u,", i); //r0-r7
            memcpy(&str[pos], tmp, sizeof(tmp));
            pos += 3; //depends on size of tmp
        }
    }
    if (pos) str[pos - 1] = 0; //removes the coma on the last register
    return bits; //number of 1 bits
}

static u32 FormatStringRegisterList_arm(u8 str[STRING_LENGTH], u16 reg) {
    /**/
    //todo for later: maybe group consecutive registers together with a -
    u32 bits = 0;
    u32 pos = 0; //position in the str array
    for (u32 i = 0; i < 16; i++) //all registers
    {
        if (BITS(reg, i, 1))
        {
            bits++;
            u8 tmp[5] = { 0 };
            sprintf(tmp, "r%u,", i);
            memcpy(&str[pos], tmp, sizeof(tmp));
            pos = (i < 10) ? pos + 3 : pos + 4;
        }
    }
    if (pos) str[pos - 1] = 0; //removes the coma on the last register
    return bits; //number of 1 bits
}

static u32 CountBits(u8 b) {
    /* Count bits in a byte */
    static const u8 lut[16] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };
    return lut[b & 0x0f] + lut[b >> 4];
}

//todo: inline IfThen functions in Disassemble_thumb (no IT blocks in ARMv5TE)

static int IfThen_imm_2(const u8* op, u8 str[STRING_LENGTH], u8 rm, u8 imm) {
    /* Formatting, return size of characters written by sprintf */
    //if (it) return sprintf(str, "%s%s r%u, #0x%X", op, cond, rm, imm);
    return sprintf(str, "%ss r%u, #0x%X", op, rm, imm);
}

static int IfThen_imm_3(const u8* op, u8 str[STRING_LENGTH], u8 rd, u8 rm, u8 imm) {
    /* Formatting, return size of characters written by sprintf */
    //if (it) return sprintf(str, "%s%s r%u, r%u, #0x%X", op, cond, rd, rm, imm);
    return sprintf(str, "%ss r%u, r%u, #0x%X", op, rd, rm, imm);
}

static int IfThen_reg_2(const u8* op, u8 str[STRING_LENGTH], u8 rd, u8 rm) {
    /* Formatting, return size of characters written by sprintf */
    //if (it) return sprintf(str, "%s%s r%u, r%u", op, cond, rd, rm);
    return sprintf(str, "%ss r%u, r%u", op, rd, rm);
}

static int IfThen_reg_3(const u8* op, u8 str[STRING_LENGTH], u8 rd, u8 rm, u8 rn) {
    /* Formatting, return size of characters written by sprintf */
    //if (it) return sprintf(str, "%s%s r%u, r%u, r%u", op, cond, rd, rm, rn);
    return sprintf(str, "%ss r%u, r%u, r%u", op, rd, rm, rn);
}

static void FormatExtraLoadStore(u32 c, u8* str, u8 cond, const u8* op) {
    /*  */
    u8 rd = BITS(c, 12, 4);
    u8 rn = BITS(c, 16, 4);
    u8 w = BITS(c, 21, 1);
    u8 p = BITS(c, 24, 1);
    if (BITS(c, 22, 1)) //immediate
    {
        u8 ofs = (BITS(c, 8, 4) << 4) | BITS(c, 0, 4);
        u8* sign = BITS(c, 23, 1) ? "+" : "-";
        if (p) //offset, pre-indexed
        {
            u8* pre = w ? "!" : "";
            sprintf(str, "%s%s r%u, [r%u, #%s0x%X]%s", op, Conditions[cond], rd, rn, sign, ofs, pre);
        }
        else //post-indexed
        {
            if (w) return; //w must be 0, else UNPREDICTABLE //todo: check if really invalid
            sprintf(str, "%s%s r%u, [r%u], #%s0x%X", op, Conditions[cond], rd, rn, sign, ofs);
        }
    }
    else //register
    {
        u8 rm = BITS(c, 0, 4);
        u8* sign = BITS(c, 23, 1) ? "" : "-"; //implicit "+" in front of registers
        if (p) //offset, pre-indexed
        {
            u8* pre = w ? "!" : "";
            sprintf(str, "%s%s r%u, [r%u, %sr%u]%s", op, Conditions[cond], rd, rn, sign, rm, pre);
        }
        else //post-indexed
        {
            if (w) return; //w must be 0, else UNPREDICTABLE //todo: check if really invalid
            sprintf(str, "%s%s r%u, [r%u], %sr%u", op, Conditions[cond], rd, rn, sign, rm);
        }
    }
}

static u32 Disassemble_thumb(u32 code, u8 str[STRING_LENGTH], ARMARCH tv) {
    /* Convert a code into a string, return 0 if processed THUMB 16-bit, 1 if processed THUMB 32-bit  */

    //todo: refactor, follow documentation better
    //todo: remove ARMv6 instructions
    //switch (c >> 13)
    //0: Shift by immediate, Add/substract register, Add/substract immediate
    //1: Add/substract/compare/move immediate
    //2: Data-processing register, Special data-processing, Branch/exchange instruction set, Load from literal pool, Load/store register offset
    //3: Load/store word/byte immediate offset
    //4: Load/store halword immediate offset, Load/store to/from stack
    //5: Add to SP or PC, Miscellaneous (see fig. 6-2)
    //6: Load/store multiple, Conditional branch, Undefined instruction, Software interrupt
    //7: Unconditional branch, BLX suffix, Undefined instruction, BL/BLX prefix, BL suffix

    u32 thumb_size = 0; //return value
    u16 c = code & 0xffff; //low 16 bits
    int size = 0; //return value of sprintf to be passed to CheckSpecialRegister

    switch (c >> 13)
    {
    case 0: //0x0000 //LSL, LSR, ASR, ADD, SUB
    {
        switch (BITS(c, 11, 2))
        {
        case 0:
        {
            if (tv >= ARMv5TE)
            {
                if (BITS(c, 6, 5)) //LSL immediate
                {
                    size = IfThen_imm_3("lsl", str, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
                }
                else //MOV register
                {
                    size = sprintf(str, "movs r%u, r%u", BITS(c, 0, 3), BITS(c, 3, 3));
                }
            }
            else //ARMv4T
            {
                size = sprintf(str, "lsl r%u, r%u, #0x%X", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
            }
            break;
        }
        case 1: //LSR immediate
        {
            if (tv >= ARMv5TE)
            {
                size = IfThen_imm_3("lsr", str, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
            }
            else //ARMv4T
            {
                size = sprintf(str, "lsr r%u, r%u, #0x%X", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
            }
            break;
        }
        case 2: //ASR immediate
        {
            if (tv >= ARMv5TE)
            {
                size = IfThen_imm_3("asr", str, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
            }
            else //ARMv4T
            {
                size = sprintf(str, "asr r%u, r%u, #0x%X", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
            }
            break;
        }
        case 3: //ADD, SUB
        {
            if (BITS(c, 10, 1)) //ADD/SUB immediate
            {
                if (BITS(c, 9, 1)) //SUB immediate
                {
                    if (tv >= ARMv5TE)
                    {
                        size = IfThen_imm_3("sub", str, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                    else //ARMv4T
                    {
                        size = sprintf(str, "sub r%u, r%u, #0x%X", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                }
                else //ADD immediate
                {
                    if (tv >= ARMv5TE)
                    {
                        size = IfThen_imm_3("add", str, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                    else //ARMv4T
                    {
                        size = sprintf(str, "add r%u, r%u, #0x%X", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                }
            }
            else //ADD/SUB register
            {
                if (BITS(c, 9, 1)) //SUB register
                {
                    if (tv >= ARMv5TE)
                    {
                        size = IfThen_reg_3("sub", str, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                    else //ARMv4T
                    {
                        size = sprintf(str, "sub r%u, r%u, r%u", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                }
                else //ADD register
                {
                    if (tv >= ARMv5TE)
                    {
                        size = IfThen_reg_3("add", str, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                    else //ARMv4T
                    {
                        size = sprintf(str, "add r%u, r%u, r%u", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                }
            }
            break;
        }
        }
        break;
    }

    case 1: //0x2000 //MOV CMP, ADD, SUB
    {
        u8 index = BITS(c, 11, 2);
        if (index == 1) size = sprintf(str, "cmp r%u, #0x%X", BITS(c, 8, 3), BITS(c, 0, 8));
        else size = IfThen_imm_2(MovAddSubImmediate[index], str, BITS(c, 8, 3), BITS(c, 0, 8));
        break;
    }

    case 2: //0x4000 //lots...
    {
        if (BITS(c, 10, 3) == 1)
        {
            switch (BITS(c, 8, 2))
            {
            case 0: //ADD register
            {
                if (tv >= ARMv5TE)
                {
                    u8 d = (BITS(c, 7, 1) << 3) | (BITS(c, 0, 3));
                    u8 m = BITS(c, 3, 4);
                    if (m == 13) size = sprintf(str, "add r%u, sp, r%u", d, d); //ADD sp + register v1
                    else size = sprintf(str, "add r%u, r%u", d, m); //ADD
                }
                break;
            }
            case 1: //CMP high register
            {
                if (tv >= ARMv5TE)
                {
                    size = sprintf(str, "cmp r%u, r%u", (BITS(c, 7, 1) << 3) | (BITS(c, 0, 3)), BITS(c, 3, 4));
                }
                break;
            }
            case 2: //MOV register
            {
                size = sprintf(str, "mov r%u, r%u", (BITS(c, 7, 1) << 3) | (BITS(c, 0, 3)), BITS(c, 3, 4));
                break;
            }
            case 3:
            {
                if (BITS(c, 7, 1)) size = sprintf(str, "blx r%u", BITS(c, 3, 4)); //BLX
                else size = sprintf(str, "bx r%u", BITS(c, 3, 4)); //BX
                break;
            }
            }
        }
        else if (!BITS(c, 10, 3))
        {
            u8 index = BITS(c, 6, 4);
            switch (index)
            {
            case 8:
            case 10:
            case 11:
            {
                size = sprintf(str, "%s r%u, r%u", DataProcessing_thumb[index], BITS(c, 0, 3), BITS(c, 3, 3));
                break;
            }
            default:
            {
                size = IfThen_reg_2(DataProcessing_thumb[index], str, BITS(c, 0, 3), BITS(c, 3, 3));
            }
            }
        }
        else
        {
            if (BITS(c, 12, 1)) size = sprintf(str, "%s r%u, [r%u, r%u]", LoadStoreRegister[BITS(c, 9, 3)], BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3)); //Load/store register offset  
            else size = sprintf(str, "ldr r%u, [pc, #0x%X]", BITS(c, 8, 3), 4 * BITS(c, 0, 8)); //LDR literal pool
        }
        break;
    }

    case 3: //0x6000 //STR, LDR, STRB, LDRB
    {
        switch (BITS(c, 11, 2))
        {
        case 0: //STR
        {
            size = sprintf(str, "str r%u, [r%u, #0x%X]", BITS(c, 0, 3), BITS(c, 3, 3), 4 * BITS(c, 6, 5));
            break;
        }
        case 1: //LDR
        {
            size = sprintf(str, "ldr r%u, [r%u, #0x%X]", BITS(c, 0, 3), BITS(c, 3, 3), 4 * BITS(c, 6, 5));
            break;
        }
        case 2: //STRB
        {
            size = sprintf(str, "strb r%u, [r%u, #0x%X]", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
            break;
        }
        case 3: //LDRB
        {
            size = sprintf(str, "ldrb r%u, [r%u, #0x%X]", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
            break;
        }
        }
        break;
    }

    case 4: //0x8000 //STR, LDR, STRH, LDRH
    {
        if (BITS(c, 12, 1))
        {
            if (tv >= ARMv5TE)
            {
                if (BITS(c, 11, 1)) size = sprintf(str, "ldr r%u, [sp, #0x%X]", BITS(c, 8, 3), 4 * BITS(c, 0, 8)); //LDR stack
                else size = sprintf(str, "str r%u, [sp, #0x%X]", BITS(c, 8, 3), 4 * BITS(c, 0, 8)); //STR stack
            }
        }
        else
        {
            if (BITS(c, 11, 1)) size = sprintf(str, "ldrh r%u, [r%u, #0x%X]", BITS(c, 0, 3), BITS(c, 3, 3), 2 * BITS(c, 6, 5)); //LRDH immediate offset
            else size = sprintf(str, "strh r%u, [r%u, #0x%X]", BITS(c, 0, 3), BITS(c, 3, 3), 2 * BITS(c, 6, 5)); //STRH immediate offset
        }
        break;
    }

    case 5: //0xA000 //Misc and ADD to sp or pc
    {
        if (BITS(c, 12, 1)) //misc
        {
            switch (BITS(c, 8, 4))
            {
            case 0: //ADD/SUB sp
            {
                if (BITS(c, 7, 1)) size = sprintf(str, "sub sp, sp, #0x%X", 4 * BITS(c, 0, 7)); //SUB             
                else size = sprintf(str, "add sp, sp, #0x%X", 4 * BITS(c, 0, 7)); //ADD
                break;
            }
            //Compare and branch on (non-)zero
            case 1:
            case 3:
            case 9:
            case 11:
            {
                //if (BITS(c, 11, 1)) size = sprintf(str, "cbnz r%u, #0x%X", BITS(c, 0, 3), 4 + 2 * ((BITS(c, 9, 1) << 5) | BITS(c, 3, 5))); //CBNZ           
                //else size = sprintf(str, "cbz r%u, #0x%X", BITS(c, 0, 3), 4 + 2 * ((BITS(c, 9, 1) << 5) | BITS(c, 3, 5))); //CBZ               
                break;
            }
            case 2: //Sign/zero extend
            {
                //if (tv >= ARMv6)
                //{
                //    size = sprintf(str, "%s r%u, r%u", SignZeroExtend[BITS(c, 6, 2)], BITS(c, 0, 3), BITS(c, 3, 3));
                //}
                break;
            }
            //PUSH/POP
            case 4:
            case 5:
            case 12:
            case 13:
            {
                u8 reglist[STRING_LENGTH] = { 0 };
                u16 registers = BITS(c, 0, 9);
                if (BITS(c, 11, 1)) //POP
                {
                    if (FormatStringRegisterList_thumb(reglist, registers, "pc")) //if BitCount(registers) < 1 then UNPREDICTABLE
                    {
                        size = sprintf(str, "pop {%s}", reglist);
                    }
                }
                else //PUSH
                {
                    if (FormatStringRegisterList_thumb(reglist, registers, "lr")) //if BitCount(registers) < 1 then UNPREDICTABLE
                    {
                        size = sprintf(str, "push {%s}", reglist);
                    }
                }
                break;
            }
            case 6: //SETEND and CPS
            {
                //if (tv >= ARMv6)
                //{
                //    if (c == 0xB650) size = sprintf(str, "setend le");
                //    else if (c == 0xB658) size = sprintf(str, "setend be");
                //    else if (BITS(c, 4, 8) == 100); //n/a
                //    else if (BITS(c, 5, 7) == 51)
                //    {
                //        if (!BITS(c, 3, 1)) size = sprintf(str, "cps%s %s", CPS_effect[BITS(c, 4, 1)], CPS_flags[BITS(c, 0, 3)]); //CPS
                //    }
                //}
                break;
            }
            case 10: //Reverse byte
            {
                //if (tv >= ARMv6)
                //{
                //    switch (BITS(c, 6, 2))
                //    {
                //    case 0: //REV
                //    {
                //        size = sprintf(str, "rev r%u, r%u", BITS(c, 0, 3), BITS(c, 3, 3));
                //        break;
                //    }
                //    case 1: //REV16
                //    {
                //        size = sprintf(str, "rev16 r%u, r%u", BITS(c, 0, 3), BITS(c, 3, 3));
                //        break;
                //    }
                //    case 2: //undefined
                //    {
                //        break;
                //    }
                //    case 3: //REVSH
                //    {
                //        size = sprintf(str, "revsh r%u, r%u", BITS(c, 0, 3), BITS(c, 3, 3));
                //        break;
                //    }
                //    }
                //}
                break;
            }
            case 14: //BKPT
            {
                size = sprintf(str, "bkpt #0x%X", BITS(c, 0, 8));
                break;
            }
            case 15: //IT and NOP-hints
            {
                //u8 mask = BITS(c, 0, 4);
                //if (mask) //IT
                //{
                //    u8 firstcond = BITS(c, 4, 4);
                //    if (firstcond == NV); //n/a
                //    else if (firstcond == AL && CountBits(mask) != 1); //n/a
                //    else
                //    {
                //        if (firstcond & 1) size = sprintf(str, "it%s %s", IT_xyz_1[mask], Conditions[firstcond]);
                //        else size = sprintf(str, "it%s %s", IT_xyz_0[mask], Conditions[firstcond]);
                //    }
                //}
                //else //NOP-compatible hints
                //{
                //    u8 hint = BITS(c, 4, 4);
                //    switch (hint)
                //    {
                //    case 0: size = sprintf(str, "nop"); break;
                //    case 1: size = sprintf(str, "yield"); break;
                //    case 2: size = sprintf(str, "wfe"); break;
                //    case 3: size = sprintf(str, "wfi"); break;
                //    case 4: size = sprintf(str, "sev"); break;
                //    default: size = sprintf(str, "hint #0x%X", hint);
                //    }
                //}
                break;
            }
            }
        }
        else //add to SP or PC
        {
            if (BITS(c, 11, 1)) size = sprintf(str, "add r%u, sp, #0x%X", BITS(c, 8, 3), 4 * BITS(c, 0, 8)); //ADD (SP plus immediate)
            else size = sprintf(str, "adr r%u, #0x%X", BITS(c, 8, 3), 4 * BITS(c, 0, 8)); //ADR (pc)
        }
        break;
    }

    case 6: //0xC000 //B, SWI, LDMIA, STMIA
    {
        if (BITS(c, 12, 1)) //Conditional branch, Undefined, System call
        {
            switch (BITS(c, 8, 4))
            {
            case 14: //UDF "Permanently undefined space", OS dependant
            {
                size = sprintf(str, "udf #0x%X", BITS(c, 0, 8));
                break;
            }
            case 15: //SWI
            {
                size = sprintf(str, "swi #0x%X", BITS(c, 0, 8));
                break;
            }
            default: //B conditional
            {
                size = sprintf(str, "b%s #0x%X", Conditions[BITS(c, 8, 4)], 4 + 2 * SIGNEX32_BITS(c, 0, 8));
            }
            }
        }
        else //Load/store multiple
        {
            u8 reglist[STRING_LENGTH] = { 0 };
            u16 registers = BITS(c, 0, 8);
            u8 rn = BITS(c, 8, 3);
            if (BITS(c, 11, 1)) //LDMIA
            {
                if (FormatStringRegisterList_thumb(reglist, registers, ""))
                {
                    if (BITS(registers, rn, 1)) size = sprintf(str, "ldmia r%u, {%s}", rn, reglist); //no '!' because rn is also in registers
                    else size = sprintf(str, "ldmia r%u!, {%s}", rn, reglist);
                }
            }
            else //STMIA
            {
                if (FormatStringRegisterList_thumb(reglist, registers, ""))
                {
                    size = sprintf(str, "stmia r%u!, {%s}", rn, reglist);
                }
            }
        }
        break;
    }

    case 7: //0xE000 //B, then 32-bit instructions
    {
        switch (BITS(c, 11, 2))
        {
        case 0:
        {
            size = sprintf(str, "b #0x%X", 4 + 2 * SIGNEX32_BITS(c, 0, 11)); //11 bits to signed 32 bits
            break;
        }
        //case 1: break; //undefined on first pass
        case 2: //BL/BLX prefix
        {
            c = code >> 16; //get high 16 bits
            if (c >> 13 == 7)
            {
                switch (BITS(c, 11, 2))
                {
                case 1: //BLX suffix
                {
                    if (!BITS(c, 0, 1))
                    {
                        thumb_size++;
                        u32 ofs = ((BITS((code & 0xffff), 0, 10) << 10) | (BITS(c, 1, 10))) << 2;
                        size = sprintf(str, "blx #0x%X", 4 + SIGNEX32_VAL(ofs, 22));
                    }
                    break;
                }
                //case 2: break; //BL/BLX prefix
                case 3: //BL suffix
                {
                    thumb_size++;
                    u32 ofs = ((BITS((code & 0xffff), 0, 10) << 11) | (BITS(c, 0, 11))) << 1;
                    size = sprintf(str, "bl #0x%X", 4 + (int)SIGNEX32_VAL(ofs, 22));
                    break;
                }
                }
            }
            break;
        }
        }
        break;
    }
    }
    if (!str[0]) //in case nothing was written to it
    {
        debug_na_count++;
        sprintf(str, "n/a");
    }
    else
    {
        CheckSpecialRegister(str, size); //formatting
    }
    return thumb_size;
}

static void Disassemble_arm(u32 code, u8 str[STRING_LENGTH], ARMARCH av) {
    /**/
    //Reference: page 68 of 811 from the ARM Architecture reference manual june 2000 edition
    //only supports ARMv5
    //todo: extra caution for UNPREDICTABLE instructions, need to remove them? or decode regardless?

    int size = 0; //return value of sprintf to be passed to CheckSpecialRegister
    u32 c = code; //alias
    u8 cond = BITS(c, 28, 4); //condition bits

    //todo: check unconditional instructions first to avoid putting a check later at each stage

    switch (BITS(c, 25, 3))
    {
    case 0: //Data processing, DSP instructions, ...
    {
        if (cond == NV) break; //undefined
        if (BITS(c, 4, 1))
        {
            if (BITS(c, 7, 1)) //Multiplies, extra load/stores: see fig 3-2
            {
                u8 oplo = BITS(c, 5, 2);
                if (!oplo)
                {
                    if (!BITS(c, 22, 3)) //Multiply (accumulate)
                    {
                        u8 rm = BITS(c, 0, 4);
                        u8 rs = BITS(c, 8, 4);
                        u8 rn = BITS(c, 12, 4);
                        u8 rd = BITS(c, 16, 4);
                        u8* s = BITS(c, 20, 1) ? "s" : "";
                        if (BITS(c, 21, 1)) //MLA
                        {
                            size = sprintf(str, "mla%s%s r%u, r%u, r%u, r%u", s, Conditions[cond], rd, rm, rs, rn);
                        }
                        else //MUL
                        {
                            if (rn) break; //Should-Be-Zero
                            size = sprintf(str, "mul%s%s r%u, r%u, r%u", s, Conditions[cond], rd, rm, rs);
                        }
                    }
                    else if (BITS(c, 23, 1)) //Multiply (accumulate) long
                    {
                        u8* s = BITS(c, 20, 1) ? "s" : "";
                        size = sprintf(str, "%s%s%s r%u, r%u, r%u, r%u", MultiplyLong[BITS(c, 21, 2)], s, Conditions[cond], BITS(c, 12, 4), BITS(c, 16, 4), BITS(c, 0, 4), BITS(c, 8, 4));
                    }
                    else //Swap/swap byte (SWP, SWPB)
                    {
                        if (BITS(c, 8, 4)) break; //Should-Be-Zero
                        u8* b = BITS(c, 22, 1) ? "b" : ""; //byte or no
                        size = sprintf(str, "swp%s%s r%u, r%u, [r%u]", b, Conditions[cond], BITS(c, 12, 4), BITS(c, 0, 4), BITS(c, 16, 4));
                    }
                }
                else
                {
                    if (!BITS(c, 22, 1) && BITS(c, 8, 4)) break; //Should-Be-Zero if register offset
                    if (oplo == 1) //Load/store halfword
                    {
                        u8* l = BITS(c, 20, 1) ? "ldrh" : "strh"; //load or store
                        FormatExtraLoadStore(c, str, cond, l);
                    }
                    else
                    {
                        if (BITS(c, 20, 1)) //Load signed halfword/byte
                        {
                            u8* h = BITS(c, 5, 1) ? "ldrsh" : "ldrsb"; //halfword/byte
                            FormatExtraLoadStore(c, str, cond, h);
                        }
                        else //Load/store two words
                        {
                            if (BITS(c, 12, 1)) break; //undefined if Rd is odd
                            u8* s = BITS(c, 5, 1) ? "strd" : "ldrd"; //store or load
                            FormatExtraLoadStore(c, str, cond, s);
                        }
                    }
                }
            }
            else
            {
                if (BITS(c, 23, 2) == 2 && !BITS(c, 20, 1)) //Miscellanous instructions, see fig 3-3
                {
                    u8 oplo = BITS(c, 5, 2);
                    u8 ophi = BITS(c, 21, 2);
                    switch (oplo)
                    {
                    case 0:
                    {
                        if (ophi == 3) //Count leading zeroes (CLZ)
                        {
                            //note: if PC is in either register, UNPREDICTABLE
                            if (!BITS(c, 16, 4) || !BITS(c, 8, 4)) break; //Should-Be-One
                            size = sprintf(str, "clz%s r%u, r%u", Conditions[cond], BITS(c, 12, 4), BITS(c, 0, 4));
                        }
                        else if (ophi == 1)//Branch/exchange instruction set (BX)
                        {
                            if (BITS(c, 8, 12) != 0xfff) break; //Should-Be-One
                            size = sprintf(str, "bx%s r%u", Conditions[cond], BITS(c, 0, 4));
                        }
                        break;
                    }
                    case 1: //Branch and link/exchange instruction set (BLX)
                    {
                        if (ophi != 1) break;
                        if (BITS(c, 8, 12) != 0xfff) break; //Should-Be-One
                        size = sprintf(str, "blx%s r%u", Conditions[cond], BITS(c, 0, 4));
                        break;
                    }
                    case 2: //Enhanced DSP add/sub (QADD, QDADD, QSUB, QDSUB)
                    {
                        //note: if PC is in either register, UNPREDICTABLE
                        if (BITS(c, 8, 4)) break; //Should-Be-Zero
                        size = sprintf(str, "%s%s r%u, r%u, r%u", DSP_AddSub[ophi], Conditions[cond], BITS(c, 12, 4), BITS(c, 0, 4), BITS(c, 16, 4));
                        break;
                    }
                    case 3: //Software breakpoint (BKPT)
                    {
                        if (ophi != 1) break;
                        size = sprintf(str, "bkpt #0x%X", (BITS(c, 8, 12) << 4) | BITS(c, 0, 4));
                        break;
                    }
                    }
                }
                else //Data processing register shift
                {
                    u8 rm = BITS(c, 0, 4);
                    u8 shift = BITS(c, 5, 2);
                    u8 rs = BITS(c, 8, 4);
                    u8 rd = BITS(c, 12, 4);
                    u8 rn = BITS(c, 16, 4);
                    u8 op = BITS(c, 21, 4);
                    u8* s = BITS(c, 20, 1) ? "s" : "";
                    switch (op)
                    {
                    case 8: //TST
                    case 9: //TEQ
                    case 10: //CMP
                    case 11: //CMN
                    {
                        size = sprintf(str, "%s%s r%u, r%u, %s r%u", DataProcessing_arm[op], Conditions[cond], rn, rm, Shifters[shift], rs);
                        break;
                    }
                    case 13: //MOV
                    case 15: //MVN
                    {
                        size = sprintf(str, "%s%s%s r%u, r%u, %s r%u", DataProcessing_arm[op], s, Conditions[cond], rd, rm, Shifters[shift], rs);
                        break;
                    }
                    default:
                    {
                        size = sprintf(str, "%s%s%s r%u, r%u, %s r%u", DataProcessing_arm[op], s, Conditions[cond], rd, rm, Shifters[shift], rs);
                    }
                    }
                }
            }
        }
        else //bit 4 == 0
        {
            if (BITS(c, 23, 2) == 2 && !BITS(c, 20, 1)) //Miscellanous instructions, see fig 3-3
            {
                if (BITS(c, 7, 1)) //Enhanced DSP multiplies
                {
                    //note: PC for any register is UNPREDICTABLE
                    u8 rm = BITS(c, 0, 4);
                    u8* x = BITS(c, 5, 1) ? "t" : "b";
                    u8* y = BITS(c, 6, 1) ? "t" : "b";
                    u8 rs = BITS(c, 8, 4);
                    u8 rn_rdlo = BITS(c, 12, 4);
                    u8 rd_rdhi = BITS(c, 16, 4);
                    u8 op = BITS(c, 21, 2);
                    switch (op)
                    {
                    case 0: //SMLA
                    {
                        size = sprintf(str, "%s%s%s%s r%u, r%u, r%u, r%u", DSP_Multiplies[op], x, y, Conditions[cond], rd_rdhi, rm, rs, rn_rdlo);
                        break;
                    }
                    case 1: //SMLAW, SMULW
                    {
                        if (BITS(c, 5, 1)) //SMULW
                        {
                            if (rn_rdlo) break; //Should-Be-Zero 
                            size = sprintf(str, "smulw%s%s r%u, r%u, r%u", y, Conditions[cond], rd_rdhi, rm, rs);
                        }
                        else //SMLAW
                        {
                            size = sprintf(str, "smlaw%s%s r%u, r%u, r%u, r%u", y, Conditions[cond], rd_rdhi, rm, rs, rn_rdlo);
                        }
                        break;
                    }
                    case 2: //SMLAL
                    {
                        size = sprintf(str, "%s%s%s%s r%u, r%u, r%u, r%u", DSP_Multiplies[op], x, y, Conditions[cond], rn_rdlo, rd_rdhi, rm, rs);
                        break;
                    }
                    case 3: //SMUL
                    {
                        if (rn_rdlo) break; //Should-Be-Zero
                        size = sprintf(str, "%s%s%s%s r%u, r%u, r%u", DSP_Multiplies[op], x, y, Conditions[cond], rd_rdhi, rm, rs);
                        break;
                    }
                    }
                }
                else
                {
                    if (!BITS(c, 0, 12) && BITS(c, 16, 4) == 15) //Move status reg to reg (MRS)
                    {
                        //note: if Rd == PC, UNPREDICTABLE
                        u8 sreg = BITS(c, 22, 1) ? 's' : 'c'; //SPSR (1) or CPSR (0)
                        size = sprintf(str, "mrs%s r%u, %cpsr", Conditions[cond], BITS(c, 12, 4), sreg);
                    }
                    else if (BITS(c, 12, 4) == 15 && !BITS(c, 4, 8) && BITS(c, 21, 1)) //Move reg to status reg (MSR register)
                    {
                        u8 sreg = BITS(c, 22, 1) ? 's' : 'c'; //SPSR (1) or CPSR (0)
                        size = sprintf(str, "msr%s %cpsr_%s, r%u", Conditions[cond], sreg, MSR_cxsf[BITS(c, 16, 4)], BITS(c, 0, 4));
                    }
                }
            }
            else //Data processing immediate shift
            {
                //todo: write a function for all 3 Data Processing switch-cases
                u8 op = BITS(c, 21, 4);
                u8 rm = BITS(c, 0, 4);
                u8 shift = BITS(c, 5, 2);
                u8 shift_imm = BITS(c, 7, 5);
                u8 sstr[STRING_LENGTH] = "rrx"; //default, overwrite if incorrect
                if ((shift == 1 || shift == 2) && !shift_imm) shift_imm = 32; //0~31 for LSL, 1~32 for LSR, ASR and ROR, always 0 for RRX
                if (!(shift == 3 && !shift_imm)) sprintf(sstr, "%s #%u", Shifters[shift], shift_imm);
                u8 rd = BITS(c, 12, 4);
                u8 rn = BITS(c, 16, 4);
                u8* s = BITS(c, 20, 1) ? "s" : "";
                switch (op)
                {
                case 8: //TST
                case 9: //TEQ
                case 10: //CMP
                case 11: //CMN
                {
                    size = sprintf(str, "%s%s r%u, r%u, %s", DataProcessing_arm[op], Conditions[cond], rn, rm, sstr);
                    break;
                }
                case 13: //MOV
                case 15: //MVN
                {
                    size = sprintf(str, "%s%s%s r%u, r%u, %s", DataProcessing_arm[op], s, Conditions[cond], rd, rm, sstr);
                    break;
                }
                default:
                {
                    size = sprintf(str, "%s%s%s r%u, r%u, r%u, %s", DataProcessing_arm[op], s, Conditions[cond], rd, rn, rm, sstr);
                }
                }
            }
        }
        break;
    }
    case 1: //Data processing and MSR immediate
    {
        if (cond == NV) break; //undefined
        u32 imm = ROR(BITS(c, 0, 8), 2 * BITS(c, 8, 4));

        if (BITS(c, 12, 4) == 15 && !BITS(c, 20, 1)) //MSR immediate
        {
            u8 sreg = BITS(c, 22, 1) ? 's' : 'c'; //SPSR (1) or CPSR (0)
            size = sprintf(str, "msr%s %cpsr_%s, #0x%X", Conditions[cond], sreg, MSR_cxsf[BITS(c, 16, 4)], imm);
        }
        else //Data processing immediate
        {
            u8 op = BITS(c, 21, 4);
            u8 rd = BITS(c, 12, 4);
            u8 rn = BITS(c, 16, 4);
            u8* s = BITS(c, 20, 1) ? "s" : "";
            switch (op)
            {
            case 8: //TST
            case 9: //TEQ
            case 10: //CMP
            case 11: //CMN
                //always update the condition codes: <op>{<cond>} <Rn>, <shift>
            {
                size = sprintf(str, "%s%s r%u, #0x%X", DataProcessing_arm[op], Conditions[cond], rn, imm);
                break;
            }
            case 13: //MOV
            case 15: //MVN
                //only one source operand: <op>{<cond>}{S} <Rd>, <shift>
            {
                size = sprintf(str, "%s%s%s r%u, #0x%X", DataProcessing_arm[op], s, Conditions[cond], rd, imm);
                break;
            }

            default: //others: <op>{<cond>}{S} <Rd>, <Rn>, <shift>
            {
                size = sprintf(str, "%s%s%s r%u, r%u, #0x%X", DataProcessing_arm[op], s, Conditions[cond], rd, rn, imm);
            }
            }
        }
        break;
    }
    case 2: //Load/store immediate offset
    {
        if (cond == NV) break; //undefined
        //bits 25, 24, 23 and 21 decide the addressing mode
        u8 rd = BITS(c, 12, 4);
        u8 rn = BITS(c, 16, 4);
        u16 imm = BITS(c, 0, 12); //12 bits for LDR and LDRB (8 bits for LDRH and LDRSB)
        u8* sign = BITS(c, 23, 1) ? "+" : "-"; //sign of the immediate offset
        u8* ls = BITS(c, 20, 1) ? "ldr" : "str"; //load or store
        u8* b = BITS(c, 22, 1) ? "b" : ""; //byte or word
        if (BITS(c, 24, 1)) //offset or pre-indexed
        {
            u8* w = BITS(c, 21, 1) ? "!" : ""; //pre-indexed if 1
            size = sprintf(str, "%s%s%s r%u, [r%u, #%s0x%X]%s", ls, b, Conditions[cond], rd, rn, sign, imm, w);
        }
        else //post-indexed
        {
            u8* w = BITS(c, 21, 1) ? "t" : ""; //user mode
            size = sprintf(str, "%s%s%s%s r%u, [r%u], #%s0x%X", ls, b, w, Conditions[cond], rd, rn, sign, imm);
        }
        break;
    }
    case 3: //Load/store register offset
    {
        if (cond == NV) break; //undefined
        if (BITS(c, 4, 1)) break; //undefined
        u8 rm = BITS(c, 0, 4);
        u8 shift = BITS(c, 5, 2);
        u16 shift_imm = BITS(c, 7, 5);
        u8 rd = BITS(c, 12, 4);
        u8 rn = BITS(c, 16, 4);
        u8* sign = BITS(c, 23, 1) ? "" : "-"; //sign of the immediate offset, + implicit
        u8* ls = BITS(c, 20, 1) ? "ldr" : "str"; //load or store
        u8* b = BITS(c, 22, 1) ? "b" : ""; //byte or word
        u8 sstr[STRING_LENGTH] = "rrx"; //default, overwrite if incorrect
        if ((shift == 1 || shift == 2) && !shift_imm) shift_imm = 32; //0~31 for LSL, 1~32 for LSR, ASR and ROR, always 0 for RRX
        if (!(shift == 3 && !shift_imm)) sprintf(sstr, "%s #%u", Shifters[shift], shift_imm);
        //note: if rm==r15 or rn==r15 then UNPREDICTABLE
        //note: if rn==rm then UNPREDICTABLE
        if (BITS(c, 24, 1)) //offset or pre-indexed
        {
            u8* w = BITS(c, 21, 1) ? "!" : ""; //pre-indexed if 1
            size = sprintf(str, "%s%s%s r%u, [r%u, %sr%u, %s]%s", ls, b, Conditions[cond], rd, rn, sign, rm, sstr, w);
        }
        else //post-indexed
        {
            u8* w = BITS(c, 21, 1) ? "t" : ""; //user mode
            size = sprintf(str, "%s%s%s%s r%u, [r%u], %sr%u, %s", ls, b, w, Conditions[cond], rd, rn, sign, rm, sstr);
        }
        break;
    }
    case 4: //Load/store multiple
    {
        if (cond == NV) break; //undefined
        u8 reglist[STRING_LENGTH] = { 0 };
        FormatStringRegisterList_arm(reglist, BITS(c, 0, 16));
        u8 rn = BITS(c, 16, 4);
        u8* w = BITS(c, 21, 1) ? "!" : ""; //W bit
        u8* s = BITS(c, 22, 1) ? "^" : ""; //S bit
        u8 am = BITS(c, 23, 2); //PU bits
        if (BITS(c, 20, 1)) //LDM
        {
            size = sprintf(str, "ldm%s%s r%u%s, {%s}%s", Conditions[cond], AddressingModes[am], rn, w, reglist, s);
        }
        else //STM
        {
            size = sprintf(str, "stm%s%s r%u%s, {%s}%s", Conditions[cond], AddressingModes[am], rn, w, reglist, s);
        }
        break;
    }
    case 5: //Branch instructions
    {
        if (cond == NV) //BLX
        {
            size = sprintf(str, "blx #0x%X", 8 + 4 * SIGNEX32_BITS(c, 0, 24) + 2 * BITS(c, 24, 1));
        }
        else //Branch and branch with link
        {
            if (BITS(c, 24, 1)) //BL
            {
                size = sprintf(str, "bl #0x%X", 8 + 4 * SIGNEX32_BITS(c, 0, 24));
            }
            else //B
            {
                size = sprintf(str, "b #0x%X", 8 + 4 * SIGNEX32_BITS(c, 0, 24));
            }
        }
        break;
    }
    case 6: //todo: Coprocessor load/store, Double register transfers 
    {
        //if (cond == NV) break; //only unpredictable prior to ARMv5
        if (BITS(c, 21, 4) == 2) //MCRR, MRRC
        {
            //note: if PC is specified for Rn or Rd, UNPREDICTABLE
            u8* op = BITS(c, 20, 1) ? "mrrc" : "mcrr";
            size = sprintf(str, "%s%s p%u, #0x%X, r%u, r%u, c%u", op, Conditions[cond], BITS(c, 8, 4), BITS(c, 4, 4), BITS(c, 12, 4), BITS(c, 16, 4), BITS(c, 0, 4));
        }
        else //LDC, STD -todo
        {
            u8* op = BITS(c, 20, 1) ? "ldc" : "stc";
            u8* str_cond = (cond == NV) ? "2" : Conditions[cond]; //LDC2, STC2
            u8* l = BITS(c, 22, 1) ? "l" : ""; //long //todo: need to place the "l" after ldc2 but between ldc and cond for example
            u8 str_cond_long[8] = { 0 };
            if (cond == NV) sprintf(str_cond_long, "%s%s", str_cond, l);
            else sprintf(str_cond_long, "%s%s", l, str_cond);
            u8 ofs_opt = BITS(c, 0, 8);
            u8 cp_num = BITS(c, 8, 4);
            u8 crd = BITS(c, 12, 4);
            u8 rn = BITS(c, 16, 4);
            u8* sign = BITS(c, 23, 1) ? "+" : "-";
            switch ((2 * BITS(c, 24, 1)) | BITS(c, 21, 1)) //(p*2) | w
            {
            case 0: //p==0, w==0 //unindexed: [<Rn>], <option>
            {
                size = sprintf(str, "%s%s p%u, c%u, [r%u], {0x%X}", op, str_cond_long, cp_num, crd, rn, ofs_opt);
                break;
            }
            case 1: //p==0, w==1 //post indexed: [<Rn>], #+/-<offset_8>*4
            {
                size = sprintf(str, "%s%s p%u, c%u, [r%u], #%s0x%X", op, str_cond_long, cp_num, crd, rn, sign, 4 * ofs_opt);
                break;
            }
            case 2: //p==1, w==0 //immediate offset: [<Rn>, #+/-<offset_8>*4]
            {
                size = sprintf(str, "%s%s p%u, c%u, [r%u, #%s0x%X]", op, str_cond_long, cp_num, crd, rn, sign, 4 * ofs_opt);
                break;
            }
            case 3: //p==1, w==1 //pre indexed: [<Rn>, #+/-<offset_8>*4]!
            {
                size = sprintf(str, "%s%s p%u, c%u, [r%u, #%s0x%X]!", op, str_cond_long, cp_num, crd, rn, sign, 4 * ofs_opt);
                break;
            }
            }
        }
        break;
    }
    case 7: //Software Interrupt, Coprocessor register transfer, Coprocessor data processing
    {
        if (BITS(c, 24, 1)) //SWI
        {
            if (cond == NV) break;
            size = sprintf(str, "swi%s #0x%X", Conditions[cond], BITS(c, 0, 24));
        }
        else
        {
            u8 crn = BITS(c, 16, 4);
            u8 p = BITS(c, 8, 4);
            u8 rd_crd = BITS(c, 12, 4);
            u8 crm = BITS(c, 0, 4);
            u8 op2 = BITS(c, 5, 3);
            u8* str_cond = (cond == NV) ? "2" : Conditions[cond]; //suffix
            if (BITS(c, 4, 1)) //MCR, MRC
            {
                u8* str_op = BITS(c, 20, 1) ? "mrc" : "mcr";
                size = sprintf(str, "%s%s p%u, #0x%X, r%u, c%u, c%u, #0x%X", str_op, str_cond, p, BITS(c, 21, 3), rd_crd, crn, crm, op2);
            }
            else //CDP
            {
                size = sprintf(str, "cdp%s p%u, #0x%X, c%u, c%u, c%u, #0x%X", str_cond, p, BITS(c, 20, 4), rd_crd, crn, crm, op2); //no condition suffix for cdp2
            }
        }
        break;
    }
    }

    //also: unconditionnal instructions if bits 28 to 31 are 1111
    if (!str[0] && (c & 0xFD70F000) == 0xF550F000) //Cache preload (PLD)
    {
        //todo: placing it here is wasteful, find a workaround
        //bit-pattern: 1111 01x1 x101 xxxx 1111 xxxx xxxx xxxx
        //data mask  : 1111 1101 0111 0000 1111 0000 0000 0000
        //inst. mask : 1111 0101 0101 0000 1111 0000 0000 0000
        //note: only offset addressing modes
        // [<Rn>, #+/-<offset_12>] //immediate
        // [<Rn>, +/-<Rm>] //register
        // [<Rn>, +/-<Rm>, <shift> #<shift_imm>] //scaled register
        u8 rn = BITS(c, 16, 4);
        if (BITS(c, 25, 1)) //(scaled) register
        {
            u8 rm = BITS(c, 0, 4);
            u8 shift = BITS(c, 5, 2);
            u16 shift_imm = BITS(c, 7, 5);
            u8* sign = BITS(c, 23, 1) ? "" : "-";
            u8 sstr[STRING_LENGTH] = "rrx"; //default, overwrite if incorrect
            if ((shift == 1 || shift == 2) && !shift_imm) shift_imm = 32; //0~31 for LSL, 1~32 for LSR, ASR and ROR, always 0 for RRX
            if (!(shift == 3 && !shift_imm)) sprintf(sstr, "%s #%u", Shifters[shift], shift_imm);
            size = sprintf(str, "pld [r%u, %sr%u, %s]", rn, sign, rm, sstr);
        }
        else //immediate
        {
            u8* sign = BITS(c, 23, 1) ? "+" : "-";
            size = sprintf(str, "pld [r%u, #%s0x%X]", rn, sign, BITS(c, 0, 12));
        }
    }

    if (!str[0]) //in case nothing was written to it
    {
        debug_na_count++;
        sprintf(str, "n/a"); //don't need to get size, no special register formatting
    }
    else
    {
        CheckSpecialRegister(str, size); //formatting of SP, LR, PC
    }
}

static void Debug_DisassembleCode_arm(u32 c) {
    /* Debug: disassemble a single ARM instruction from the ARMv5TE architecture */
    u8 s[STRING_LENGTH] = { 0 };
    Disassemble_arm(c, s, ARMv5TE);
    printf("%08X -> %s\n", c, s);
}

static void Debug_DumpAllInstructions(void) {
    /* Debug: call only when you want to dump the complete THUMB instruction set to a file */

    FILE* fp = fopen("ARMv5TE_THUMB_instruction_set.txt", "w+");

    for (u32 code = 0; code <= 0xffff; code++)
    {
        u8 str[STRING_LENGTH] = { 0 };
        Disassemble_thumb(code, str, ARMv5TE);
        fprintf(fp, "%04X %s\n", code, str);
    }

    fclose(fp);

    printf("N/A instructions remaining: %u (%u%% done)\n", debug_na_count, 100 - 100 * debug_na_count / 65536);
}

static int GetFileSize_mine(FILE* fp) {
    /* Return the size of an opened file */
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    rewind(fp);
    return size;
}

static int DisassembleFile(FILE* in, FILE* out, FILERANGE* range) {
    /* Disassemble from a binary file, print to another file */
    int size = GetFileSize_mine(in);
    if (range->start > size || range->end > size) return 0;
    if (range->end && (range->end < size))
    {
        size = range->end;
    }
    fprintf(out, "Disassembly of %u bytes:\n\n", range->end - range->start);

    fseek(in, range->start, SEEK_SET);

    for (int i = range->start; i < (range->start + size) / 2; i++)
    {
        u8 str[STRING_LENGTH] = { 0 };
        u32 code = 0;
        fread(&code, 4, 1, in); //prefetch 32 bits
        if (Disassemble_thumb(code, str, ARMv5TE)) //32-bit
        {
            fprintf(out, "%08X: %08X %s\n", range->start + (i - range->start) * 2, code, str);
            i++;
        }
        else //16-bit
        {
            fseek(in, -2, SEEK_CUR); //go back 2 bytes
            fprintf(out, "%08X: %04X     %s\n", range->start + (i - range->start) * 2, code & 0xffff, str);
        }
    }

    fprintf(out, "\n%u unknown instructions.\n", debug_na_count);
    return 1; //success
}

static int IsValidPath(u8* path) {
    /* Check if length of path/filename is */
    //todo: check if you can also open it
    u32 hasDotAndEom = 0;
    if (!path || !path[0]) return 0; //needs to be at least one char
    for (u32 i = 1; i < PATH_LENGTH; i++)
    {
        if (path[i] == '.') hasDotAndEom = 1;
        if (!path[i])
        {
            hasDotAndEom++;
            break; //found EOM
        }
    }
    if (hasDotAndEom == 2) return 1;
    return 0;
}

static int IfValidRangeSet(FILERANGE* range, u8* r) {
    /* Check to see if the range string yields a valid FILERANGE */
    //Acceptable formats:
    //"0x%X-0x%X" //start to end
    //"--0x%X" //unspecified start (default to beginning of file, 0) to end
    //"0x%X--" //start to unspecified end (default to end of file)

    u32 start = 0;
    u32 end = 0;

    if (!r || !r[0]) return 0; //needs to be at least one char

    /* Init buffer (memcpy with two limit conditions) */
    u8 str[RANGE_LENGTH] = { 0 };
    u32 i = 0;
    while (r[i] && (i < RANGE_LENGTH - 1)) //must be null terminated
    {
        str[i] = r[i];
        i++;
    }

    if (*(u16*)str == 0x2d2d) //detect double dash
    {
        end = strtol(&str[2], NULL, 16);
    }
    else
    {
        for (u32 i = 0; i < RANGE_LENGTH; i++)
        {
            if (str[i] == '-') //first dash
            {
                start = strtol(str, NULL, 16);
                end = strtol(&str[i + 1], NULL, 16);
                break;
            }
        }
    }

    if (end && (start > end))
    {
        printf("WARNING: If END is specified and non-zero, it cannot be greater than START. The whole file will be disassembled.\n");
        return 0; //start can't be greater than end if end is non-zero
    }

    /* Success, set */
    range->start = start;
    range->end = end;
    return 1;
}

/* ENTRY POINT */

//#define DEBUG //comment out to disable debug ifdefs

int main(int argc, char* argv[]) {

    clock_t start = clock();


#ifdef DEBUG
    //Debug_DumpAllInstructions();
    Debug_DisassembleCode_arm(0xfd40d1ff);
    //u32 i = 0;
    //while (++i != 0xffffff) //4654 ms with variable size, 5137 ms with size == 64
    //{
    //    Debug_DisassembleCode_arm(i);
    //}
    //printf("%u n/a instructions, %.2f%% complete.\n", debug_na_count, 100 - (100 * (float)debug_na_count / (float)0x100000000));

#else

    //todo: ask for ARM or THUMB mode, or auto detect, or do both
    u8* filename_in = argv[1];
    u8* filename_out = NULL;
    FILE* file_in = NULL;
    FILE* file_out = NULL;
    FILERANGE filerange = { 0 };

    if (IsValidPath(filename_in)) //file in
    {
        file_in = fopen(filename_in, "rb");
        if (file_in == NULL)
        {
            printf("ERROR: The file \"%s\" doesn't exist. Aborting.\n", filename_in);
            return 0;
        }

        if (IsValidPath(argv[2])) //file out
        {
            filename_out = argv[2];
            file_out = fopen(filename_out, "w+");
            if (file_out == NULL) return 0; //couldn't create file for some reason

            IfValidRangeSet(&filerange, argv[3]);
            printf("Starting disassembly of \"%s\".\n", filename_in);
            if (DisassembleFile(file_in, file_out, &filerange))
            {
                printf("Successfully disassembled \"%s\" to \"%s\".\n", filename_in, filename_out);
            }
            else
            {
                printf("ERROR: DisassembleFile failed.\n");
            }
            fclose(file_in);
            fclose(file_out);
        }
        else //stdout
        {
            IfValidRangeSet(&filerange, argv[2]);
            printf("Starting disassembly of \"%s\".\n", filename_in);
            if (DisassembleFile(file_in, stdout, &filerange))
            {
                printf("Successfully disassembled \"%s\".\n", filename_in);
            }
            else
            {
                printf("ERROR: DisassembleFile failed.\n");
            }
            fclose(file_in);
        }
        return 0;
    }

    printf("Nothing was done\n");
#endif // DEBUG

    printf("Completion time: %.0f ms\n", (double)clock() - (double)start);

    return 0;
}
