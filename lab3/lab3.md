### 练习

对实验报告的要求：
 - 基于markdown格式来完成，以文本方式为主
 - 填写各个基本练习中要求完成的报告内容
 - 完成实验后，请分析ucore_lab中提供的参考答案，并请在实验报告中说明你的实现与参考答案的区别
 - 列出你认为本实验中重要的知识点，以及与对应的OS原理中的知识点，并简要说明你对二者的含义，关系，差异等方面的理解（也可能出现实验中的知识点没有对应的原理知识点）
 - 列出你认为OS原理中很重要，但在实验中没有对应上的知识点
 
#### 练习0：填写已有实验
本实验依赖实验1/2。请把你做的实验1/2的代码填入本实验中代码中有“LAB1”,“LAB2”的注释相应部分。

#### 练习1：理解基于FIFO的页面替换算法（思考题）
描述FIFO页面置换算法下，一个页面从被换入到被换出的过程中，会经过代码里哪些函数/宏的处理（或者说，需要调用哪些函数/宏），并用简单的一两句话描述每个函数在过程中做了什么？（为了方便同学们完成练习，所以实际上我们的项目代码和实验指导的还是略有不同，例如我们将FIFO页面置换算法头文件的大部分代码放在了`kern/mm/swap_fifo.c`文件中，这点请同学们注意）
 - 至少正确指出10个不同的函数分别做了什么？如果少于10个将酌情给分。我们认为只要函数原型不同，就算两个不同的函数。要求指出对执行过程有实际影响,删去后会导致输出结果不同的函数（例如assert）而不是cprintf这样的函数。如果你选择的函数不能完整地体现”从换入到换出“的过程，比如10个函数都是页面换入的时候调用的，或者解释功能的时候只解释了这10个函数在页面换入时的功能，那么也会扣除一定的分数


- int do_pgfault(struct mm_struct *mm, uint_t error_code, uintptr_t addr) 
    - 页面换入时，产生缺页异常会调用do_pgfault函数
    - ​ swap_init_ok是一个标记位，代表交换初始化成功，可以开始替换的过程了。首先声明了一个页，之后将结构mm、虚拟地址和这个空页，调用了swap_in函数。该函数首先为传入的空页page分配初始化，之后获取了mm一级页表对应的二级页表，通过swapfs_read尝试将硬盘中的内容换入到新的page中，最后，建立起该页的虚拟地址和物理地址之间的对应关系，然后设置为可交换，该页的虚拟地址设置为传入的地址。至此，do_pgfault结束，建立起了新的映射关系，下次访问不会有异常。
    - struct vma_struct *vma = find_vma(mm, addr); 通过虚拟地址找到对应的vma
        - pte_t *get_pte(pde_t *pgdir, uintptr_t la, bool create) 
        - 获取页表项,create=1说明如果页表项不存在则需要新创建这个页表项
        - pgdir:PDT（页目录表）的内核虚拟基地址 la:需要映射的线性地址
    - ptep = get_pte(mm->pgdir, addr, 1);在页表中查找或创建页表项
    - pgdir_alloc_page(mm->pgdir, addr, perm) 
        - 调用alloc_page和page_insert函数来分配一个页大小的内存，并在线性地址（la）和PDT（页目录表）pgdir之间建立一个地址映射关系。
        - 调用alloc_page函数来分配一个页大小的内存，并返回该页面的物理地址（page）
        - 调用page_insert函数将线性地址（la）和物理地址（pa）建立映射关系，并将映射关系添加到PDT（pgdir）中
    - if ((ret = swap_in(mm, addr, &page)) != 0)
        - 判断换入是否成功
        - int
swap_in(struct mm_struct *mm, uintptr_t addr, struct Page **ptr_result)
        - 将一个页面从磁盘区域换入内存
        - 调用alloc_page函数分配一个新的页面，并将其地址存储在result指针中
        - 使用get_pte函数获取给定线性地址在页表中的页表项指针（ptep）
        - 调用swapfs_read函数从磁盘交换区中读取对应的页面数据，并将数据存储在result页面中
        - 将加载的页面地址存储在ptr_result指针指向的位置，并返回0表示加载成功
    - page_insert(mm->pgdir, page, addr, perm);
        - 在页表中建立一个物理地址和线性地址之间的映射关系
        - 调用page_ref_inc函数增加指向该物理页面的虚拟地址的引用计数
        - 如果该页表项已经存在映射关系，则判断原先映射的物理页面是否与当前要映射的物理页面相同。如果相同，则不需要进行任何操作；否则，需要先删除原先的映射关系
        - 调用tlb_invalidate函数刷新TLB
    - swap_map_swappable(mm, addr, page, 1); 标记这个页面将来是可以再换出的


- struct Page *alloc_pages(size_t n)
    - 分配n个物理页面
    - local_intr_save(intr_flag);保存中断状态，并禁止中断
    - 调用pmm的alloc_pages分配物理页面
    - 恢复中断状态

- _fifo_map_swappable函数
    - ​ 将最近被用到的页面添加到算法所维护的次序队列
    - 建立一个指针指向最近使用的页面，然后在列表中新建一个空指针，将最近用到的页面添加到次序队尾

- _fifo_swap_out_victim函数
    - 查询哪个页面需要被换出,并执行换出操作
    - 使用链表操作，删除掉最早进入的那个页，并按照注释将这个页面给到传入参数ptr_page

- pmm.c/alloc_pages():分配空闲页的函数，在该函数中如果没有空闲的页时调用swap_out();换出一个页面到磁盘。
- swap.c/swap_out():该函数完成具体的物理页面换出到磁盘的过程
- swap_out()->swap_fifo.c/swap_out_victim(mm, &page, in_tick):调用FIFO算法将最早换入的页面从链表中卸下并存入page指针里
- pmm.c/get_pte():取到要换出的page后，调用该函数根据其虚拟地址分配一个对应的页表项的地址
- kern/fs/swapfs.c/swapfs_write:将要换出的物理页面写入硬盘上并根据返回值判断是否写入失败
- sm->map_swappable(mm, v, page, 0):如果写入磁盘失败，将刚刚取出的物理页重新链入到原来位置
- free_page(page):如果写入磁盘成功，从内存中释放刚刚换出的页面
- pmm.c/tlb_invalidate:由于页表改变，调用此函数执行flush_tlb()刷新TLB

1. **int do_pgfault (struct mm_struct \*mm, uint_t error_code, uintptr_t addr)**

   - **调用** ： 页面换入时，产生缺页异常会在调用 `do_pgfault` 函数

      ```mermaid
          graph LR
          A[exception_handler]-->B[pgfault_handler]
          B-->C[do_pgfault]
      ```

   - **功能** ：`do_pgfault` 在发生缺页异常时，检测当前页表中是否有包含该地址的页表项，若无则分配，并根据页表项将磁盘中的数据加载到物理页内并建立映射关系。

2. **struct vma_struct \* find_vma(struct mm_struct \*mm, uintptr_t addr)**
   - **调用** ： 在 `do_pgfault`  中根据 addr 寻找包含该地址的 vma.

      ```mermaid
          graph LR
          A[do_pgfault] --> B[find_vma]
      ```

   - **功能** ：根据 `addr`， 依照 `vma->vm_start <= addr && vma->vm_end > addr` 的关系， 遍历查找满足该条件的虚拟存储区域。

3. **pte_t \*get_pte(pde_t \*pgdir, uintptr_t la, bool create)**
   - **调用** ： 在 `do_pgfault`  中根据 addr 寻找包含该地址的 pte

      ```mermaid
          graph LR
          A[do_pgfault] --> B[get_pte]
      ```

   - **功能** ： 在 do_pgfault 中确定地址合法后，基于 PDT 查找 pte, 若没有找到，在第三个参数为 1 时创建一个 pte.

4. **int swap_in(struct mm_struct \*mm, uintptr_t addr, struct Page \*\*ptr_result)**
   - **调用** ： 在 `do_pgfault`  中根据 addr 将磁盘中对应的页换入到内存中

      ```mermaid
          graph LR
          A[do_pgfault] --> B[swap_in]
      ```

   - **功能** ： 利用 `alloc_page` 从系统内存管理系统中获得一个页后， 将 “磁盘” 中的页换入到分配的这个页中。

5. **int page_insert(pde_t \*pgdir, struct Page \*page, uintptr_t la, uint32_t perm)**
   - **调用** ： 在 `do_pgfault`  中根据 addr 将 swap_in 获得的页放入对应的 pte 中

      ```mermaid
          graph LR
          A[do_pgfault] --> B[page_insert]
      ```

   - **功能** ： 根据 addr 将 页放入对应的 pte 中，更新 tlb

6. **int swap_map_swappable(struct mm_struct \*mm, uintptr_t addr, struct Page \*page, int swap_in)**
   - **调用** ： 在 `do_pgfault`  调用记录页的使用/换入

      ```mermaid
          graph LR
          A[do_pgfault] --> B[swap_map_swappable]
      ```

   - **功能** ： 记录页的使用/换入
   - `_fifo_map_swappable` 函数
     - 将最近被用到的页面添加到算法所维护的次序队列
     - 建立一个指针指向最近使用的页面，然后在列表中新建一个空指针，将最近用到的页面添加到次序队尾

   - `_fifo_swap_out_victim` 函数
     - 查询哪个页面需要被换出,并执行换出操作
     - 使用链表操作，删除掉最早进入的那个页，并按照注释将这个页面给到传入参数 ptr_page
发送给 OS资料暂存群​​​










#### 练习2：深入理解不同分页模式的工作原理（思考题）
get_pte()函数（位于`kern/mm/pmm.c`）用于在页表中查找或创建页表项，从而实现对指定线性地址对应的物理页的访问和映射操作。这在操作系统中的分页机制下，是实现虚拟内存与物理内存之间映射关系非常重要的内容。
 - get_pte()函数中有两段形式类似的代码， 结合sv32，sv39，sv48的异同，解释这两段代码为什么如此相像。
 - 目前get_pte()函数将页表项的查找和页表项的分配合并在一个函数里，你认为这种写法好吗？有没有必要把两个功能拆开？

两段相似代码
- 第一段代码的作用是创建一级页表项（pdep1），并将其设置为有效和用户可访问。如果一级页表项已经存在映射关系，则直接返回该页表项的指针。如果一级页表项不存在映射关系，则分配一个物理页面，并将该页面的物理地址清零，然后创建一个新的页表项，并将其写入页表中。
- 第二段代码的作用是创建二级页表项（pdep0），并将其设置为有效和用户可访问。如果二级页表项已经存在映射关系，则直接返回该页表项的指针。如果二级页表项不存在映射关系，则分配一个物理页面，并将该页面的物理地址清零，然后创建一个新的页表项，并将其写入页表中。
- 这两段代码之所以相似，是因为它们都是用于创建页表项的，并且都包含了相同的逻辑：检查页表项是否已经存在映射关系，如果不存在则分配物理页面并创建新的页表项。
- sv32使用32位虚拟地址，sv39使用39位虚拟地址，sv48使用48位虚拟地址。这些不同的页表格式在页表项的结构和位域设置上有所不同，但是页表项的创建逻辑是相似的。因此，无论使用哪种页表格式，创建页表项的代码都会有相似的结构和逻辑。

- 关于页表项查找和页表项分配功能的区分是用传入的create来进行区分的，首先通过`pde_t *pdep1 = &pgdir[PDX1(la)];`查找该地址对应的一级页表项序号，如果没有查找到或者查找到的页面无效，则根据create的取值判断是否需要为其分配物理页面，接下来对于下一级页表项的处理的思路也类似，从而实现了将页表项的查找和页表项的分配合并在一个函数里。个人感觉没有必要把两个功能拆开，因为一般来说在页表项的分配前本来就需要检查是否已经存在对应关系，相当于页表项的查找是页表项分配工作的一部分，因此放在一个函数里进行即可。

#### 练习3：给未被映射的地址映射上物理页（需要编程）
补充完成do_pgfault（mm/vmm.c）函数，给未被映射的地址映射上物理页。设置访问权限 的时候需要参考页面所在 VMA 的权限，同时需要注意映射物理页时需要操作内存控制 结构所指定的页表，而不是内核的页表。
请在实验报告中简要说明你的设计实现过程。请回答如下问题：
 - 请描述页目录项（Page Directory Entry）和页表项（Page Table Entry）中组成部分对ucore实现页替换算法的潜在用处。
 - 如果ucore的缺页服务例程在执行过程中访问内存，出现了页访问异常，请问硬件要做哪些事情？
- 数据结构Page的全局变量（其实是一个数组）的每一项与页表中的页目录项和页表项有无对应关系？如果有，其对应关系是啥？

1. 页目录表和mm_struct结构对应，用于根据传入的线性地址索引对应的页表；页表项，即一个PTE用来描述一般意义上的物理页时，应该有PTE_P标记，即表示物理页存在；但当它用来描述一个被置换出去的物理页时，它被用来维护该物理页与swap磁盘上扇区的映射关系，此时没有PTE_P标记。页替换涉及到换入换出，换入时需要将某个虚拟地址对应于磁盘的一页内容读入到内存中，换出时需要将某个虚拟页的内容写到磁盘中的某个位置，因此页表项可以记录该虚拟页在磁盘中的位置，也为换入换出提供磁盘位置信息

分页机制的实现确保了虚拟地址和物理地址之间的对应关系。一方面，通过查找虚拟地址是否存在于一二级页表中可知发现该地址是否是合法的；同时可以通过修改映射关系实现页替换操作。另一方面，在实现页替换时涉及到换入换出：换入时需要将某个虚拟地址对应的磁盘的一页内容读入到内存中，换出时需要将某个虚拟页的内容写到磁盘中的某个位置。而页表项可以记录该虚拟页在磁盘中的位置，为换入换出提供磁盘位置信息，页目录项则是用来索引对应的页表。同时，我们可知PDE和PTE均保留了一些位给操作系统使用，具体可以应用在页替换算法时。present位为0时CPU不使用PTE上内容，这时候这些位便会闲置，可以将闲置位用于保存别的信息，例如页替换算法被换出的物理页在交换分区的位置等。同时，需要注意到dirty位，操作系统根据脏位可以判断是否对页数据进行write through。

页目录项：

    P (Present) 位：表示该页保存在物理内存中。
    R (Read/Write) 位：表示该页可读可写。
    U (User) 位：表示该页可以被任何权限用户访问。
    W (Write Through) 位：表示 CPU 可以直写回内存。
    D (Cache Disable) 位：表示不需要被 CPU 缓存。
    A (Access) 位：表示该页被写过。
    S (Size) 位：表示一个页 4MB 。
    9-11 位保留给 OS 使用。
    12-31 位指明 PTE 基质地址。

页表项：

    0-3 位同 PDE。
    C (Cache Disable) 位：同 PDE D 位。
    A (Access) 位：同 PDE 。
    D (Dirty) 位：表示该页被写过。
    G (Global) 位：表示在 CR3 寄存器更新时无需刷新 TLB 中关于该页的地址。
    9-11 位保留给 OS 使用。
    12-31 位指明物理页基址。

```C
struct Page {
    int ref;                        // page frame's reference counter
    uint_t flags;                 // array of flags that describe the status of the page frame
    uint_t visited;
    unsigned int property;          // the num of free block, used in first fit pm manager
    list_entry_t page_link;         // free list link
    list_entry_t pra_page_link;     // used for pra (page replace algorithm)
    uintptr_t pra_vaddr;            // used for pra (page replace algorithm)
};
```


2. CPU会把产生异常的线性地址存储在CR2寄存器中，并且把表示页访问异常类型的error Code保存在中断栈中，以便恢复现场。然后就是和普通的中断一样，保护现场，将寄存器的值压入栈中，设置错误代码 error_code，触发 Page Fault 异常，然后压入 error_code 中断服务例程，将外存的数据换到内存中来，最后退出中断，回到进入中断前的状态。

3. 数据结构page是最低级的页表，目录项是一级页表，存储的内容是页表项的起始地址（二级页表），而页表项是二级页表，存储的是每个页表的开始地址，这些内容之间的关系时通过线性地址高低位不同功能的寻址体现的




#### 练习4：补充完成Clock页替换算法（需要编程）
通过之前的练习，相信大家对FIFO的页面替换算法有了更深入的了解，现在请在我们给出的框架上，填写代码，实现 Clock页替换算法（mm/swap_clock.c）。
请在实验报告中简要说明你的设计实现过程。请回答如下问题：
 - 比较Clock页替换算法和FIFO算法的不同。

 具体实现
 - 初始化
     ```c
    static int
    _clock_init_mm(struct mm_struct *mm)
    {     
     // 初始化pra_list_head为空链表
     list_init(&pra_list_head);
     // 初始化当前指针curr_ptr指向pra_list_head，表示当前页面替换位置为链表头
     curr_ptr=&pra_list_head;
     // 将mm的私有成员指针指向pra_list_head，用于后续的页面替换算法操作
     mm->sm_priv = &pra_list_head;
     return 0;
    }
    ```
- 将页面加入链表
    ```c
    static int
    _clock_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
    {
    list_entry_t *entry=&(page->pra_page_link);
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    assert(entry != NULL && curr_ptr != NULL);
    //换入页的在链表中的位置并不影响，因此将其插入到链表最末端。
    // 将页面page插入到页面链表pra_list_head的末尾
    list_add(head->prev , entry);
    // 将页面的visited标志置为1，表示该页面已被访问
    page->visited=1;
    return 0;
    }
    ```
- 页面换出
    ```c
    static int
    _clock_swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick)
    {
     list_entry_t *head=(list_entry_t*) mm->sm_priv;
         assert(head != NULL);
     assert(in_tick==0);
    while (1) {
        // 遍历页面链表pra_list_head，查找最早未被访问的页面
        // 获取当前页面对应的Page结构指针
        // 如果当前页面未被访问，则将该页面从页面链表中删除，并将该页面指针赋值给ptr_page作为换出页面
        // 如果当前页面已被访问，则将visited标志置为0，表示该页面已被重新访问
            if(curr_ptr==head){
            curr_ptr=list_next(curr_ptr);
            }
            print_cur();
            struct Page *ptr = le2page(curr_ptr, pra_page_link);
            if(ptr->visited==0){
                *ptr_page = ptr; //标记为换出页
                list_entry_t *p = curr_ptr;
                curr_ptr=list_next(curr_ptr);
                print_cur();
                list_del(p);         
                break;
            }
            ptr->visited=0;
            curr_ptr=list_next(curr_ptr);
    }
    return 0;
    }
    ```

 Clock页替换算法将内存中的页连接成一个环形链表，并给每一个页表项新增了一个访问位用来标记该页当前是否被访问过，被访问时该位置1。当需要将换出页面时，遍历环形链表，对当前指针指向的页所对应的页表项进行查询，如果访问位为0，则淘汰该页，如果访问位为1，则将访问位置0，并访问链表中的下一页。
Clock页替换算法和FIFO算法都是从最早进入的页面开始访问，不同的是FIFO算法直接选择了最早的页面进行替换，而Clock算法跳过了不久之前被访问过的页，是LRU算法的一种近似体现。
设计实现过程：
- 全局变量：
  - pra_list_head：存放页面环形链表的表头
  - *curr_ptr：当前指针
- _clock_init_mm：初始化环形链表、当前指针和结构体mm的私有成员指针
- _clock_map_swappable：将页面插入到环形链表末尾并将访问位置1表示该页面最近被访问过
- _clock_swap_out_victim：寻找换出的目标页面，遍历环形链表，查找最早未被访问的页面，如果当前指针指向的页面访问位为0，则将其从链表中删除并将该页面指针赋给ptr_page作为换出页面，如果访问位为1，则重新置0表示该页面已被重新访问


#### 练习5：阅读代码和实现手册，理解页表映射方式相关知识（思考题）
如果我们采用”一个大页“ 的页表映射方式，相比分级页表，有什么好处、优势，有什么坏处、风险？

- 好处
    - 实现简单，仅需要维护一个页表
    - 访问速度快，直接访问页表即可，中间无需其他查找操作，所以速度较快
    - 内存占用少，仅需要一个页表即可支持页表映射，所需内存少
- 坏处
    - 页表大小受限制，大虚拟内存空间不适用。由于每个虚拟内存页对应一个页表项，当虚拟内存非常大，页表需要的物理内存随之增加，系统会进行限制
    - 访问效率（另一个角度会较低）。页表是以线性结构存储页表项，虚拟空间很大时，页表项数量随之增大，查找速度慢，查找对应的物理页框也就慢了。
    - TLB缓存频繁缺失。TLB缓存有限，虚拟空间很大，导致一级页表较大，导致TLB无法缓存最近使用的所有页表项，导致命中概率降低，查询转换速率降低。


#### 扩展练习 Challenge：实现不考虑实现开销和效率的LRU页替换算法（需要编程）
challenge部分不是必做部分，不过在正确最后会酌情加分。需写出有详细的设计、分析和测试的实验报告。完成出色的可获得适当加分。

    LRU算法的主要思路为，在添加页面时将页面插入到链表头。在访问页面时，如果该页面已经处在链表中，则需要调整链表中页面的顺序：删掉链表中的原页面，将页面重新添加到链表头部。换出页面时，直接将链表尾部页面换出。

与FIFO又区别的部分的代码为swappable的代码，如下所示。

```c
static int
_lru_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{//将一个页面映射到可换出列表中，将该页面插入到链表的尾部
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    list_entry_t *entry=&(page->pra_page_link);
    assert(entry != NULL && head != NULL);
    curr_ptr=list_next(head);
    struct Page *ptr;
    while(curr_ptr!=head->prev) //如果链表中已有该页面
    {
        ptr = le2page(curr_ptr, pra_page_link);
        cprintf("curr_ptr %p\n", ptr);
        if(ptr==page)
        {
            list_del(curr_ptr);
            break;
            cprintf("链表顺序已改变");
        }
        curr_ptr=list_next(curr_ptr);
    }
    list_add(head, entry);
    return 0;
}
```

只有访问页面时，调用`_lru_map_swappable`函数才能整理链表，因此需要知道访问页面的时机。可以考虑`do_pgfault`函数中，页面的perm权限都改为可读不可写，这样就能每次访问的时候都触发异常，进入到`do_pgfault`中调用`swap_map_swappable`函数，就能整理链表的顺序了。


