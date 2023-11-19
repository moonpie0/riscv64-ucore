
### 练习1：分配并初始化一个进程控制块（需要编码）

alloc_proc函数（位于kern/process/proc.c中）负责分配并返回一个新的struct proc_struct结构，用于存储新建立的内核线程的管理信息。ucore需要对这个结构进行最基本的初始化，你需要完成这个初始化过程。

    【提示】在alloc_proc函数的实现中，需要初始化的proc_struct结构中的成员变量至少包括：state/pid/runs/kstack/need_resched/parent/mm/context/tf/cr3/flags/name。

请在实验报告中简要说明你的设计实现过程。请回答如下问题：

    请说明proc_struct中struct context context和struct trapframe *tf成员变量含义和在本实验中的作用是啥？（提示通过看代码和编程调试可以判断出来）


各个变量的详细解释如下：

- state：进程状态，proc.h中定义了四种状态：创建（UNINIT）、睡眠（SLEEPING）、就绪（RUNNABLE）、退出（ZOMBIE，等待父进程回收其资源）
- pid：进程ID，调用本函数时尚未指定，默认值设为-1
- runs：线程运行总数，默认值0
- need_resched：标志位，表示该进程是否需要重新参与调度以释放CPU，初值0（false，表示不需要）
- parent：父进程控制块指针，初值NULL
- mm：用户进程虚拟内存管理单元指针，由于系统进程没有虚存，其值为NULL
- context：进程上下文，默认值全零
- tf：中断帧指针，默认值NULL
- cr3：该进程页目录表的基址寄存器，初值为ucore启动时建立好的内核虚拟空间的页目录表首地址boot_cr3（在kern/mm/pmm.c的pmm_init函数中初始化）
- flags：进程标志位，默认值0
- name：进程名数组



```c

static struct proc_struct *
alloc_proc(void) {
    //分配一个proc_struct结构体的内存
    struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
    if (proc != NULL) {
    //LAB4:EXERCISE1 李颖 2110939
      //进程状态，未初始化
        proc->state = PROC_UNINIT;
        //进程ID 
        proc->pid = -1;
        //线程运行总数，默认0
        proc->runs = 0;
        //内核栈
        proc->kstack = 0;
        //是否需要重新调度以释放CPU，初值0（false，表示不需要）
        proc->need_resched = 0; 
        //父进程控制块指针
        proc->parent = NULL;
        //用户进程虚拟内存管理单元指针，由于系统进程没有虚存，其值为NULL
        proc->mm = NULL;
        //上下文
        memset(&(proc->context), 0, sizeof(struct context));
        //当前中断的trapframe
        proc->tf = NULL;
        //页目录表基地址，初始化为初值为ucore启动时建立好的内核虚拟空间的页目录表首地址boot_cr3
        proc->cr3 = boot_cr3;
        //进程标志
        proc->flags = 0;
        //进程名数组
        memset(proc->name, 0, PROC_NAME_LEN);
    }
    return proc;
    //返回指向初始化后的proc_struct结构体的指针proc
}
```

 `struct context context`
- 这是进程上下文结构体，用于保存进程在执行时的寄存器状态。在进程切换时，需要保存当前进程的寄存器状态，并将要切换到的进程的寄存器状态加载到CPU寄存器中，以实现进程的切换。
- 包含：`ra/sp/s0~s11`
- `context`指进程上下文，这部分空间用于保存创建进程时父进程的部分寄存器值：`eip, esp, ebx, ecx, edx, esi, edi, ebp`；其他寄存器在切换进程时值不变，故不需要保存。

- `struct trapframe *tf`
    - 这是一个指向 `Trap frame` 结构体的指针，用于保存当前中断发生时的 CPU 寄存器状态。在发生中断时，CPU 会将当前的寄存器状态保存到 `Trap frame` 中，并将中断处理程序的入口地址加载到程序计数器中，以便中断处理程序能够正确地执行。因此，tf 指针指向了当前中断的 `Trap frame `结构体，其中包含了当前中断的所有寄存器状态。
    - ```c
        struct trapframe {
        //嵌套的结构体，用于保存所有通用寄存器的值
        struct pushregs gpr; 
        //保存CPU状态寄存器的值
        uintptr_t status;
        //发生中断时程序计数器PC的值
        uintptr_t epc;
        //保存发生地址相关异常时的虚拟地址值
        uintptr_t badvaddr;
        //发生中断或异常的原因码
        uintptr_t cause;
        };```
    - tf是中断帧的指针，总是指向内核栈的某个位置：当进程从用户空间跳到内核空间时，中断帧记录了进程在被中断前的状态。当内核需要跳回用户空间时，需要调整中断帧以恢复让进程继续执行的各寄存器值。

### 练习2：为新创建的内核线程分配资源（需要编码）==


创建一个内核线程需要分配和设置好很多资源。kernel_thread函数通过调用do_fork函数完成具体内核线程的创建工作。do_kernel函数会调用alloc_proc函数来分配并初始化一个进程控制块，但alloc_proc只是找到了一小块内存用以记录进程的必要信息，并没有实际分配这些资源。ucore一般通过do_fork实际创建新的内核线程。do_fork的作用是，创建当前内核线程的一个副本，它们的执行上下文、代码、数据都一样，但是存储位置不同。因此，我们实际需要”fork”的东西就是stack和trapframe。在这个过程中，需要给新内核线程分配资源，并且复制原进程的状态。你需要完成在kern/process/proc.c中的do_fork函数中的处理过程。它的大致执行步骤包括：

    调用alloc_proc，首先获得一块用户信息块。

    为进程分配一个内核栈。

    复制原进程的内存管理信息到新进程（但内核线程不必做此事）

    复制原进程上下文到新进程

    将新进程添加到进程列表

    唤醒新进程

    返回新进程号

```c
static void
copy_thread(struct proc_struct *proc, uintptr_t esp, struct trapframe *tf) {
    proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE - sizeof(struct trapframe));
    *(proc->tf) = *tf;

    // Set a0 to 0 so a child process knows it's just forked
    proc->tf->gpr.a0 = 0;
    proc->tf->gpr.sp = (esp == 0) ? (uintptr_t)proc->tf : esp;

    proc->context.ra = (uintptr_t)forkret;
    proc->context.sp = (uintptr_t)(proc->tf);
}

/* do_fork -     parent process for a new child process
 * @clone_flags: used to guide how to clone the child process
 * @stack:       the parent's user stack pointer. if stack==0, It means to fork a kernel thread.
 * @tf:          the trapframe info, which will be copied to child process's proc->tf
 */
int
do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf) {
    int ret = -E_NO_FREE_PROC;
    struct proc_struct *proc;
    if (nr_process >= MAX_PROCESS) {
        goto fork_out;
    }
    ret = -E_NO_MEM;
    //LAB4:EXERCISE2 李颖 2110939
    //尝试分配内存，失败就退出
    if((proc = alloc_proc()) == NULL)goto fork_out;
    //把此进程的父进程设置为current
    proc->parent = current;
    //尝试分配内核栈
    if(setup_kstack(proc)==-E_NO_MEM)goto bad_fork_cleanup_proc;
    //尝试复制父进程内存
    if(copy_mm(clone_flags,proc)!= 0)goto bad_fork_cleanup_kstack;
    //复制中断帧和上下文
    copy_thread(proc,stack,tf);
    bool flag;
    //屏蔽中断
    local_intr_save(flag);
    proc->pid=get_pid();//获取PID
    hash_proc(proc);//建立hash映射
    list_add_after(&proc_list,&(proc->list_link));//添加进链表
    /*用list_add_before或者list_add也完全ok*/
    nr_process++;//进程数++
    local_intr_restore(flag);//恢复中断
    wakeup_proc(proc);//唤醒进程
    return proc->pid;//返回PID
    

fork_out:
    return ret;

bad_fork_cleanup_kstack:
    put_kstack(proc);
bad_fork_cleanup_proc:
    kfree(proc);
    goto fork_out;
}
```


请在实验报告中简要说明你的设计实现过程。请回答如下问题：

    请说明ucore是否做到给每个新fork的线程一个唯一的id？请说明你的分析和理由。

分配ID的代码是get_pid(),这段代码会不断的在链表中遍历，直到找到一个合适的last_pid才会返回，这个last_pid满足两个条件(1)不大于MAX_PID(2)未被分配过，因此ucore为每个新fork的线程分配了一个唯一的id。

### 练习3：编写proc_run 函数（需要编码）

proc_run用于将指定的进程切换到CPU上运行。它的大致执行步骤包括：

    检查要切换的进程是否与当前正在运行的进程相同，如果相同则不需要切换。

    禁用中断。你可以使用/kern/sync/sync.h中定义好的宏local_intr_save(x)和local_intr_restore(x)来实现关、开中断。

    切换当前进程为要运行的进程。

    切换页表，以便使用新进程的地址空间。/libs/riscv.h中提供了lcr3(unsigned int cr3)函数，可实现修改CR3寄存器值的功能。

    实现上下文切换。/kern/process中已经预先编写好了switch.S，其中定义了switch_to()函数。可实现两个进程的context切换。

    允许中断。



```c
    void
    proc_run(struct proc_struct *proc) {
    if (proc != current) {
        // LAB4:EXERCISE3 李颖 2110939
       //用于保存中断状态
       bool intr_flag;
       //指向当前进程和即将运行的进程
        struct proc_struct *prev = current, *next = proc;
        // 将当前中断状态保存到intr_flag中
        local_intr_save(intr_flag);
        {
            //将当前进程变更为proc
            current = proc;
            //加载新进程的内核栈指针
            load_esp0(next->kstack + KSTACKSIZE);
            //加载新进程的页目录基址寄存器
            lcr3(next->cr3);
            //进行进程上下文的切换，将prev进程的上下文切换到next进程的上下文
            switch_to(&(prev->context), &(next->context));
        }
        //恢复之间保存的中断状态
        local_intr_restore(intr_flag);
        }
    }
```


请回答如下问题：

    在本实验的执行过程中，创建且运行了几个内核线程？

有两个内核线程
- 创建第0个内核线程`idleproc`。在 `init.c::kern_init` 函数调用了 `proc.c::proc_init` 函数。 `proc_init `函数启动了创建内核线程的步骤。首先当前的执行上下文（从 kern_init 启动至今）就可以看成是 uCore 内核（也可看做是内核进程）中的一个内核线程的上下文。为此，uCore 通过给当前执行的上下文分配一个进程控制块以及对它进行相应初始化，将其打造成第 0 个内核线程 – `idleproc`。
- 创建第 1 个内核线程 `initproc`。第 0 个内核线程主要工作是完成内核中各个子系统的初始化，然后就通过执行 cpu_idle 函数开始过退休生活了。所以 uCore 接下来还需创建其他进程来完成各种工作，但 idleproc 内核子线程自己不想做，于是就通过调用 `kernel_thread` 函数创建了一个内核线程 `init_main`。在Lab4中，这个子内核线程的工作就是输出一些字符串，然后就返回了（参看 init_main 函数）。但在后续的实验中，init_main 的工作就是创建特定的其他内核线程或用户进程。

### 扩展练习 Challenge：

    说明语句local_intr_save(intr_flag);....local_intr_restore(intr_flag);是如何实现开关中断的？


这两个函数实现的意义就是避免在进程切换过程中处理中断。因为有些过程是互斥的，只允许一个线程进入，因此需要关闭中断来处理临界区；如果此时在切换过程中又一次中断的话，那么该进程保存的值就很可能出bug并且丢失难寻回了。

是保护进程切换不会被中断，以免进程切换时其他进程再进行调度，相当于互斥锁。之前在第六步添加进程到列表的时候也需要有这个操作，是因为进程进入列表的时候，可能会发生一系列的调度事件，比如我们所熟知的抢断等，加上这么一个保护机制可以确保进程执行不被打乱。

```c
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
#define local_intr_restore(x) __intr_restore(x);```





https://blog.csdn.net/JustinAustin/article/details/121884725

