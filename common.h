#include <stdlib.h>
#include <string.h>

#define RET_OK 1
#define RET_ERR 0
#define PRINT_DIVIDER printf("========================================\n")
#define LOG(msg) printf("[%s] %s\n", __FUNCTION__, msg)
    
static char NULL_STR[] = "???";
static char DYNSTR_SECTION[] = ".dynstr";
static char STRTAB_SECTION[] = ".strtab";
static char DYNSYM_SECTION[] = ".dynsym";
static char SYMTAB_SECTION[] = ".symtab";
static char INTERP_SECTION[] = ".interp";
static char PLT_SECTION[] = ".plt";
static char PLTGOT_SECTION[] = ".plt.got";
static char GOT_SECTION[] = ".got";

typedef struct
{
    Elf64_Ehdr ehdr;
    Elf64_Phdr *phdr_arr;
    Elf64_Half phdr_num;
    Elf64_Shdr *shdr_arr;
    Elf64_Half shdr_num;

    Elf64_Shdr *dynstr_shdr;
    Elf64_Shdr *dynsym_shdr;
    Elf64_Shdr *strtab_shdr;
} FileElf;

typedef struct
{
    unsigned char *buffer;
    size_t size;
    FileElf elf;
} FileStru;

char *SHT_STRTAB_STR(int type)
{
    static char *SHT_STRTAB_STR_TABLE[] = {
        "SHT_NULL",
        "SHT_PROGBITS",
        "SHT_SYMTAB",
        "SHT_STRTAB",
        "SHT_RELA",
        "SHT_HASH",
        "SHT_DYNAMIC",
        "SHT_NOTE",
        "SHT_NOBITS",
        "SHT_REL",
        "SHT_SHLIB",
        "SHT_DYNSYM",

        "SHT_LOPROC", // 0X70000000
        "SHT_HIPROC", // OX7FFFFFFF
        "SHT_LOUSER", // 0X80000000
        "SHT_HIUSER", // 0X8FFFFFFF
    };

    if (type >= sizeof(SHT_STRTAB_STR_TABLE) / sizeof(SHT_STRTAB_STR_TABLE[0]))
    {
        return NULL_STR;
    }
    return SHT_STRTAB_STR_TABLE[type];
}

char *E_TYPE_STR(int type)
{
    static char *E_TYPE_STR_TABLE[] = {
        "ET_NONE",
        "ET_REL",
        "ET_EXEC",
        "ET_DYN",
        "ET_CORE",

        "ET_LOPROC", // 0xff00
        "ET_HIPROC", // 0xffff
    };

    static char default_name[] = "???";
    if (type >= sizeof(E_TYPE_STR_TABLE) / sizeof(E_TYPE_STR_TABLE[0]))
    {
        return default_name;
    }
    return E_TYPE_STR_TABLE[type];
}

char *E_CLASS_STR(int type)
{
    static char *E_CLASS_STR_TABLE[] = {
        "ELFCLASSNONE",
        "ELFCLASS32",
        "ELFCLASS64",
    };

    if (type >= sizeof(E_CLASS_STR_TABLE) / sizeof(E_CLASS_STR_TABLE[0]))
    {
        return NULL_STR;
    }
    return E_CLASS_STR_TABLE[type];
}

char *ST_INFO_STR(int type)
{
    static char *ST_INFO_STR_TABLE[] = {
        "STT_NOTYPE",
        "STT_OBJECT",
        "STT_FUNC",
        "STT_SECTION",
        "STT_FILE",
    };

    if (type >= sizeof(ST_INFO_STR_TABLE) / sizeof(ST_INFO_STR_TABLE[0]))
    {
        return NULL_STR;
    }
    return ST_INFO_STR_TABLE[type];
}

char *ST_BIND_STR(int type)
{
    static char *ST_BIND_STR_TABLE[] = {
        "STB_LOCAL",
        "STB_GLOBAL",
        "STB_WEAK",
    };

    if (type >= sizeof(ST_BIND_STR_TABLE) / sizeof(ST_BIND_STR_TABLE[0]))
    {
        return NULL_STR;
    }
    return ST_BIND_STR_TABLE[type];
}

char *SH_FLAGS_STR(int type)
{
    struct {
        int bit;
        char *name;
    } sh_flags_table[] = {
        {0x1, "SHF_WRITE"},
        {0x2, "SHF_ALLOC"},
        {0x4, "SHF_EXECINSTR"},
        {0xf0000000, "SHF_MASKPROC"},
    };

    // todo: memory leak
    char *tmp = calloc(100, sizeof(char));
    char *p = tmp;
    for (int idx = 0; idx < sizeof(sh_flags_table) / sizeof(sh_flags_table[0]); idx++)
    {
        if (type & sh_flags_table[idx].bit) {
            if (strlen(p) == 0) {
                sprintf(p, "%s", sh_flags_table[idx].name);
            } else {
                sprintf(p, "%s|%s", p, sh_flags_table[idx].name);
            }
        }
    }

    return strlen(tmp) > 0 ? tmp : NULL_STR;
}

char *P_TYPE_STR(int type)
{
    static char *P_TYPE_STR_TABLE[] = {
        "PT_NULL",
        "PT_LOAD",
        "PT_DYNAMIC",
        "PT_INTERP",
        "PT_NOTE",
        "PT_SHLIB",
        "PT_PHDR",

        "PT_LOPROC", // 0x70000000 
        "PT_HIPROC", // 0x7fffffff
    };

    if (type >= sizeof(P_TYPE_STR_TABLE) / sizeof(P_TYPE_STR_TABLE[0]))
    {
        return NULL_STR;
    }
    return P_TYPE_STR_TABLE[type];
}

char *P_FLAGS_STR(int type)
{
    struct {
        int bit;
        char *name;
    } p_flags_table[] = {
        {0x1, "PF_X"},
        {0x2, "PF_W"},
        {0x4, "PF_R"},
        {0xf0000000, "PF_MASKPROC"},
    };

    // todo: memory leak
    char *tmp = calloc(100, sizeof(char));
    char *p = tmp;
    for (int idx = 0; idx < sizeof(p_flags_table) / sizeof(p_flags_table[0]); idx++)
    {
        if (type & p_flags_table[idx].bit) {
            if (strlen(p) == 0) {
                sprintf(p, "%s", p_flags_table[idx].name);
            } else {
                sprintf(p, "%s|%s", p, p_flags_table[idx].name);
            }
        }
    }

    return strlen(tmp) > 0 ? tmp : NULL_STR;
}



char *D_TAG_STR(int type)
{
    static char *D_TAG_STR_TABLE[] = {
        "NULL",
        "DT_NEEDED",
        "DT_PLTRELSZ",
        "DT_PLTGOT",
        "DT_HASH",
        "DT_STRTAB",
        "DT_SYMTAB",
        "DT_RELA",
        "DT_RELASZ",
        "DT_RELAENT",
        "DT_STRSZ",
        "DT_SYMENT",
        "DT_INIT",
        "DT_FINI",
        "DT_SONAME",
        "DT_RPATH",
        "DT_SYMBOLIC",
        "DT_REL",
        "DT_RELSZ",
        "DT_RELENT",
        "DT_PLTREL",
        "DT_DEBUG",
        "DT_TEXTREL",
        "DT_JMPREL",

        "DT_LOPROC", // 0x70000000 
        "DT_HIPROC", // 0x7fffffff
    };

    if (type >= sizeof(D_TAG_STR_TABLE) / sizeof(D_TAG_STR_TABLE[0]))
    {
        return NULL_STR;
    }
    return D_TAG_STR_TABLE[type];
}