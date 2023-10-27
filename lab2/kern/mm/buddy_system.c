#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddy_system.h>
#include <stdio.h>

free_area_t free_area;

#define free_list (free_area.free_list)
#define nr_free (free_area.nr_free)

int IS_POWER_OF_2(int n)
{
    int i;
    for(i=1;i<n;i*=2);
    return n==i ? 1 : 0;
}

int MAX(int a, int b){ return a>b?a:b;}

int MIN_BLOCK(int n){ //找到比n大的最小的2^k块
    int i=1;
    for(i;i<n;i*=2);
    return i;
}

int LEFT_LEAF(int index){ return index*2+1;}
int RIGHT_LEAF(int index){ return index*2+2;}
int PARENT (int index){ return (index+1)/2-1;}


/*----------------------------------------------------*/

struct buddy2 {
  unsigned size;
  unsigned longest[1]; //此处数组大小为1表明这是可以向后扩展的
};

struct allocBlock{ //已分配的块
    struct Page* base; //分配的页首
    int offset; //二叉树的偏移量
    int pageNum; //大小,即单位页的数量
};

struct buddy2 tree[80000]; //数组形式的完全二叉树
//0为根，12为左右子节点，以此类推

struct allocBlock block[80000]; 
int blockNum;

/*----------------------------------------------------*/

void buddy2_new( int size ) { //初始化二叉树上的节点
  cprintf("buddy2_new\n");
  unsigned node_size;
  blockNum=0; //设置已分配块数为0
  int i;
  if (size < 1 || !IS_POWER_OF_2(size)) //分配不合理
    return;

  tree[0].size = size; //根节点的size即为管理内存的总数
  node_size = size * 2;
  for (i = 0; i < 2 * size - 1; ++i) { //节点总数为i<2*size
    if (IS_POWER_OF_2(i+1))  
      node_size /= 2; //节点管理内存数减半
    tree[i].longest[0] = node_size; //记录对应内存块大小
  }
  return ;
}

static void
buddy_init(void) {
    list_init(&free_list);
    nr_free = 0;
}

static void
buddy_init_memmap(struct Page *base, size_t n) { //初始化内存映射
    assert(n > 0); //断言，确保n>0
    cprintf("buddy_init_memmap\n");
    struct Page *p = base;  
    for (; p != base + n; p ++) {  //初始化
        assert(PageReserved(p)); //确保页面是保留的，即可给内核使用的
        p->flags = 0;
        p->property = 1; //每页大小为1
        set_page_ref(p, 0); //引用计数置为0
        SetPageProperty(p); //将p设置为页首，即每个地址都是一个页
        list_add(&free_list, &(p->page_link)); //加在前面
    }
    nr_free += n;
    buddy2_new(MIN_BLOCK(n)); //初始化二叉树
}

int buddy2_alloc( int size) {
  cprintf("buddy2_alloc\n");
  unsigned index = 0;
  unsigned node_size;
  unsigned offset = 0;
  //if (self==NULL)
    //return -1;
  if (size <= 0) //分配不合理
    size = 1;
  else if (!IS_POWER_OF_2(size))
    size = MIN_BLOCK(size);
    //将size调整到2的幂的大小
  if (tree[index].longest[0] < size) //检查是否超过最大限度，超过则无法分配
    return -1;
    //深度优先搜索
  //cprintf("node_size=%d\n",tree[0].size);
  //cprintf("size is:%d\n",size);
  for(node_size = tree[0].size; node_size != size; node_size /= 2 ) {
    //cprintf("self[index].longest is: %d\n",self->longest[]);
    //cprintf("node_size is:%d\n",node_size);
    //cprintf("size is:%d\n",size);
    int left_longest=tree[LEFT_LEAF(index)].longest[0];
    int right_longest=tree[RIGHT_LEAF(index)].longest[0];
    if(left_longest>=size){ //左右两个子节点都符合的话，找更小的那个
        //cprintf("left.longest is:%d\n",left_longest);
        if(right_longest>=size)
        {
          //cprintf("right.longest is:%d\n",right_longest);
          index=left_longest<=right_longest?LEFT_LEAF(index):RIGHT_LEAF(index);
         // cprintf("index=%d\n",index);
        }
            
        else
        {
        	//cprintf("LEFT_LEAF(index)%d=",LEFT_LEAF(index));
		index=LEFT_LEAF(index);
		//cprintf("index=%d\n",index);
        }         
    }
    else
    {
    //cprintf("RIGHT_LEAF(index)%d=",RIGHT_LEAF(index));
    	index = RIGHT_LEAF(index);
    	//cprintf("index=%d\n",index);
    }
        //cprintf("for_end_node_size is:%d\n",node_size);
    //cprintf("for_end_size is:%d\n",size);
    //if(node_size==2 && size==1)
    	//break;
  }
  tree[index].longest[0] = 0; //说明该节点已被使用，无法再被分配
  offset = (index + 1) * node_size - tree[0].size;
  while (index) { //回溯更新父节点的使用情况
  //因为小块内存被占用，大块就不能分配了，因此取左右子树较大值
    index = PARENT(index);
     // cprintf("index=%d\n",index);
    tree[index].longest[0]=
      MAX(tree[LEFT_LEAF(index)].longest[0], tree[RIGHT_LEAF(index)].longest[0]);
  }
  //cprintf("1");
  return offset; //返回适合块的索引
}

static struct Page *
buddy_alloc_pages(size_t n) {
    assert(n > 0);
    cprintf("buddy_alloc_pages\n");
    if (n > nr_free) {
        return NULL;
    }
    struct Page *page = NULL, *p;
    list_entry_t *le = &free_list;

    block[blockNum].offset=buddy2_alloc(n);
    //定位到偏移量所对应的页
    for(int i=0;i<=block[blockNum].offset;i++) 
        le=list_next(le);
    page=le2page(le, page_link); //从这里开始分配页

    //if (page != NULL) {
        block[blockNum].base=page;
        int pagenum=MIN_BLOCK(n); //分配页的数量调整为2^k
        block[blockNum].pageNum=pagenum;
        blockNum++;
        page->property=pagenum;  //合成一个大页
        for(int i=0;i<pagenum;i++) //因为已合成，因此清空后面的页
        {
            p=le2page(le,page_link); //开始改变页的状态
            ClearPageProperty(p);
            le=list_next(le);
        }
        nr_free-=pagenum; //剩余的空闲页
        page->property=n;
   // } 
    return page;
}

static void
buddy_free_pages(struct Page *base, size_t n) {
    cprintf("buddy_free_pages\n");
    unsigned node_size, index = 0;
    unsigned left_longest, right_longest;
    //找到分配的块
    int block_pos;
    for(block_pos=0;block_pos<blockNum;block_pos++)
        if(block[block_pos].base==base)
            break;
    int offset=block[block_pos].offset;
    int pagenum=MIN_BLOCK(n);

    assert(offset>=0 && offset<tree[0].size );
    node_size=1;
    index = offset + tree[0].size - 1;
    nr_free += pagenum; 
    tree[index].longest[0]=node_size;

    struct Page * p;
    list_entry_t* le = list_next(&free_list);
    for(int i=0;i<offset;i++)
        le=list_next(le); //找到当时分配的首页
    for(int i=0;i<pagenum;i++) //收回分配页
    {
        p=le2page(le,page_link);
        p->flags = 0;
        p->property=1;
        //set_page_ref(p, 0); //引用计数置为0
        SetPageProperty(p);
        le=list_next(le);
    }
    //longest恢复到原来满状态的值。继续向上回溯，检查是否存在合并的块，
    //依据就是左右子树longest的值相加是否等于原空闲块满状态的大小，
    //如果能够合并，就将父节点longest标记为相加的和

    while (index) { //迭代修改二叉树祖先节点的数值
    index = PARENT(index);
    node_size *= 2;
    left_longest = tree[LEFT_LEAF(index)].longest[0];
    right_longest = tree[RIGHT_LEAF(index)].longest[0];
    if (left_longest + right_longest == node_size)
      tree[index].longest[0] = node_size;
    else
      tree[index].longest[0] = MAX(left_longest, right_longest);
    }

    //删除分配块，把所有数据前移
    for(int i=block_pos;i<blockNum-1;i++)
        block[i]=block[i+1];
    blockNum--;
}

static size_t
buddy_nr_free_pages(void) {
    return nr_free;
}


void print_tree()
{
  int i=0;
  while ((i<16*2-1))
  {
   int temp=2*i+1;
 for(;i<temp;i++)
 {
 cprintf("%u ",tree[i].longest[0]);
 }
 cprintf("\n");
 }
  cprintf("---------------------------\n");
}

static void
buddy_check(void) {
    struct Page  *A, *B, *C , *D;
    A = B =C = D =NULL;
    assert((A = alloc_page()) != NULL);
    assert((B = alloc_page()) != NULL);
    assert((C = alloc_page()) != NULL);
    assert((D = alloc_page()) != NULL);
    assert( A != B && B!= C && C!=D && D!=A);
    assert(page_ref(A) == 0 && page_ref(B) == 0 && page_ref(C) == 0 && page_ref(D) == 0);
    //free page就是free pages(A,1) (函数扩展)
    free_page(A);
    free_page(B);
    free_page(C);
    free_page(D); 
    
    cprintf("------------------------------checking-------------------------------\n");
      print_tree();
    A=alloc_pages(3);
    print_tree();
    B=alloc_pages(1);
    print_tree();
    C=alloc_pages(5);
    print_tree();
    D=alloc_pages(2);
    print_tree();
    free_page(B);
    print_tree();
    free_page(D);
    print_tree();
    free_page(A);
    print_tree();
    free_page(C);
    print_tree();
    cprintf("------------------------------check succeeded---------------------------\n");
}


const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};

