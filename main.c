#include "dthumb.h"

/* DEBUG FUNCTIONS */

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

/* COMMAND LINE UTILITY STRUCTS, ENUMS AND FUNCTIONS */

typedef enum {
    DARM,
    DTHUMB
}DMODE;

typedef enum {
    DARGS_INVALID,
    DARGS_SINGLE,
    DARGS_STDOUT,
    DARGS_FILEOUT
}DARGS_STATUS;

typedef struct {
    int start; //starting address
    int end; //end address
}FILERANGE;

typedef struct {
    u8* fname_in;
    u8* fname_out;
    FILERANGE frange;
    DMODE dmode;
    u32 code;
}DARGS;

static int GetFileSize_mine(FILE* fp) {
    /* Return the size of an opened file */
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    rewind(fp);
    return size;
}

static int DisassembleFile(FILE* in, FILE* out, FILERANGE* range, DMODE dmode) {
    /* Disassemble from a binary file, print to another file */
    int size = GetFileSize_mine(in);
    if (range->start > size || range->end > size) return 0; //out of range
    if (range->end == 0) range->end = size;
    size = range->end - range->start;
    fprintf(out, "Disassembly of %u (0x%X) bytes:\n\n", size, size);

    fseek(in, range->start, SEEK_SET);

    if (dmode == DARM)
    {
        for (int i = 0; i < size / 4; i++)
        {
            u8 str[STRING_LENGTH] = { 0 };
            u32 code = 0;
            fread(&code, 4, 1, in); //read 32 bits
            Disassemble_arm(code, str, ARMv5TE);
            fprintf(out, "%08X: %08X %s\n", range->start + i * 4, code, str);
        }
    }
    else //DTHUMB
    {
        for (int i = 0; i < size / 2; i++)
        {
            u8 str[STRING_LENGTH] = { 0 };
            u32 code = 0;
            fread(&code, 4, 1, in); //prefetch 32 bits
            if (Disassemble_thumb(code, str, ARMv5TE) == SIZE_32) //32-bit
            {
                fprintf(out, "%08X: %08X %s\n", range->start + i * 2, code, str);
                i++;
            }
            else //16-bit
            {
                fseek(in, -2, SEEK_CUR); //go back 2 bytes
                fprintf(out, "%08X: %04X     %s\n", range->start + i * 2, code & 0xffff, str);
            }
        }
    }

    fprintf(out, "\n%u unknown instructions.\n", debug_na_count);
    return 1; //success
}

static void DisassembleSingle(u32 code, DMODE dmode) {
    //todo: disassemble a single code
    u8 str[STRING_LENGTH] = { 0 };
    if (dmode == DARM)
    {
        Disassemble_arm(code, str, ARMv5TE);
        printf("%08X %s\n", code, str);
    }
    else
    {
        if (Disassemble_thumb(code, str, ARMv5TE) == SIZE_32) //32-bit
        {
            printf("%08X %s\n", code, str);
        }
        else //16-bit
        {
            printf("%04X     %s\n", code & 0xffff, str);
        }
    }
}

static int IsValidPath(u8* path) {
    /* Check if length of path/filename is */
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

static int IfValidCodeSet(u32* code, u8* str) {
    /* Convert string to 32-bit hex value */
    if (strlen(str) > 8) return 0;
    *code = (u32)strtoll(str, NULL, 16);
    return 1;
}

static int IfValidRangeSet(FILERANGE* range, u8* r) {
    /* Check to see if the range string yields a valid FILERANGE */
    //Acceptable formats:
    //"X-X" //start to end
    //"--X" //unspecified start (default to beginning of file, 0) to end
    //"X--" //start to unspecified end (default to end of file)

    u32 start = 0;
    u32 end = 0;
    u32 valid = 0;

    if (!r || !r[0] || r[0] == '/') return 0; //needs to be at least one char, +smart abort for detecting "/a"

    /* Init buffer (memcpy with two limit conditions) */
    u8 str[RANGE_LENGTH] = { 0 };
    u32 i = 0; //count chars
    while (r[i] && (i < RANGE_LENGTH - 1)) //must be null terminated
    {
        str[i] = r[i];
        i++;
    }

    if (*(u16*)str == 0x2d2d) //detect double dash
    {
        end = strtol(&str[2], NULL, 16);
        valid = 1;
    }
    else
    {
        for (u32 j = 0; j < i; j++)
        {
            if (str[j] == '-') //first dash
            {
                start = strtol(str, NULL, 16);
                end = strtol(&str[j + 1], NULL, 16);
                valid = 1;
                break;
            }
        }
    }

    if (!valid) return 0;

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

static int IfValidModeSet(DMODE* dmode, u8* m) {
    /* Only check first two chars to change from default DTHUMB to DARM mode */
    if (!m || !m[0]) return 0; //needs to be at least one char
    if (*(u16*)m == 0x612f) //"/a"
    {
        *dmode = DARM;
        return 1;
    }
    return 0;
}

static int ParseCommandLineArguments(DARGS* dargs, int argc, char* argv[]) {
    /* You can pass arguments in any order, but they need to be valid */
    /* fname_in has to be valid, else return 0 (failed) */
    /* The other arguments can be invalid, default behavior is handled */

    //todo: make better use of argc to know which argv you can read from (careful with invalid pointers, oob reads)

    if (argc > 5) return DARGS_INVALID;

    if (IsValidPath(argv[1])) //filein
    {
        dargs->fname_in = argv[1];
        if (IsValidPath(argv[2])) //fileout
        {
            dargs->fname_out = argv[2];
            if (IfValidRangeSet(&dargs->frange, argv[3]))
            {
                IfValidModeSet(&dargs->dmode, argv[4]);
            }
            else if (IfValidModeSet(&dargs->dmode, argv[3]))
            {
                IfValidRangeSet(&dargs->frange, argv[4]);
            }
            return DARGS_FILEOUT;
        }
        else //stdout
        {
            if (IfValidRangeSet(&dargs->frange, argv[2]))
            {
                IfValidModeSet(&dargs->dmode, argv[3]);
            }
            else if (IfValidModeSet(&dargs->dmode, argv[2]))
            {
                IfValidRangeSet(&dargs->frange, argv[3]);
            }
            return DARGS_STDOUT;
        }
    }
    else if (IfValidCodeSet(&dargs->code, argv[1])) //single code
    {
        IfValidModeSet(&dargs->dmode, argv[2]);
        return DARGS_SINGLE;
    }
    return DARGS_INVALID;
}

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
    printf("Completion time: %.0f ms\n", (double)clock() - (double)start);
#else

    DARGS dargs = { NULL, NULL, {0}, DTHUMB };
    DARGS_STATUS ds = ParseCommandLineArguments(&dargs, argc, argv);

    switch (ds)
    {
    case DARGS_INVALID:
    {
        printf("Nothing was done\n");
        return 0; //terminate
    }
    case DARGS_SINGLE:
    {
        DisassembleSingle(dargs.code, dargs.dmode);
        break;
    }
    case DARGS_STDOUT:
    {
        FILE* file_in = fopen(dargs.fname_in, "rb"); //ignore warning, IsValidPath makes sure it isn't NULL
        if (file_in == NULL)
        {
            printf("ERROR: The file \"%s\" doesn't exist. Aborting.\n", dargs.fname_in);
            return 0; //terminate
        }
        printf("Starting disassembly of \"%s\".\n", dargs.fname_in);
        if (DisassembleFile(file_in, stdout, &dargs.frange, dargs.dmode))
        {
            printf("Successfully disassembled \"%s\".\n", dargs.fname_in);
        }
        else
        {
            printf("ERROR: DisassembleFile failed.\n");
        }
        fclose(file_in);
        break;
    }
    case DARGS_FILEOUT:
    {
        FILE* file_in = fopen(dargs.fname_in, "rb"); //ignore warning, IsValidPath makes sure it isn't NULL
        if (file_in == NULL)
        {
            printf("ERROR: The file \"%s\" doesn't exist. Aborting.\n", dargs.fname_in);
            return 0; //terminate
        }
        FILE* file_out = fopen(dargs.fname_out, "w+"); //ignore warning, IsValidPath makes sure it isn't NULL
        if (file_out == NULL)
        {
            printf("ERROR: The file \"%s\" could not be created. Aborting.\n", dargs.fname_out);
            return 0; //terminate
        }

        printf("Starting disassembly of \"%s\".\n", dargs.fname_in);
        if (DisassembleFile(file_in, file_out, &dargs.frange, dargs.dmode))
        {
            printf("Successfully disassembled \"%s\" to \"%s\".\n", dargs.fname_in, dargs.fname_out);
        }
        else
        {
            printf("ERROR: DisassembleFile failed.\n");
        }
        fclose(file_in);
        fclose(file_out);
        break;
    }
    }

#endif // DEBUG

    printf("Completion time: %.0f ms\n", (double)clock() - (double)start);
    return 0;
}
