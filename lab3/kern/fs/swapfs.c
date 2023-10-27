#include <swap.h>
#include <swapfs.h>
#include <mmu.h>
#include <fs.h>
#include <ide.h>
#include <pmm.h>
#include <assert.h>

void //初始化swap分区
swapfs_init(void) {
    static_assert((PGSIZE % SECTSIZE) == 0);  //检查页大小是否是扇区大小的整数倍
    if (!ide_device_valid(SWAP_DEV_NO)) { //检查swap设备是否有效
        panic("swap fs isn't available.\n"); //panic错误
    }
    max_swap_offset = ide_device_size(SWAP_DEV_NO) / (PGSIZE / SECTSIZE); //swap分区的最大偏移量，磁盘大小/页数
}

int //entry:swap分区的偏移量 
swapfs_read(swap_entry_t entry, struct Page *page) { //读取swap分区数据
    return ide_read_secs(SWAP_DEV_NO, swap_offset(entry) * PAGE_NSECT, page2kva(page), PAGE_NSECT);  //swap_offset:实际偏移量
}

int
swapfs_write(swap_entry_t entry, struct Page *page) {
    return ide_write_secs(SWAP_DEV_NO, swap_offset(entry) * PAGE_NSECT, page2kva(page), PAGE_NSECT);
}

