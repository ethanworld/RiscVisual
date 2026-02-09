#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <string.h>
#include "common.h"

/**
 * 解析ELF Header
 */
int parse_ehdr(FileStru *file)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)file->buffer;
    FileElf *elf = &file->elf;
    elf->ehdr = *ehdr;
    elf->phdr_arr = (Elf64_Phdr *)(file->buffer + elf->ehdr.e_phoff);
    elf->phdr_num = ehdr->e_phnum;
    elf->shdr_arr = (Elf64_Shdr *)(file->buffer + elf->ehdr.e_shoff);
    elf->shdr_num = ehdr->e_shnum;
    printf("ELF HEADER:\n");
    printf("[e_type]%s, [e_class]%s, [e_phnum]%d, [e_shnum]:%d\n",
           E_TYPE_STR(ehdr->e_type),
           E_CLASS_STR(ehdr->e_ident[EI_CLASS]),
           ehdr->e_phnum, ehdr->e_shnum);

    PRINT_DIVIDER;
    return RET_OK;
}

int parse_phdr(FileStru *file)
{
    FileElf *elf = &file->elf;
    for (int idx = 0; idx < elf->phdr_num; idx++)
    {
        Elf64_Phdr *phdr = elf->phdr_arr + idx;
        printf("[idx]%d, [p_type]%d: %s, [p_flag]%s\n",
               idx,
               phdr->p_type,
               P_TYPE_STR(phdr->p_type),
               P_FLAGS_STR(phdr->p_flags));
        PRINT_DIVIDER;
    }
    return RET_OK;
}

/**
 * 解析SHT_STRTAB类型的section
 */
int parse_shdr_strtab(FileStru *file, Elf64_Shdr *shdr)
{
    LOG("start parse SHT_STRTAB");
    unsigned char *shstr_ctx = file->buffer + shdr->sh_offset;
    unsigned char *shstr_p = shstr_ctx;
    int cnt = 0;
    while (cnt < shdr->sh_size)
    {
        printf("%s\n", shstr_p);
        cnt += (strlen(shstr_p) + 1);
        shstr_p = shstr_ctx + cnt;
    }

    return RET_OK;
}

/**
 * .symtab + .strtab -- 是全量符号表，主要用于调试用，程序加载和运行不需要
 * .dynsym + .dynstr -- 动态链接需要，ld会读取
 * .dynsym和.symtab都是Elf64_Sym数据结构
 */
int parse_shdr_symtab(FileStru *file, Elf64_Shdr *shdr)
{
    LOG("start parse sym");

    Elf64_Shdr *shstr_shdr = file->elf.shdr_arr + file->elf.ehdr.e_shstrndx;
    unsigned char *shdr_name = file->buffer + shstr_shdr->sh_offset + shdr->sh_name;
    Elf64_Shdr *str_shdr = NULL;
    if (strcmp(shdr_name, DYNSYM_SECTION) == 0)
    {
        str_shdr = file->elf.dynstr_shdr;
    }
    else if (strcmp(shdr_name, SYMTAB_SECTION) == 0)
    {
        str_shdr = file->elf.strtab_shdr;
    }

    Elf64_Sym *sym_ctx = (Elf64_Sym *)(file->buffer + shdr->sh_offset);
    for (int idx = 0; idx < shdr->sh_size / shdr->sh_entsize; idx++)
    {
        Elf64_Sym *sym = sym_ctx + idx;
        unsigned char st_info = sym->st_info;
        unsigned char *sym_name = file->buffer + str_shdr->sh_offset + sym->st_name;
        printf("%d, [st_name]%s, [st_info]%s, [st_bind]%s\n",
               idx, sym_name,
               ST_INFO_STR(ELF64_ST_TYPE(st_info)),
               ST_BIND_STR(ELF64_ST_BIND(st_info)));
    }
    return 0;
}

/**
 * .dynamic -- 是链接器的 “导航表”——DT_NEEDED告诉链接器加载哪些 so，DT_STRTAB/DT_SYMTAB告诉链接器去哪里找符号；
 */
int parse_shdr_dynamic(FileStru *file, Elf64_Shdr *shdr)
{
    LOG("start parse dynamic");

    Elf64_Shdr *str_shdr = file->elf.dynstr_shdr;
    Elf64_Dyn *dyn_ctx = (Elf64_Dyn *)(file->buffer + shdr->sh_offset);
    for (int idx = 0; idx < shdr->sh_size / shdr->sh_entsize; idx++)
    {
        Elf64_Dyn *dyn = dyn_ctx + idx;
        // unsigned char st_info = dyn->d_tag;

        unsigned char *sym_name = NULL_STR;
        switch (dyn->d_tag)
        {
        case DT_NEEDED:
            sym_name = file->buffer + file->elf.dynstr_shdr->sh_offset + dyn->d_un.d_val;
            break;
        default:
            break;
        }
        printf("%d, [d_tag]%li:%s, %s\n",
               idx, dyn->d_tag, D_TAG_STR(dyn->d_tag), sym_name);
    }
}

/**
 * .interp section 是ELF文件中专门存储动态链接器路径的节，内容非常简单：就是一个以\0结尾的字符串
 * 例如：/lib64/ld-linux-x86-64.so.2
 */
int parse_shdr_interp(FileStru *file, Elf64_Shdr *shdr)
{
    LOG("start parse interp");

    unsigned char *interp_ctx = file->buffer + shdr->sh_offset;
    printf("ld path is:%s\n", interp_ctx);
}

/**
 * 对应.rela.dyn和.rela.plt，是ELF中的重定位表
 * 核心作用是告诉LD：要修正哪个地址」「关联哪个符号」「用什么规则修正」「是否需要偏移」
 * .rela.dyn -- 数据重定位，例如全局变量，函数的静态地址
 * .rela.plt -- 函数重定位，动态函数的地址
 */
int parse_shdr_rela(FileStru *file, Elf64_Shdr *shdr)
{
    LOG("start parse rela");

    Elf64_Shdr *str_shdr = file->elf.dynstr_shdr;
    Elf64_Shdr *sym_shdr = file->elf.dynsym_shdr;
    Elf64_Rela *rela_ctx = (Elf64_Rela *)(file->buffer + shdr->sh_offset);
    Elf64_Sym *sym_ctx = (Elf64_Sym *)(file->buffer + sym_shdr->sh_offset);
    
    for (int idx = 0; idx < shdr->sh_size / shdr->sh_entsize; idx++)
    {
        Elf64_Rela *rela = rela_ctx + idx;
        Elf64_Sym *sym = sym_ctx + ELF64_R_SYM(rela->r_info);
        unsigned char *sym_name = file->buffer + str_shdr->sh_offset + sym->st_name;
        printf("%d, [r_offset]0x%x, [r_sym]%s\n", idx, rela->r_offset, sym_name);
    }
}

int parse_shdr_prog(FileStru *file, Elf64_Shdr *shdr)
{
}

/**
 * 一、代码程序相关的section
 * .init & .init_array -- 此节区包含可执行指令，是进程初始化代码的一部分。程序开始执行时，系统会在开始调用主程序入口（通常指 C 语言的 main 函数）前执行这些代码。
 * .text -- 此节区包含程序的可执行指令。
 * .fini & .fini_array -- 此节区包含可执行的指令，是进程终止代码的一部分。程序正常退出时，系统将执行这里的代码。
 *
 * 二、数据相关的section
 * .bss -- 未初始化的全局变量对应的节。此节区不占用 ELF 文件空间，但占用程序的内存映像中的空间。当程序开始执行时，系统将把这些数据初始化为 0。bss 其实是 block started by symbol 的简写。
 * .data -- 这些节区包含初始化了的数据，会在程序的内存映像中出现。
 * .rodata -- 这些节区包含只读数据，这些数据通常参与进程映像的不可写段。
 *
 * 三、符号表和字符串表
 * .dynsym
 * .symtab
 * .dynstr
 * .strtab
 * .shstrtab
 * 
 * 四、重定位，用于地址修正
 * .rela.dyn -- 通用动态重定位表，修正全局数据或者函数的地址
 * .rela.plt -- plt表的重定位表，修正PLT跳转地址，动态函数延迟绑定
 * 
 * 五、动态链接，运行时才会依赖这部分section
 * .dynamic -- 是链接器的 “导航表”——DT_NEEDED告诉链接器加载哪些 so，DT_STRTAB/DT_SYMTAB告诉链接器去哪里找符号；
 * .plt
 * .plt.got
 * .plt.sec
 * .got
 * .got.plt
 * .interp --记录ldd的绝对路径
 *
 * 六、辅助节，调试/注释/安全
 * .debug_* -- 调试信息段，GDB使用，包括源码行号等
 * .note -- 注释、元信息，程序的版本，编译信息等
 * .eh_frame -- 异常处理帧
 * .gnu_stack -- 栈属性，
 * .gnu_relro -- 只读重定位段
 * 
 */
int parse_shdr(FileStru *file)
{
    struct
    {
        int sh_type;
        char *sh_name;
        int (*sh_func)(FileStru *file, Elf64_Shdr *shdr);
    } sh_handler_table[] = {
        {SHT_SYMTAB, NULL, parse_shdr_symtab},
        {SHT_STRTAB, NULL, parse_shdr_strtab},
        {SHT_DYNSYM, NULL, parse_shdr_symtab},
        {SHT_DYNAMIC, NULL, parse_shdr_dynamic},
        {SHT_NULL, INTERP_SECTION, parse_shdr_interp},
        {SHT_RELA, NULL, parse_shdr_rela},
        {SHT_PROGBITS, PLTGOT_SECTION, parse_shdr_prog}};

    FileElf *elf = &file->elf;
    Elf64_Shdr *shstr_shdr = elf->shdr_arr + elf->ehdr.e_shstrndx;

    // scan to find str section
    for (int idx = 0; idx < elf->shdr_num; idx++)
    {
        Elf64_Shdr *shdr = elf->shdr_arr + idx;
        unsigned char *shdr_name = file->buffer + shstr_shdr->sh_offset + shdr->sh_name;
        if (strcmp(shdr_name, DYNSTR_SECTION) == 0)
        {
            elf->dynstr_shdr = shdr;
        }
        else if (strcmp(shdr_name, STRTAB_SECTION) == 0)
        {
            elf->strtab_shdr = shdr;
        }
        else if (strcmp(shdr_name, DYNSYM_SECTION) == 0)
        {
            elf->dynsym_shdr = shdr;
        }
    }

    for (int idx = 0; idx < elf->shdr_num; idx++)
    {
        Elf64_Shdr *shdr = elf->shdr_arr + idx;
        unsigned char *shdr_name = file->buffer + shstr_shdr->sh_offset + shdr->sh_name;
        printf("[idx]%d, [sh_name]%d:%s, [sh_type]%s, [sh_flags]%s\n",
               idx, shdr->sh_name, shdr_name,
               SHT_STRTAB_STR(shdr->sh_type),
               SH_FLAGS_STR(shdr->sh_flags));
        for (int jdx = 0; jdx < sizeof(sh_handler_table) / sizeof(sh_handler_table[0]); jdx++)
        {
            if (sh_handler_table[jdx].sh_func == NULL)
            {
                continue;
            }
            if (sh_handler_table[jdx].sh_name != NULL && shdr_name != NULL && strcmp(sh_handler_table[jdx].sh_name, shdr_name) == 0)
            {
                sh_handler_table[jdx].sh_func(file, shdr);
                break;
            }
            if (shdr->sh_type != SHT_NULL && shdr->sh_type == sh_handler_table[jdx].sh_type)
            {
                sh_handler_table[jdx].sh_func(file, shdr);
                break;
            }
        }
        PRINT_DIVIDER;
    }
    return RET_OK;
}

int parse_start(FileStru *file)
{
    parse_ehdr(file);
    parse_shdr(file);
    parse_phdr(file);
    return RET_OK;
}

int main(int argc, char *argv[])
{
    char *filepath = "/coding/RiscVisual/elf/a.out";
    // filepath = argv[1];
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL)
    {
        perror("file open failed");
        return -1;
    }

    int ret = fseek(fp, 0, SEEK_END);
    if (ret != 0)
    {
        perror("fseek failed");
        fclose(fp);
        return -1;
    }

    FileStru file = {0};
    file.size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    file.buffer = malloc(file.size);
    size_t read_count = fread(file.buffer, 1, file.size, fp);
    fclose(fp);

    printf("file size: %zu\n", read_count);

    parse_start(&file);
    return 0;
}