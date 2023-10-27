#include <assert.h>
#include <defs.h>
#include <fs.h>
#include <ide.h>
#include <stdio.h>
#include <string.h>
#include <trap.h>
#include <riscv.h>

void ide_init(void) {} //对swap硬盘的初始化

#define MAX_IDE 2 //最大的IDE设备为2
#define MAX_DISK_NSECS 56 //每个IDE设备最大扇区数为2
static char ide[MAX_DISK_NSECS * SECTSIZE]; //模拟IDE硬盘存储空间：最大扇区数*每个扇区大小
//SECTSIZE：扇区大小512字节

bool ide_device_valid(unsigned short ideno) { return ideno < MAX_IDE; } //检查IDE编号是否有效

size_t ide_device_size(unsigned short ideno) { return MAX_DISK_NSECS; } //获取硬盘大小

//读取扇区数据
int ide_read_secs(unsigned short ideno, uint32_t secno, void *dst, //dst目标地址
                  size_t nsecs) { //读取的扇区数量
    int iobase = secno * SECTSIZE; //起始扇区号*扇区大小
    memcpy(dst, &ide[iobase], nsecs * SECTSIZE); //将硬盘数据复制到目标地址
    return 0;
}

int ide_write_secs(unsigned short ideno, uint32_t secno, const void *src,
                   size_t nsecs) {
    int iobase = secno * SECTSIZE;
    memcpy(&ide[iobase], src, nsecs * SECTSIZE);
    return 0;
}
