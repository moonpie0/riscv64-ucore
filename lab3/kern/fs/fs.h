#ifndef __KERN_FS_FS_H__
#define __KERN_FS_FS_H__

#include <mmu.h>

#define SECTSIZE            512 //扇区大小512字节
#define PAGE_NSECT          (PGSIZE / SECTSIZE) //一页需要的扇区数量

#define SWAP_DEV_NO         1 //swap磁盘编号 只有一块磁盘

#endif /* !__KERN_FS_FS_H__ */

