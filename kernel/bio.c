// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"


#define NBUCKET 13

extern uint ticks;

struct {
  struct spinlock lock;
  struct buf buf[NBUF];//NBUF=30

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
  
  // struct buf *table[NBUCKET];
  // struct spinlock table_lock[NBUCKET];

} bcache;

struct buf *table[NBUCKET];
struct spinlock table_lock[NBUCKET];


static int show=0;

static void insert(uint dev,uint blockno,struct buf **p,struct buf *n,struct buf *b)
{
    // int i=blockno % NBUCKET;
    // acquire(&table_lock[i]);	
    // update value
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt=1;

    // insert a as the head of p
    b->next=n;
    *p=b;
    
    // release(&table_lock[i]);

}

static void update(uint dev,uint blockno,struct buf *b)
{
    // int i=blockno % NBUCKET;
    // acquire(&table_lock[i]);	
    // struct buf * ori=table[i];

    // update value
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt=1;
    
   /* if(b==b->next)
    {
         printf("diff same  table %p,next:%p,original table:%p\n",b,b->next,ori);
    }*/


    // release(&table_lock[i]);

}



static struct buf* get(uint dev, uint blockno)
{
   int i= blockno % NBUCKET; 
  

   struct buf *b=0;
   int j=0;
   for(b=table[i];b!=0;b=b->next,j++)
   {
     /* if(j>2000)
     {	     
        printf("get:%p\n",b);	
        panic("get block");	
     }*/
     if(b->dev==dev && b->blockno == blockno)
     {    
         break;
     }
   }
   return b;
 
}




static void remove(struct buf *b_cur)
{
   int i=b_cur->blockno % NBUCKET; 
  
   if(show) printf("remove:%d\n",i);

   struct buf *b=0;
   // acquire(&table_lock[i]);
   if(table[i]==b_cur) 
   {
        table[i]=table[i]->next;
       //	printf("remove:%p,new:%p\n",b_cur,table[i]);   
   }
   else
   {
     for(b=table[i];b!=0;b=b->next)
     {
       if(b->next==b_cur)
       {
         break;
       }
      }
     if(b!=0 && b->next==b_cur){   
        b->next=b_cur->next;
	/*
	if(b->next)printf("remove:%p,new:%p,%p\n",b_cur,b,b->next);   
	else printf("remove:%p empty,new:%p\n",b_cur,b);*/   

      }
   }
   b_cur->next=0;
   // release(&table_lock[i]);
   if(show) printf("remove exit:%d\n",i);

}


void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");


  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  
    initsleeplock(&b->lock, "buffer");
  }

  for(int i=0;i<NBUCKET;i++) 
  {  
      initlock(&table_lock[i], "bcache.bucket");
  }


  
  /*
  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }*/
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  
  
  struct buf *b=0;

  int i= blockno % NBUCKET;
  
  if(show)printf("bget:%d\n",i);
  acquire(&table_lock[i]);


  // Is the block already cached?
  if((b=get(dev,blockno))!=0)
  {
     b->refcnt++;
     release(&table_lock[i]);
     if(show) printf("bget exit:%d\n",i);
     acquiresleep(&b->lock);

     return b;
  }
  
  // release(&table_lock[i]); 

  // acquire(&bcache.lock);

  uint min_time_stamp=-1;
  struct buf *min_b=0;
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
     int j=b->blockno % NBUCKET;	  
     if(j!=i) acquire(&table_lock[j]);	  
	  
     if(b->refcnt==0 && b->time_stamp<min_time_stamp)
     {
	 min_time_stamp=b->time_stamp;
	 min_b=b;
     }
     if(j!=i) release(&table_lock[j]);
  }
  
  b=min_b;
  if(b!=0)
  {
     int j=b->blockno % NBUCKET;	  
     if(j!=i) acquire(&table_lock[j]);	  
     if( j != i  )//new block hash to the different bucket as the old block
     {
	    remove(b);//remove b from original table
             release(&table_lock[j]);
            // acquire(&table_lock[i]);	
             insert(dev,blockno,&table[i],table[i],b);// insert b to table[i]
            //  release(&table_lock[j]);	

      }	 
     else
     {
             // new block hash to the same bucket as the old block
	     // just update value,no need to insert b to table[i] 
	     update(dev,blockno,b); 
             // release(&table_lock[j]);

     }
	 release(&table_lock[i]);
	 // release(&bcache.lock);
	 if(show) printf("bget and bcache exit:%d\n",i);
         acquiresleep(&b->lock);
         return b;
    

  } 

  // if(min_time_stamp==-1) printf("no buffers\n");
  /* 
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
     int j=b->blockno % NBUCKET;	  
     acquire(&table_lock[j]);	  

     if(b->refcnt==0 && b->time_stamp==min_time_stamp)
     {
         if(show) printf("bget:bcache exit %d\n",i);

	 // if(table[i]!=b &&  b->blockno % NBUCKET != i  )//new block hash to the different bucket as the old block
	  if( j != i  )//new block hash to the different bucket as the old block
	 {
	     remove(b);//remove b from original table
             acquire(&table_lock[i]);	
             insert(dev,blockno,&table[i],table[i],b);// insert b to table[i]
             release(&table_lock[i]);	

	 }	 
	 else
	 {
             // new block hash to the same bucket as the old block
	     // just update value,no need to insert b to table[i] 

	     update(dev,blockno,b);
	 }
	 
	 
	 release(&table_lock[i]);
	 if(show) printf("bget and bcache exit:%d\n",i);
         acquiresleep(&b->lock);
         return b;
     }
     if(i!=j) release(&table_lock[j]);
  }*/
  panic("bget: no buffers");

	 

  
  /*
  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");*/
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
 
  releasesleep(&b->lock);
  int i=b->blockno % NBUCKET;
  if(show) printf("brelse:%d\n",i);

  acquire(&table_lock[i]);
  b->refcnt--;
  
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->time_stamp=ticks;
  }
  release(&table_lock[i]);
  if(show) printf("brelese exit:%d\n",i);

  /*
  releasesleep(&b->lock);

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  release(&bcache.lock);*/
}

void
bpin(struct buf *b) {
  /*
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
  */
  
  int i=b->blockno % NBUCKET;	
  acquire(&table_lock[i]);	
  b->refcnt++;
  release(&table_lock[i]);	
	     
 
}

void
bunpin(struct buf *b) {
  /*
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);*/

  int i=b->blockno % NBUCKET;	
  acquire(&table_lock[i]);	
  b->refcnt--;
  release(&table_lock[i]);	
		
  
}


