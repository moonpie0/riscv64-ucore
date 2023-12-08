#ifndef __LIBS_ELF_H__
#define __LIBS_ELF_H__

#include <defs.h>

#define ELF_MAGIC   0x464C457FU         // "\x7FELF" in little endian

/* file header */
struct elfhdr {
    //模数，标识该文件是否为elf文件，必须等于ELF_MAGIC常量
    uint32_t e_magic;     // must equal ELF_MAGIC
    //保留字段，未使用
    uint8_t e_elf[12];
    //elf文件类型（可重定位 可执行 共享目标文件）
    uint16_t e_type;      // 1=relocatable, 2=executable, 3=shared object, 4=core image
    //目标机器类型，elf文件架构
    uint16_t e_machine;   // 3=x86, 4=68K, etc.
    //elf版本号
    uint32_t e_version;   // file version, always 1
    //可执行文件入口地址，程序开始第一条指令地址
    uint64_t e_entry;     // entry point if executable
    //elf文件中程序头表(program header table)的偏移量
    uint64_t e_phoff;     // file position of program header or 0
    //节头表(section header table)
    uint64_t e_shoff;     // file position of section header or 0
    uint32_t e_flags;     // architecture-specific flags, usually 0
    uint16_t e_ehsize;    // size of this elf header
    //每个程序头大小
    uint16_t e_phentsize; // size of an entry in program header
    //程序头表中程序头数量
    uint16_t e_phnum;     // number of entries in program header or 0
    uint16_t e_shentsize; // size of an entry in section header
    uint16_t e_shnum;     // number of entries in section header or 0
    //包含节名称的节头在节头表中的索引
    uint16_t e_shstrndx;  // section number that contains section name strings
};

/* program section header */
struct proghdr {
    //程序段类型，可能是可加载的代码或数据、动态链接信息等
    uint32_t p_type;   // loadable code or data, dynamic linking info,etc.
    //程序段属性，读写执行等权限位
    uint32_t p_flags;  // read/write/execute bits
    //程序段在可执行文件中的起始位置距离文件开始的偏移量
    uint64_t p_offset; // file offset of segment
    uint64_t p_va;     // virtual address to map segment
    uint64_t p_pa;     // physical address, not used
    uint64_t p_filesz; // size of segment in file
    //内存大小
    uint64_t p_memsz;  // size of segment in memory (bigger if contains bss）
    //对齐要求
    uint64_t p_align;  // required alignment, invariably hardware page size
};

/* values for Proghdr::p_type */
#define ELF_PT_LOAD                     1

/* flag bits for Proghdr::p_flags */
#define ELF_PF_X                        1
#define ELF_PF_W                        2
#define ELF_PF_R                        4

#endif /* !__LIBS_ELF_H__ */

