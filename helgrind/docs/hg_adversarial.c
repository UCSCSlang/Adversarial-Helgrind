#include "hg_adversarial.h"

VgHashTable write_buffers;

typedef struct _AddrVal {
  Addr addr;
  int val;
} AddrVal;

typedef struct _AdvWBNode {
  void *next;
  Thread *thread;
  XArray *wb;
} AdvWBNode;

void adv_init(Thread *thread)
{
  write_buffers = VG_(HT_construct)("hg_writebuffer_map");
  XArray *wb = VG_(newXA)(HG_(zalloc), "hg_wb_main", HG_(free), sizeof(AddrVal));
  VG_(HT_add_node)(write_buffers, wb);
}

void adv_thread_create(Thread *thread, Thread *child)
{
  adv_fence(thread);
  XArray *wb = VG_(newXA)(HG_(zalloc), "hg_wb_child", HG_(free), sizeof(AddrVal));
  AdvWBNode *wbNode = HG_(zalloc)("wb_node", sizeof(AdvWBNode));
  wbNode->wb = wb;
  wbNode->thread = child;
  VG_(HT_add_node)(write_buffers, wb);
}

void adv_fence(Thread *thread)
{
  XArray *wb = VG_(HT_lookup)(write_buffers, thread);
  int i, val;
  for (i = 0; i < VG_(sizeXA)(wb); i++) {
    AddrVal *av = (AddrVal *)VG_(indexXA)(wb, i);
    //val = *(av->addr);
  }
  VG_(dropTailXA)(wb, VG_(sizeXA)(wb));
}

int adv_read32(Thread *thread, Addr addr)
{
  XArray *wb = VG_(HT_lookup)(write_buffers, thread);
  int i, val;
  for (i = 0; i < VG_(sizeXA)(wb); i++) {
    AddrVal *av = (AddrVal *)VG_(indexXA)(wb, i);
    //val = *(av->addr);
  }
  return val;
}

void adv_thread_join(Thread *thread, Thread *child)
{
  adv_fence(thread);
  adv_fence(child);
  XArray *wb = VG_(HT_lookup)(write_buffers, child);
  VG_(HT_remove)(write_buffers, child);
  VG_(deleteXA)(wb);
}

void adv_write32(Thread *thread, Addr addr, int val)
{
  XArray *wb = VG_(HT_lookup)(write_buffers, thread);
  AddrVal *av = HG_(zalloc)("av", sizeof(AddrVal));
  VG_(addToXA)(wb, av);
}

