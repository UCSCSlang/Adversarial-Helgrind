#include "hg_adv.h"

#define ADV_DEBUG 0
#define MAX_ARRAY_SIZE 2048
#define DROP_SIZE 1023

VgHashTable write_buffers_8;
VgHashTable write_buffers_16;
VgHashTable write_buffers_32;
VgHashTable write_buffers_64;

typedef struct _AddrWBNode {
  void *next;
  Addr addr;
  XArray *wb;
} AddrWBNode;

typedef struct _AdvWB {
  UInt tid; // Thread ID
  XArray *wb; // Relative WriteBuffer
} AdvWB;

#define GET_WB_DECL(sz)					\
  XArray *get_or_create_wb_##sz(Thread *t, Addr a);	\
  XArray *get_random_wb_##sz(Thread *t, Addr a);

GET_WB_DECL(8)
GET_WB_DECL(16)
GET_WB_DECL(32)
GET_WB_DECL(64)

void adv_init(void) {
  write_buffers_8 = VG_(HT_construct)("hg_writebuffer_map");
  write_buffers_16 = VG_(HT_construct)("hg_writebuffer_map");
  write_buffers_32 = VG_(HT_construct)("hg_writebuffer_map");
  write_buffers_64 = VG_(HT_construct)("hg_writebuffer_map");
}

#define ADV_TRACK_ADDR(sz)						\
  void adv_track_addr_##sz(Addr a) {					\
    AddrWBNode *node = VG_(HT_lookup)(write_buffers_##sz, (UInt) a);	\
    if (node != NULL)							\
      return; \
    XArray *wb = VG_(newXA)(HG_(zalloc),				\
			    "addr_wb", HG_(free), sizeof(AdvWB));	\
    node = HG_(zalloc)("addr_wb_node", sizeof(AddrWBNode));		\
    node->addr = a;							\
    node->wb = wb;							\
    VG_(HT_add_node)(write_buffers_##sz, node);			\
    if (ADV_DEBUG)							\
      VG_(printf)("tracking address 0x%x\n", (UInt)a);			\
  }

ADV_TRACK_ADDR(8)
ADV_TRACK_ADDR(16)
ADV_TRACK_ADDR(32)
ADV_TRACK_ADDR(64)

/**
 * Handles the event of a read to a memory location
 *
 * Use one of the most outdated values found in a writebuffer != wb[t] if there's any
 * and write it to the specified memory location (to be used from the subsequent memory access)
 *
 * @param t operating thread
 * @param a the address at which the read is attempted
 *
 */
#define ADV_READ(sz)							\
  void adv_read_##sz(Thread *t, Addr a) {				\
    XArray *wb = get_random_wb_##sz(t, a);				\
    if (wb == NULL)							\
      return;								\
									\
    uint##sz##_t val;							\
    UInt size = VG_(sizeXA)(wb);					\
    UInt index = VG_(random)(NULL) % size;				\
    if (size > 0)							\
      val = *((uint##sz##_t*)VG_(indexXA)(wb, 0));			\
    else if (ADV_DEBUG) {						\
      val = *(uint##sz##_t*)a;						\
      VG_(printf)("read val %x at 0x%x [%s] thread: %d\n",		\
		  val, (UInt)a, size>0 ? "s" : "f", t->coretid);	\
      if (*(uint##sz##_t*)a != val)					\
	VG_(printf)("actual val %x\n", (uint##sz##_t)*(uint##sz##_t*)a); \
    }									\
    if (size > 0)							\
      (*(uint##sz##_t*)a) = val;					\
  }

ADV_READ(8)
ADV_READ(16)
ADV_READ(32)
ADV_READ(64)

/**
 * Records a write in the writebuffer
 *
 * @param t operating thread
 * @param a the memory location (address) at which the write is done
 * @param v the written value
 */
#define ADV_WRITE(sz)						 \
  void adv_write_##sz(Thread *t, Addr a, uint##sz##_t v) {	 \
    XArray *wb = get_or_create_wb_##sz(t, a);			 \
    if (wb == NULL)						 \
      return;							 \
    UInt size = VG_(sizeXA)(wb);				 \
    if (size >= MAX_ARRAY_SIZE)					 \
      VG_(dropTailXA)(wb, DROP_SIZE);				 \
    UInt idx = VG_(addToXA)(wb, &v);				 \
    if (ADV_DEBUG) {						 \
      VG_(printf)("write val %x at 0x%x thread: %d index: %d\n", \
		  v, (UInt)a, t->coretid, idx);			 \
    }								 \
  }

ADV_WRITE(8)
ADV_WRITE(16)
ADV_WRITE(32)
ADV_WRITE(64)

/**
 *  Flushes the writebuffer for the current thread
 *
 *  @param t operating thread
 */
#define ADV_FENCE(sz)							\
  void adv_fence_##sz(Thread *t); \
  void adv_fence_##sz(Thread *t) {					\
    VG_(HT_ResetIter)(write_buffers_##sz);				\
    AddrWBNode *node;							\
    while ((node = VG_(HT_Next)(write_buffers_##sz)) != NULL) {		\
      Addr a = node->addr;						\
      XArray *awb = node->wb;						\
      Int i, j;								\
      for (i = 0; i < VG_(sizeXA)(awb); i++) {				\
	XArray *wb = ((AdvWB*)VG_(indexXA)(awb, i))->wb;		\
	for (j = 0; j < VG_(sizeXA)(wb); j++)				\
	  *((uint##sz##_t *)a) = *((uint##sz##_t*)VG_(indexXA)(wb, j)); \
	VG_(dropTailXA)(wb, VG_(sizeXA)(wb));				\
      }									\
    }									\
    if (ADV_DEBUG) {							\
      VG_(printf)("memory fence\n");					\
    }									\
  }

ADV_FENCE(8)
ADV_FENCE(16)
ADV_FENCE(32)
ADV_FENCE(64)

void adv_fence(Thread *t) {
  adv_fence_8(t);
  adv_fence_16(t);
  adv_fence_32(t);
  adv_fence_64(t);
}

/*
 * Gets the writebuffer for thread t and address a if a is tracked,
 * if not found it creates a new one. Returns NULL if a is not tracked.
 */
#define GET_OR_CREATE_WB(sz)						\
  XArray *get_or_create_wb_##sz(Thread *t, Addr a) {			\
    AddrWBNode *node = VG_(HT_lookup)(write_buffers_##sz, (UInt)a);	\
    if (node == NULL)							\
      return NULL;							\
    Int i;								\
    XArray *wb = node->wb;						\
    for (i = 0; i < VG_(sizeXA)(wb); i++) {				\
      AdvWB *awb = VG_(indexXA)(wb, i);					\
      if (awb->tid == t->coretid)					\
	return awb->wb;							\
    }									\
    AdvWB awb;								\
    awb.tid = t->coretid;						\
    awb.wb = VG_(newXA)(HG_(zalloc), "addr_thr_wb",			\
			HG_(free), sizeof(uint##sz##_t));		\
    VG_(addToXA)(wb, &awb);						\
    return awb.wb;							\
  }

GET_OR_CREATE_WB(8)
GET_OR_CREATE_WB(16)
GET_OR_CREATE_WB(32)
GET_OR_CREATE_WB(64)

#define GET_RANDOM_WB(sz)						\
  XArray *get_random_wb_##sz(Thread *t, Addr a) {			\
    AddrWBNode *node = VG_(HT_lookup)(write_buffers_##sz, (UInt)a);	\
    if (node == NULL)							\
      return NULL;							\
    XArray *wb = node->wb;						\
    Int size = VG_(sizeXA)(wb);						\
    if (size == 1) {							\
      AdvWB *awb = ((AdvWB *)VG_(indexXA)(wb, 0));			\
      if (awb->tid == t->coretid)					\
	return NULL;							\
      else								\
	return awb->wb;							\
    } else if (size > 1) {						\
      AdvWB *awb = ((AdvWB*)VG_(indexXA)(wb, VG_(random)(NULL) % VG_(sizeXA)(wb))); \
      while (awb->tid == t->coretid)					\
	awb = ((AdvWB*)VG_(indexXA)(wb, VG_(random)(NULL) % VG_(sizeXA)(wb))); \
      return awb->wb;							\
    } else return NULL;							\
  }

GET_RANDOM_WB(8)
GET_RANDOM_WB(16)
GET_RANDOM_WB(32)
GET_RANDOM_WB(64)

