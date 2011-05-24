#include "hg_adv.h"

#define ADV_DEBUG 0

VgHashTable write_buffers; // Write Buffers, by thread

typedef struct _AddrWBNode {
  void *next;
  Addr addr;
  XArray *wb;
} AddrWBNode;

typedef struct _AdvWB {
  UInt tid; // Thread ID
  XArray *wb; // Relative WriteBuffer
} AdvWB;

XArray *get_or_create_wb(Thread *t, Addr a);
XArray *get_random_wb(Thread *t, Addr a);

void adv_init(void) {
	write_buffers = VG_(HT_construct)("hg_writebuffer_map");
}

void adv_track_address(Addr a) {
	AddrWBNode *node = VG_(HT_lookup)(write_buffers, (UInt) a); // FIXME: doesn't work in 64 bit environments!
	if (node != NULL)
		return; // Address a is already tracked
	// Address writebuffer
	XArray *wb = VG_(newXA)(HG_(zalloc), "addr_wb", HG_(free), sizeof(AdvWB));
	node = HG_(zalloc)("addr_wb_node", sizeof(AddrWBNode));
	node->addr = a;
	node->wb = wb;
	VG_(HT_add_node)(write_buffers, node);
#if ADV_DEBUG
	VG_(printf)("tracking address 0x%x\n", (UInt)a);
#endif
}

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
void adv_read32(Thread *t, Addr a) {
	XArray *wb = get_random_wb(t, a);
	if (wb == NULL)
		return;

	UInt val, size = VG_(sizeXA)(wb);
	UInt index = VG_(random)(NULL) % size;
	if (size > 0)
		*(UInt*)a = val = *((UInt*)VG_(indexXA)(wb, index));
#if ADV_DEBUG
	else
		val = *(UInt*)a;
	VG_(printf)("read val %x at 0x%x [%s] thread: %d\n", val, (UInt)a, size>0 ? "s" : "f", t->coretid);
	if (*(UInt*)a != val)
		VG_(printf)("actual val %x\n", (UInt)*(UInt*)a);
#endif
}

/**
 * Records a write in the writebuffer
 *
 * @param t operating thread
 * @param a the memory location (address) at which the write is done
 * @param v the written value
 */
void adv_write32(Thread *t, Addr a, UInt v) {
	XArray *wb = get_or_create_wb(t, a);
	if (wb == NULL)
		return;
	UInt idx = VG_(addToXA)(wb, &v);
#if ADV_DEBUG
	VG_(printf)("write val %x at 0x%x thread: %d index: %d\n", v, (UInt)a, t->coretid, idx);
#endif
}

/**
 *  Flushes the writebuffer for the current thread
 *
 *  @param t operating thread
 */
void adv_fence(Thread *t) {
	VG_(HT_ResetIter)(write_buffers);
	AddrWBNode *node;
	while ((node = VG_(HT_Next)(write_buffers)) != NULL) {
		Addr a = node->addr;
		XArray *awb = node->wb;
		Int i, j;
		for (i = 0; i < VG_(sizeXA)(awb); i++) {
			XArray *wb = ((AdvWB*)VG_(indexXA)(awb, i))->wb;
			for (j = 0; j < VG_(sizeXA)(wb); j++)
				*((UInt *)a) = *((UInt*)VG_(indexXA)(wb, j));
			VG_(dropTailXA)(wb, VG_(sizeXA)(wb));
		}
	}
#if ADV_DEBUG
	VG_(printf)("memory fence\n");
#endif
}


/*
 * Gets the writebuffer for thread t and address a if a is tracked,
 * if not found it creates a new one. Returns NULL if a is not tracked.
 */
XArray *get_or_create_wb(Thread *t, Addr a) {
	AddrWBNode *node = VG_(HT_lookup)(write_buffers, (UInt)a); // FIXME: doesn't work in 64 bit environments!
	if (node == NULL)
		return NULL;
	Int i;
	XArray *wb = node->wb;
	for (i = 0; i < VG_(sizeXA)(wb); i++) {
		AdvWB *awb = VG_(indexXA)(wb, i);
		if (awb->tid == t->coretid)
			return awb->wb;
	}
	// Thread not found, add it
	AdvWB awb;
	awb.tid = t->coretid;
	awb.wb = VG_(newXA)(HG_(zalloc), "addr_thr_wb", HG_(free), sizeof(UInt));
	VG_(addToXA)(wb, &awb);
	return awb.wb;
}

XArray *get_random_wb(Thread *t, Addr a) {
	AddrWBNode *node = VG_(HT_lookup)(write_buffers, (UInt)a); // FIXME: doesn't work in 64 bit environments!
	if (node == NULL)
		return NULL;
	XArray *wb = node->wb;
	Int size = VG_(sizeXA)(wb);
	if (size == 1) {
		AdvWB *awb = ((AdvWB *)VG_(indexXA)(wb, 0));
		if (awb->tid == t->coretid)
			return NULL;
		else
			return awb->wb;
	} else if (size > 1) {
		AdvWB *awb = ((AdvWB*)VG_(indexXA)(wb, VG_(random)(NULL) % VG_(sizeXA)(wb)));
		while (awb->tid == t->coretid)
			awb = ((AdvWB*)VG_(indexXA)(wb, VG_(random)(NULL) % VG_(sizeXA)(wb)));
		return awb->wb;
	} else return NULL;
}
