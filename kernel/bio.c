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

extern uint ticks; // 声明外部变量 (trap.c 中的 ticks)

struct {
  struct spinlock lock[NBUCKET]; // 桶锁
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET];
} bcache;

int hash(int x) {
  return x % NBUCKET;
}

void replace_cache(struct buf *b, uint dev, uint blockno) {
  b->dev = dev;
  b->blockno = blockno;
  b->refcnt = 1;
  b->valid = 0;
}

void
binit(void)
{
  struct buf *b;

  // 初始化哈希表的桶锁
  for (int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.lock[i], "bcache");
  } 

  // 初始化每块 buffer 的锁，并且双向链表变为单向链表，初始时全都接在 bcache.head[0] 后面
  bcache.head[0].next = &bcache.buf[0];
  for (int i = 0; i < NBUF - 1; i++) {
    b = &bcache.buf[i];
    b->next = b + 1;
    initsleeplock(&b->lock, "buffer");
  }
 
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int hashid = hash(blockno); // 获取哈希值，看看映射到哪个桶
  acquire(&bcache.lock[hashid]); // 获取这个桶的锁

  // Is the block already cached? 该 block 已经被 cached 了的情况
  for(b = bcache.head[hashid].next; b; b = b->next){ // 单向链表遍历
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[hashid]); // 返回前释放这个桶的锁
      acquiresleep(&b->lock); // 获取这个 buffer 的锁
      return b;
    }
  }

  // 如果该 block 没有被 cache
  // 1. 查找当前桶中是否有空闲块
  for (b = bcache.head[hashid].next; b; b = b->next) {
    if (b->refcnt == 0) { // 是一个空闲块
      replace_cache(b, dev, blockno);
      release(&bcache.lock[hashid]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 2. 查找其他桶中是否有空闲块，并且选择 LRU
  int bucket_id = -1; // 当前选中的空闲块所在的桶编号
  struct buf *pre;
  struct buf *take_buf = 0;
  struct buf *last_take = 0;
  uint time = __UINT32_MAX__;
  for (int i = 0; i < NBUCKET; i++) { // 遍历所有桶
    if (i == hashid) continue;
    acquire(&bcache.lock[i]); // 获取当前桶的锁
    for (b = bcache.head[i].next, pre = &bcache.head[i]; b; b = b->next, pre = pre->next) { // 遍历这个桶拥有的空闲块
      if (b->refcnt == 0 && b->ticks < time) { // 如果是空闲块，并且上次使用的时间更早
        time = b->ticks; // 更新时间戳
        last_take = pre; // 记录一下当前位置前一个块的为主子
        take_buf = b; // 目前选中的空闲块
        if (bucket_id != -1 && bucket_id != i && holding(&bcache.lock[bucket_id])) { // 如果前一个选中的块不在当前的桶中，则释放前一个块锁持有的桶锁
          release(&bcache.lock[bucket_id]);
        }
        bucket_id = i; // 更新桶编号
      }
    }
    if (bucket_id != i) { // 如果没有用到这个桶，则释放桶锁
      release(&bcache.lock[i]);
    }
  }

  if (!take_buf) { // 没有找到空闲块
    panic("bget: no buffers");
  }

  replace_cache(take_buf, dev, blockno); // 写 cache
  last_take->next = take_buf->next; // 链表中删除选中的空闲块
  take_buf->next = 0;
  release(&bcache.lock[bucket_id]);
  release(&bcache.lock[hashid]);
  for (b = &bcache.head[hashid]; b->next; b = b->next); // 获取映射到的桶的最后一项
  b->next = take_buf; // 移到最后面
  acquiresleep(&take_buf->lock);
  return take_buf;
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

  int hashid = hash(b->blockno);
  acquire(&bcache.lock[hashid]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->ticks = ticks;
  }
  release(&bcache.lock[hashid]);
}

void
bpin(struct buf *b) {
  int hashid = hash(b->blockno);
  acquire(&bcache.lock[hashid]);
  b->refcnt++;
  release(&bcache.lock[hashid]);
}

void
bunpin(struct buf *b) {
  int hashid = hash(b->blockno);
  acquire(&bcache.lock[hashid]);
  b->refcnt--;
  release(&bcache.lock[hashid]);
}

