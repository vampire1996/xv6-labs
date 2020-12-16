// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.// defined by kernel.ld.


uint64 refCnt[(PHYSTOP-KERNBASE)/PGSIZE];//reference count for each physical page

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

int refCntHelper(uint64 pa,char func)
{
   // printf("cnt:%c\n",func);	
   if(pa>=KERNBASE && pa<=PHYSTOP)
   {
        
       int idx=(pa-KERNBASE)/PGSIZE;// get the index in array refCnt corresponding to phiscal address pa
       if(func=='+') refCnt[idx]++;
       else if(func=='-') refCnt[idx]=refCnt[idx]==0?0:refCnt[idx]-1;
       else if(func=='1') refCnt[idx]=1;
       else if(func=='v') return refCnt[idx];// get value
       return -1;
   }
   return 0;
}



void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
  memset(refCnt,0,sizeof(refCnt));
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{	
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
 
  
  // refCntHelper((uint64)pa,'-');

  if(refCntHelper((uint64)pa,'v')!=0) return;// only when ref cnt is 0,do a page need to be freed

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  
  // if(r) refCntHelper((uint64)r,'1');


  return (void*)r;
}
