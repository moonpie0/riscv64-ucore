#ifndef __KERN_SYNC_SYNC_H__
#define __KERN_SYNC_SYNC_H__

#include <defs.h>
#include <intr.h>
#include <riscv.h>

//保存中断状态
//通过读取 sstatus 寄存器的值来判断中断是否使能，并在需要时禁用中断
static inline bool __intr_save(void) {
    //通过read_csr读取sstatus寄存器的值
    //如果全局中断使能位置位
    if (read_csr(sstatus) & SSTATUS_SIE) {
        //禁用中断
        intr_disable();
        return 1;
    }
    return 0;
}
//根据参数来决定是否使能中断
static inline void __intr_restore(bool flag) {
    if (flag) {
        intr_enable();
    }
}
//保存和回复中断状态
#define local_intr_save(x) \
    do {                   \
        x = __intr_save(); \
    } while (0)
#define local_intr_restore(x) __intr_restore(x);

#endif /* !__KERN_SYNC_SYNC_H__ */
