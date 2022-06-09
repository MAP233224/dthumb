#include <stdio.h>
#include <string.h>

//#define _CRT_SECURE_NO_WARNINGS 1

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define PATH_LENGTH (256)
#define STRING_LENGTH (64)
#define CONDITIONS_MAX (16)
#define BITS(x, b, n) ((x >> b) & ((1 << n) - 1)) //retrieves n bits from x starting at b
#define SIGNEX32_BITS(x, b, n) ((BITS(x,b,n) ^ (1<<(n-1))) - (1<<(n-1))) //convert n-bit value to signed 32 bits
#define SIGNEX32_VAL(x, n) ((x ^ (1<<(n-1))) - (1<<(n-1))) //convert n-bit value to signed 32 bits

typedef enum {
    ARMv4T, //ARM v4, THUMB v1
    ARMv5TE, //ARM v5, THUMB v2
    ARMv6, //ARM v6, THUMB v3
    //ARMv7
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
    AL2 //unconditional
}CONDITION;

const u8 IT_xyz_0[CONDITIONS_MAX][4] = { //inverse of the one above
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


const u8 ConditionFlags[CONDITIONS_MAX][3] = { "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le", "al", "" };

const u8 DataProcessingRegister[16][4] = {
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

const u8 MovAddSubImmediate[4][4] = { "mov","cmp","add","sub" }; //cmp won't be used
const u8 LoadStoreRegister[8][6] = { "str", "strh", "strb", "ldrsb", "ldr", "ldrh", "ldrb", "ldrsh" };
const u8 CPS_effect[2][3] = { "ie","id" };
const u8 CPS_flags[8][5] = { "none", "f", "i", "if", "a", "af", "ai", "aif" };
const u8 SignZeroExtend[4][5] = { "sxth","sxtb","uxth","uxtb" };

//const u8 ShiftImmediate[3][4] = { "lsl", "lsr","asr" };


u32 debug_na_count = 0;

/* FUNCTIONS */

u32 FormatStringRegisterList(u8 str[48], u16 reg, const u8 pclr[3]) {
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


u32 CountBits(u8 b) {
    /* Count bits in a byte */
    static const u8 lut[16] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };
    return lut[b & 0x0f] + lut[b >> 4];
}

void IfThen_imm_2(const u8* op, u8 str[STRING_LENGTH], u32 it, const u8* cond, u8 rm, u8 imm) {
    /* Formatting */
    if (it) sprintf(str, "%s%s r%u, #%X", op, cond, rm, imm);
    else sprintf(str, "%ss r%u, #%X", op, rm, imm);
}

void IfThen_imm_3(const u8* op, u8 str[STRING_LENGTH], u32 it, const u8* cond, u8 rd, u8 rm, u8 imm) {
    /* Formatting */
    if (it) sprintf(str, "%s%s r%u, r%u, #%X", op, cond, rd, rm, imm);
    else sprintf(str, "%ss r%u, r%u, #%X", op, rd, rm, imm);
}

void IfThen_reg_2(const u8* op, u8 str[STRING_LENGTH], u32 it, const u8* cond, u8 rd, u8 rm) {
    /* Formatting */
    if (it) sprintf(str, "%s%s r%u, r%u", op, cond, rd, rm);
    else sprintf(str, "%ss r%u, r%u", op, rd, rm);
}

void IfThen_reg_3(const u8* op, u8 str[STRING_LENGTH], u32 it, const u8* cond, u8 rd, u8 rm, u8 rn) {
    /* Formatting */
    if (it) sprintf(str, "%s%s r%u, r%u, r%u", op, cond, rd, rm, rn);
    else sprintf(str, "%ss r%u, r%u, r%u", op, rd, rm, rn);
}

u16 Fetch(void) {
    /* Gets the next 16-bit instruction in line */
    return 0;
}

u32 Disassemble(u32 code, u32 fromstart, u8 str[STRING_LENGTH], u32 it, const u8* cond, ARMARCH tv) {
    /*  */

    u32 thumb_size = 0; //return 0 if processed THUMB 16-bit, 1 if processed THUMB 32-bit
    u16 c = code & 0xffff; //

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
                    IfThen_imm_3("lsl", str, it, cond, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
                }
                else //MOV register
                {
                    sprintf(str, "movs r%u, r%u", BITS(c, 0, 3), BITS(c, 3, 3));
                }
            }
            else //ARMv4T
            {
                sprintf(str, "lsl r%u, r%u, #%X", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
            }
            break;
        }
        case 1: //LSR immediate
        {
            if (tv >= ARMv5TE)
            {
                IfThen_imm_3("lsr", str, it, cond, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
            }
            else //ARMv4T
            {
                sprintf(str, "lsr r%u, r%u, #%X", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
            }
            break;
        }
        case 2: //ASR immediate
        {
            if (tv >= ARMv5TE)
            {
                IfThen_imm_3("asr", str, it, cond, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
            }
            else //ARMv4T
            {
                sprintf(str, "asr r%u, r%u, #%X", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
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
                        IfThen_imm_3("sub", str, it, cond, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                    else //ARMv4T
                    {
                        sprintf(str, "sub r%u, r%u, #%X", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                }
                else //ADD immediate
                {
                    if (tv >= ARMv5TE)
                    {
                        IfThen_imm_3("add", str, it, cond, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                    else //ARMv4T
                    {
                        sprintf(str, "add r%u, r%u, #%X", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                }
            }
            else //ADD/SUB register
            {
                if (BITS(c, 9, 1)) //SUB register
                {
                    if (tv >= ARMv5TE)
                    {
                        IfThen_reg_3("sub", str, it, cond, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                    else //ARMv4T
                    {
                        sprintf(str, "sub r%u, r%u, r%u", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                }
                else //ADD register
                {
                    if (tv >= ARMv5TE)
                    {
                        IfThen_reg_3("add", str, it, cond, BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
                    }
                    else //ARMv4T
                    {
                        sprintf(str, "add r%u, r%u, r%u", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3));
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
        if (index == 1) sprintf(str, "cmp r%u, #%X", BITS(c, 8, 3), BITS(c, 0, 8));
        else IfThen_imm_2(MovAddSubImmediate[index], str, it, cond, BITS(c, 8, 3), BITS(c, 0, 8));
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
                    if (m == 13) sprintf(str, "add r%u, sp, r%u", d, d); //ADD sp + register v1
                    else sprintf(str, "add r%u, r%u", d, m); //ADD
                }
                break;
            }
            case 1: //CMP high register
            {
                if (tv >= ARMv5TE)
                {
                    sprintf(str, "cmp r%u, r%u", (BITS(c, 7, 1) << 3) | (BITS(c, 0, 3)), BITS(c, 3, 4));
                }
                break;
            }
            case 2: //MOV register
            {
                sprintf(str, "mov r%u, r%u", (BITS(c, 7, 1) << 3) | (BITS(c, 0, 3)), BITS(c, 3, 4));
                break;
            }
            case 3:
            {
                if (BITS(c, 7, 1)) sprintf(str, "blx r%u", BITS(c, 3, 4)); //BLX
                else sprintf(str, "bx r%u", BITS(c, 3, 4)); //BX
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
                sprintf(str, "%s r%u, r%u", DataProcessingRegister[index], BITS(c, 0, 3), BITS(c, 3, 3));
                break;
            }
            default:
            {
                IfThen_reg_2(DataProcessingRegister[index], str, it, cond, BITS(c, 0, 3), BITS(c, 3, 3));
            }
            }
        }
        else
        {
            if (BITS(c, 12, 1)) sprintf(str, "%s r%u, [r%u, r%u]", LoadStoreRegister[BITS(c, 9, 3)], BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 3)); //Load/store register offset  
            else sprintf(str, "ldr r%u, [pc, #%X]", BITS(c, 8, 3), 4 * BITS(c, 0, 8)); //LDR literal pool
        }
        break;
    }

    case 3: //0x6000 //STR, LDR, STRB, LDRB
    {
        switch (BITS(c, 11, 2))
        {
        case 0: //STR
        {
            sprintf(str, "str r%u, [r%u, #%X]", BITS(c, 0, 3), BITS(c, 3, 3), 4 * BITS(c, 6, 5));
            break;
        }
        case 1: //LDR
        {
            sprintf(str, "ldr r%u, [r%u, #%X]", BITS(c, 0, 3), BITS(c, 3, 3), 4 * BITS(c, 6, 5));
            break;
        }
        case 2: //STRB
        {
            sprintf(str, "strb r%u, [r%u, #%X]", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
            break;
        }
        case 3: //LDRB
        {
            sprintf(str, "ldrb r%u, [r%u, #%X]", BITS(c, 0, 3), BITS(c, 3, 3), BITS(c, 6, 5));
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
                if (BITS(c, 11, 1)) sprintf(str, "ldr r%u, [sp, #%X]", BITS(c, 8, 3), 4 * BITS(c, 0, 8)); //LDR stack
                else sprintf(str, "str r%u, [sp, #%X]", BITS(c, 8, 3), 4 * BITS(c, 0, 8)); //STR stack
            }
        }
        else
        {
            if (BITS(c, 11, 1)) sprintf(str, "ldrh r%u, [r%u, #%X]", BITS(c, 0, 3), BITS(c, 3, 3), 2 * BITS(c, 6, 5)); //LRDH immediate offset
            else sprintf(str, "strh r%u, [r%u, #%X]", BITS(c, 0, 3), BITS(c, 3, 3), 2 * BITS(c, 6, 5)); //STRH immediate offset
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
                if (BITS(c, 7, 1)) sprintf(str, "sub sp, sp, #%X", 4 * BITS(c, 0, 7));  //SUB             
                else sprintf(str, "add sp, sp, #%X", 4 * BITS(c, 0, 7)); //ADD
                break;
            }
            //Compare and branch on (non-)zero
            case 1:
            case 3:
            case 9:
            case 11:
            {
                if (BITS(c, 11, 1)) sprintf(str, "cbnz r%u, #%X", BITS(c, 0, 3), 4 + 2 * ((BITS(c, 9, 1) << 5) | BITS(c, 3, 5))); //CBNZ           
                else sprintf(str, "cbz r%u, #%X", BITS(c, 0, 3), 4 + 2 * ((BITS(c, 9, 1) << 5) | BITS(c, 3, 5))); //CBZ               
                break;
            }
            case 2: //Sign/zero extend
            {
                if (tv >= ARMv6)
                {
                    sprintf(str, "%s r%u, r%u", SignZeroExtend[BITS(c, 6, 2)], BITS(c, 0, 3), BITS(c, 3, 3));
                }
                break;
            }
            //PUSH/POP
            case 4:
            case 5:
            case 12:
            case 13:
            {
                u8 reglist[48] = { 0 };
                u16 registers = BITS(c, 0, 9); //registers = P:'0000000':register_list
                if (BITS(c, 11, 1)) //POP
                {
                    if (FormatStringRegisterList(reglist, registers, "pc")) //if BitCount(registers) < 1 then UNPREDICTABLE
                    {
                        sprintf(str, "pop {%s}", reglist); //todo: format register list
                    }
                }
                else //PUSH
                {
                    if (FormatStringRegisterList(reglist, registers, "lr")) //if BitCount(registers) < 1 then UNPREDICTABLE
                    {
                        sprintf(str, "push {%s}", reglist); //todo: format register list
                    }
                }
                break;
            }
            case 6: //SETEND and CPS
            {
                if (tv >= ARMv6)
                {
                    if (c == 0xB650) sprintf(str, "setend le");
                    else if (c == 0xB658) sprintf(str, "setend be");
                    else if (BITS(c, 4, 8) == 100); //n/a
                    else if (BITS(c, 5, 7) == 51)
                    {
                        if (!BITS(c, 3, 1)) sprintf(str, "cps%s %s", CPS_effect[BITS(c, 4, 1)], CPS_flags[BITS(c, 0, 3)]); //CPS
                    }
                }
                break;
            }
            case 10: //Reverse byte
            {
                if (tv >= ARMv6)
                {
                    switch (BITS(c, 6, 2))
                    {
                    case 0: //REV
                    {
                        sprintf(str, "rev r%u, r%u", BITS(c, 0, 3), BITS(c, 3, 3));
                        break;
                    }
                    case 1: //REV16
                    {
                        sprintf(str, "rev16 r%u, r%u", BITS(c, 0, 3), BITS(c, 3, 3));
                        break;
                    }
                    case 2: //undefined
                    {
                        break;
                    }
                    case 3: //REVSH
                    {
                        sprintf(str, "revsh r%u, r%u", BITS(c, 0, 3), BITS(c, 3, 3));
                        break;
                    }
                    }
                }
                break;
            }
            case 14: //BKPT
            {
                sprintf(str, "bkpt #%X", BITS(c, 0, 8));
                break;
            }
            case 15: //IT and NOP-hints
            {
                u8 mask = BITS(c, 0, 4);
                if (mask) //IT
                {
                    u8 firstcond = BITS(c, 4, 4);
                    if (firstcond == 15); //n/a
                    else if (firstcond == 14 && CountBits(mask) != 1); //n/a
                    else
                    {
                        if (firstcond & 1) sprintf(str, "it%s %s", IT_xyz_1[mask], ConditionFlags[firstcond]);
                        else sprintf(str, "it%s %s", IT_xyz_0[mask], ConditionFlags[firstcond]);
                    }
                }
                else //NOP-compatible hints
                {
                    u8 hint = BITS(c, 4, 4);
                    switch (hint)
                    {
                    case 0: sprintf(str, "nop"); break;
                    case 1: sprintf(str, "yield"); break;
                    case 2: sprintf(str, "wfe"); break;
                    case 3: sprintf(str, "wfi"); break;
                    case 4: sprintf(str, "sev"); break;
                    default: sprintf(str, "hint #%X", hint);
                    }
                }
                break;
            }
            }
        }
        else //add to SP or PC
        {
            if (BITS(c, 11, 1)) sprintf(str, "add r%u, sp, #%X", BITS(c, 8, 3), 4 * BITS(c, 0, 8)); //ADD (SP plus immediate)
            else sprintf(str, "adr r%u, #%X", BITS(c, 8, 3), 4 * BITS(c, 0, 8)); //ADR (pc)
        }
        break;
    }

    case 6: //0xC000 //B, SVC, LDMIA, STMIA
    {
        if (BITS(c, 12, 1)) //Conditional branch, Undefined, System call
        {
            switch (BITS(c, 8, 4))
            {
            case 14: //UDF "Permanently undefined space", OS dependant
            {
                sprintf(str, "udf #%X", BITS(c, 0, 8));
                break;
            }
            case 15: //SVC
            {
                sprintf(str, "svc #%X", BITS(c, 0, 8));
                break;
            }
            default: //B conditional
            {
                sprintf(str, "b%s #%X", ConditionFlags[BITS(c, 8, 4)], 4 + 2 * (fromstart + SIGNEX32_BITS(c, 0, 8)));
            }
            }
        }
        else //Load/store multiple
        {
            u8 reglist[48] = { 0 };
            u16 registers = BITS(c, 0, 8);
            u8 rn = BITS(c, 8, 3);
            if (BITS(c, 11, 1)) //LDMIA (LDMFD)
            {
                if (FormatStringRegisterList(reglist, registers, ""))
                {
                    if (BITS(registers, rn, 1)) sprintf(str, "ldmia r%u, {%s}", rn, reglist); //no '!' because rn is also in registers
                    else sprintf(str, "ldmia r%u!, {%s}", rn, reglist);
                }
            }
            else //STMIA (STMEA)
            {
                if (FormatStringRegisterList(reglist, registers, ""))
                {
                    sprintf(str, "stmia r%u!, {%s}", rn, reglist);
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
            sprintf(str, "b #%X", 4 + 2 * (fromstart + SIGNEX32_BITS(c, 0, 11))); //11 bits to signed 32 bits
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
                        sprintf(str, "blx #%X", 4 + fromstart + SIGNEX32_VAL(ofs, 22));
                    }
                    break;
                }
                //case 2: break; //BL/BLX prefix
                case 3: //BL suffix
                {
                    thumb_size++;
                    u32 ofs = ((BITS((code & 0xffff), 0, 10) << 11) | (BITS(c, 0, 11))) << 1;
                    sprintf(str, "bl #%X", 4 + fromstart + (int)SIGNEX32_VAL(ofs, 22));
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
    return thumb_size;
}

void SubstituteSubString(u8 dst[STRING_LENGTH], u32 index, const u8* sub, u32 size) {
    /* Insert sub string of length size (< STRING_LENGTH) at dst[index] */
    u8 tmp[STRING_LENGTH] = { 0 };
    memcpy(&tmp, sub, size);
    memcpy(&tmp[size], &dst[index + size + 1], STRING_LENGTH - index - size - 1); //save
    memcpy(&dst[index], tmp, STRING_LENGTH - index - 1); //restore
}

void CheckSpecialRegister(u8 str[STRING_LENGTH]) {
    /* Replace special registers by their common names */
    //(r12->ip), r13->sp, r14->lr, r15->pc
    for (u32 i = 0; i < STRING_LENGTH; i++)
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

//Nintendo DS CPU: ARM946E-S
//Architecture: ARMv5TE
//ARM version: 5
//THUMB version: 2

/*
Instructions available in ARMv6 but not ARMv5TE:

ARM: BXJ, CPS, CPY, MCRR2, MRRC2, PKH, QADD16, QADD8, QADDSUBX, QSUB16, QSUB8, QSUBADDX, REV, RFE, SADD, SEL, SETEND, SHADD, SHSUB, SMLAD,
SMLALD, SMLSD, SMLSLD, SMMLA, SMMLS, SMMUL, SMUAD, SMUSD, SRS, SSAT, SSUB, STREX, SXT, UADD, UHADD, UQADD, UQSUB, USAD, USAT, USUB, UXT,

THUMB: CPS, CPY, REV, SETEND, SXTB, SXTH, UXTB, UXTH

*/


void DumpAllInstructions(void) {

    FILE* fp = fopen("ARMv5TE_THUMB_instruction_set.txt", "w+");

    for (u32 code = 0; code <= 0xffff; code++)
    {
        u8 str[STRING_LENGTH] = { 0 };
        Disassemble(code, 0, str, 0, "", ARMv5TE);
        CheckSpecialRegister(str);
        fprintf(fp, "%04X %s\n", code, str);
    }

    fclose(fp);

    printf("N/A instructions remaining: %u (%u%% done)\n", debug_na_count, 100 - 100 * debug_na_count / 65536);
}

int GetFileSize_mine(FILE* fp) {
    /* Return the size of an opened file */
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    rewind(fp);
    return size;
}

int DisassembleFile_stdout(FILE* fp) {
    /**/
    int size = GetFileSize_mine(fp);
    printf("Disassembly of %u bytes:\n\n", size);

    for (int i = 0; i < size / 2; i++)
    {
        u8 str[STRING_LENGTH] = { 0 };
        u32 code = 0;
        fread(&code, 4, 1, fp); //prefetch 32 bits
        if (Disassemble(code, 0, str, 0, "", ARMv5TE)) //32-bit
        {
            CheckSpecialRegister(str);
            printf("%08X: %08X %s\n", i * 2, code, str);
            i++;
        }
        else //16-bit
        {
            fseek(fp, -2, SEEK_CUR); //go back 2 bytes
            CheckSpecialRegister(str);
            printf("%08X: %04X %s\n", i * 2, code & 0xffff, str);
        }
    }

    printf("\n%u unknown instructions.\n", debug_na_count);
    fclose(fp);
    return 1; //success
}

int DisassembleFile_fileout(FILE* in, FILE* out) {
    /**/
    int size = GetFileSize_mine(in);
    fprintf(out, "Disassembly of %u bytes:\n\n", size);

    for (int i = 0; i < size / 2; i++)
    {
        u8 str[STRING_LENGTH] = { 0 };
        u32 code = 0;
        fread(&code, 4, 1, in); //prefetch 32 bits
        if (Disassemble(code, 0, str, 0, "", ARMv5TE)) //32-bit
        {
            CheckSpecialRegister(str);
            fprintf(out, "%08X: %08X %s\n", i * 2, code, str);
            i++;
        }
        else //16-bit
        {
            fseek(in, -2, SEEK_CUR); //go back 2 bytes
            CheckSpecialRegister(str);
            fprintf(out, "%08X: %04X %s\n", i * 2, code & 0xffff, str);
        }
    }

    fprintf(out, "\n%u unknown instructions.\n", debug_na_count);
    fclose(in);
    fclose(out);
    return 1; //success
}

int IsValidPath(u8* path) {
    /* Check if length of path/filename is */
    //todo: check if you can also open it
    if (!path || !path[0]) return 0; //needs to be at least one char
    for (u32 i = 1; i < PATH_LENGTH; i++)
    {
        if (!path[i]) return 1; //found EOM
    }
    return 0;
}

typedef struct {
    u32 start; //starting address
    u32 end; //end address
}FILERANGE;


int IsValidIntString_hex(u8* b, u32 size) {
    /* Valid characters are 0~9 and A~F */
    for (u8 i = 0; i < size; i++) {
        if (b[i] < '0' || b[i] > 'F') return 0;
        if (b[i] > '9' && b[i] < 'A') return 0;
    }
    return 1;
}

int IsValidHexChar(u8 b) {
    /* Valid characters are 0~9 and A~F */
    if (b == '-') return 2; //dash special case
    if (b < '0' || b > 'F') return 0;
    if (b > '9' && b < 'A') return 0;
    return 1;
}

u8 HexLetterToNumber(u8 n) {
    /* Virtually apends the capital letters right after 9 in the ASCII table */
    switch (n) {
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        return n - 7;
    default:
        return n;
    }
}

int IfValidRangeSet(FILERANGE* range, u8* r) {
    /* Check to see if the range string yields a valid FILERANGE */
    //acceptable formats:
    //"%x-%x" //start to end
    //"--%x" //unspecified start (default to beginning of file, 0) to end
    //"%x--" //start to unspecified end (default to end of file)

    //todo: complete

    u8 dashcount = 0;
    u32 address = 0;
    u32 start = 0;
    u32 end = 0;

    if (!r || !r[0]) return 0; //needs to be at least one char
    for (u32 i = 0; i < 18; i++) //"01234567-89abcdef" is 18 chars (null terminator included)
    {
        if (!r[i]) break; //EOM

        switch (IsValidHexChar(r[i]))
        {
        case 0: return 0; //not a valid hex char
        case 1: //regular digit
        {
            address = address * 16 + HexLetterToNumber(r[i]);
            break;
        }
        case 2: //dash
        {
            switch (dashcount)
            {
            case 0:
            {
                start = address; //todo: substract ascii from address before assigning
                address = 0;
                break;
            }
            case 1:
            {
                end = address; //todo: substract ascii from address before assigning
                address = 0;
                break;
            }
            default: return 0; //can't have more than 2 dashes
            }
            dashcount++;
            break;
        }
        }
    }

    if (start > end || start == end) return 0;

    /* Success, set */
    range->start = start;
    range->end = end;
    return 1;
}


int main(int argc, char* argv[]) {

    //DumpAllInstructions();



    u8* filename_in = argv[1];

    if (IsValidPath(filename_in)) //file in
    {
        FILE* file_in = fopen(filename_in, "rb");
        if (file_in == NULL)
        {
            printf("The file \"%s\" doesn't exist.\n", filename_in);
            return 0;
        }

        u8* filename_out = argv[2];

        //todo: argv[3] start-end address disassembly of filein
        FILERANGE filerange = { 0 };
        if (IfValidRangeSet(&filerange, argv[3]))
        {
            printf("IfValidRangeSet returned 1.\n");
        }
        else
        {
            printf("IfValidRangeSet returned 0.\n");
        }


        if (IsValidPath(filename_out)) //file out
        {
            FILE* file_out = fopen(filename_out, "w+");
            if (file_out == NULL) return 0; //couldn't create file




            printf("Starting disassembly of \"%s\".\n", filename_in);
            if (DisassembleFile_fileout(file_in, file_out))
            {
                printf("Successfully disassembled \"%s\" to \"%s\".\n", filename_in, filename_out);
            }
            else
            {
                printf("Error disassembling \"%s\" to \"%s\".\n", filename_in, filename_out);
            }
        }
        else //stdout
        {
            printf("Starting disassembly of \"%s\".\n", filename_in);
            if (DisassembleFile_stdout(file_in))
            {
                printf("Successfully disassembled \"%s\".\n", filename_in);
            }
            else
            {
                printf("Error disassembling \"%s\".\n", filename_in);
            }
        }
        return 0;
    }

    printf("Nothing was done\n");
    return 0;
}
