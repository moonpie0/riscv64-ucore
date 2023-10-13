#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddy_system.h>

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
int PARENT (int index){ return (index+1)/2-2;}


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

struct buddy2 tree[40000]; //数组形式的完全二叉树
//0为根，12为左右子节点，以此类推

struct allocBlock block[40000]; 
int blockNum;

/*----------------------------------------------------*/

void buddy2_new( int size ) { //初始化二叉树上的节点
  unsigned node_size;
  int i;
  if (size < 1 || !IS_POWER_OF_2(size))
    return NULL;

  tree[0].size = size;
  node_size = size * 2;
  for (i = 0; i < 2 * size - 1; ++i) {
    if (IS_POWER_OF_2(i+1))
      node_size /= 2;
    tree[i].longest[0] = node_size; //记录对应内存块大小
  }
}

static void
buddy_init(void) {
    list_init(&free_list);
    nr_free = 0;
}

static void
buddy_init_memmap(struct Page *base, size_t n) { //初始化内存映射
    assert(n > 0); //断言，确保n>0
    blockNum=0; //设置已分配块数为0
    struct Page *p = base;  
    for (; p != base + n; p ++) {  //初始化
        assert(PageReserved(p)); //确保页面是保留的，即可给内核使用的
        p->flags = 0;
        p->property = 1; //每页大小为1
        set_page_ref(p, 0); //引用计数置为0
        SetPageProperty(p);
        list_add(&free_list, &(p->page_link)); //加在前面
    }
    nr_free += n;
    buddy2_new(MIN_BLOCK(n));
}

int buddy2_alloc(struct buddy2* self, int size) {
  unsigned index = 0;
  unsigned node_size;
  unsigned offset = 0;
  if (self==NULL)
    return -1;
  if (size <= 0) //分配不合理
    size = 1;
  else if (!IS_POWER_OF_2(size))
    size = MIN_BLOCK(size);
    //将size调整到2的幂的大小
  if (self->longest[index] < size) //检查是否超过最大限度，超过则无法分配
    return -1;
    //深度优先搜索
  for(node_size = self->size; node_size != size; node_size /= 2 ) {
    int left_longest=self[LEFT_LEAF(index)].longest[0];
    int right_longest=self[RIGHT_LEAF(index)].longest[0];
    if(left_longest>=size){ //左右两个子节点都符合的话，找更小的那个
        if(right_longest>=size)
            index=left_longest<=right_longest?LEFT_LEAF(index):RIGHT_LEAF(index);
        else
            index=LEFT_LEAF(index);
    }
    else
        index = RIGHT_LEAF(index);
  }
  self[index].longest[0] = 0; //说明该节点已被使用，无法再被分配
  offset = (index + 1) * node_size - self->size;
  while (index) { //回溯更新父节点的使用情况
  //因为小块内存被占用，大块就不能分配了，因此取左右子树较大值
    index = PARENT(index);
    self[index].longest[0] =
      MAX(self[LEFT_LEAF(index)].longest[0], self[RIGHT_LEAF(index)].longest[0]);
  }
  return offset; //返回适合块的索引
}

static struct Page *
buddy_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > nr_free) {
        return NULL;
    }
    struct Page *page = NULL, *p;
    list_entry_t *le = &free_list;

    block[blockNum].offset=buddy2_alloc(tree,n);
    //定位到偏移量所对应的页
    for(int i=0;i<=block[blockNum].offset;i++) 
        le=list_next(le);
    page=le2page(le, page_link); //从这里开始分配页

    if (page != NULL) {
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
        return page;
    } 
}

static void
buddy_free_pages(struct Page *base, size_t n) {
    unsigned node_size, index = 0;
    unsigned left_longest, right_longest;
    //找到分配的块
    int block_pos;
    for(block_pos=0;block_pos<blockNum;block_pos++)
        if(block[block_pos].base==base)
            break;
    int offset=block[block_pos].offset;
    int pagenum=MIN_BLOCK(n);

    assert(tree && offset>=0 && offset<tree[0].size );
    node_size=1;
    index = offset + tree[0].size - 1;
    nr_free += pagenum; 
    tree[index].longest[0]=node_size;

    struct Page * p;
    list_entry_t* le = &free_list;
    for(int i=0;i<offset;i++)
        le=list_next(le); //找到当时分配的首页
    for(int i=0;i<pagenum;i++) //收回分配页
    {
        p=le2page(le,page_link);
        p->flags = 0;
        p->property=1;
        set_page_ref(p, 0); //引用计数置为0
        SetPageProperty(p);
        le=list_next(le);
    }
    //longest恢复到原来满状态的值。继续向上回溯，检查是否存在合并的块，
    //依据就是左右子树longest的值相加是否等于原空闲块满状态的大小，
    //如果能够合并，就将父节点longest标记为相加的和

    while (index) { //迭代修改二叉树祖先节点的数值
    index = PARENT(index);
    node_size *= 2;
    left_longest = self[LEFT_LEAF(index)].longest[0];
    right_longest = self[RIGHT_LEAF(index)].longest[0];
    if (left_longest + right_longest == node_size)
      self[index].longest[0] = node_size;
    else
      self[index].longest[0] = MAX(left_longest, right_longest);
    }

    //删除分配块，把所有数据前移
    for(int i=block_pos;i<blockNum-1;i++)
        block[i]=block[i=1];
    blockNum--;
}

static size_t
default_nr_free_pages(void) {
    return nr_free;
}

static void
basic_check(void) {
    struct Page *p0, *p1, *p2;
    p0 = p1 = p2 = NULL;
    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(p0 != p1 && p0 != p2 && p1 != p2);
    assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);

    assert(page2pa(p0) < npage * PGSIZE);
    assert(page2pa(p1) < npage * PGSIZE);
    assert(page2pa(p2) < npage * PGSIZE);

    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    assert(alloc_page() == NULL);

    free_page(p0);
    free_page(p1);
    free_page(p2);
    assert(nr_free == 3);

    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(alloc_page() == NULL);

    free_page(p0);
    assert(!list_empty(&free_list));

    struct Page *p;
    assert((p = alloc_page()) == p0);
    assert(alloc_page() == NULL);

    assert(nr_free == 0);
    free_list = free_list_store;
    nr_free = nr_free_store;

    free_page(p);
    free_page(p1);
    free_page(p2);
}

// LAB2: below code is used to check the first fit allocation algorithm
// NOTICE: You SHOULD NOT CHANGE basic_check, default_check functions!
static void
default_check(void) {
    int count = 0, total = 0;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        assert(PageProperty(p));
        count ++, total += p->property;
    }
    assert(total == nr_free_pages());

    basic_check();

    struct Page *p0 = alloc_pages(5), *p1, *p2;
    assert(p0 != NULL);
    assert(!PageProperty(p0));

    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));
    assert(alloc_page() == NULL);

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    free_pages(p0 + 2, 3);
    assert(alloc_pages(4) == NULL);
    assert(PageProperty(p0 + 2) && p0[2].property == 3);
    assert((p1 = alloc_pages(3)) != NULL);
    assert(alloc_page() == NULL);
    assert(p0 + 2 == p1);

    p2 = p0 + 1;
    free_page(p0);
    free_pages(p1, 3);
    assert(PageProperty(p0) && p0->property == 1);
    assert(PageProperty(p1) && p1->property == 3);

    assert((p0 = alloc_page()) == p2 - 1);
    free_page(p0);
    assert((p0 = alloc_pages(2)) == p2 + 1);

    free_pages(p0, 2);
    free_page(p2);

    assert((p0 = alloc_pages(5)) != NULL);
    assert(alloc_page() == NULL);

    assert(nr_free == 0);
    nr_free = nr_free_store;

    free_list = free_list_store;
    free_pages(p0, 5);

    le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        count --, total -= p->property;
    }
    assert(count == 0);
    assert(total == 0);
}
//这个结构体在
const struct pmm_manager buddy_pmm_manager = {
    .name = "default_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = default_nr_free_pages,
    .check = default_check,
};

