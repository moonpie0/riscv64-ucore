#ifndef __KERN_MM_VMM_H__
#define __KERN_MM_VMM_H__

#include <defs.h>
#include <list.h>
#include <memlayout.h>
#include <sync.h>

//pre define
struct mm_struct;

// the virtual continuous memory area(vma), [vm_start, vm_end), 
// addr belong to a vma means  vma.vm_start<= addr <vma.vm_end 
struct vma_struct {  //虚拟内存区域
    struct mm_struct *vm_mm; // the set of vma using the same PDT 
    uintptr_t vm_start;      // start addr of vma      
    uintptr_t vm_end;        // end addr of vma, not include the vm_end itself
    uint_t vm_flags;       // flags of vma
    list_entry_t list_link;  // linear list link which sorted by start addr of vma
};

#define le2vma(le, member)                  \
    to_struct((le), struct vma_struct, member) //将链表节点转换为vma结构体

#define VM_READ                 0x00000001
#define VM_WRITE                0x00000002
#define VM_EXEC                 0x00000004

// the control struct for a set of vma using the same PDT
struct mm_struct { //使用相同PDT的一组VMA
    list_entry_t mmap_list;        // 使用链表将这组VMA按照起始地址从小到大排序linear list link which sorted by start addr of vma
    struct vma_struct *mmap_cache; // 指向当前访问的VMA，用于加速查找current accessed vma, used for speed purpose
    pde_t *pgdir;                  // 这组VMA使用的PDT the PDT of these vma
    int map_count;                 // 这组VMA的数量 the count of these vma
    void *sm_priv;                   // 交换管理器的私有数据 the private data for swap manager
};

struct vma_struct *find_vma(struct mm_struct *mm, uintptr_t addr); //查找给定地址所在的VMA
struct vma_struct *vma_create(uintptr_t vm_start, uintptr_t vm_end, uint_t vm_flags); //用于创建一个新的VMA
void insert_vma_struct(struct mm_struct *mm, struct vma_struct *vma); //用于将一个VMA插入到给定的mm结构体中

struct mm_struct *mm_create(void);
void mm_destroy(struct mm_struct *mm);

void vmm_init(void);

int do_pgfault(struct mm_struct *mm, uint_t error_code, uintptr_t addr);

extern volatile unsigned int pgfault_num;
extern struct mm_struct *check_mm_struct;

#endif /* !__KERN_MM_VMM_H__ */

