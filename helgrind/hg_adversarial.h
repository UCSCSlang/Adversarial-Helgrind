#ifndef ADVERSARIAL_H
#define ADVERSARIAL_H


#include "pub_tool_basics.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_threadstate.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_hashtable.h"
#include "pub_tool_replacemalloc.h"
#include "pub_tool_machine.h"
#include "pub_tool_options.h"
#include "pub_tool_xarray.h"
#include "pub_tool_stacktrace.h"
#include "pub_tool_wordfm.h"
#include "pub_tool_debuginfo.h" // VG_(find_seginfo), VG_(seginfo_soname)
#include "pub_tool_redir.h"     // sonames for the dynamic linkers
#include "pub_tool_vki.h"       // VKI_PAGE_SIZE
#include "pub_tool_libcproc.h"  // VG_(atfork)
#include "pub_tool_aspacemgr.h" // VG_(am_is_valid_for_client)

#include "hg_basics.h"
#include "hg_wordset.h"
#include "hg_lock_n_thread.h"
#include "hg_errors.h"

#include "libhb.h"

#include "helgrind.h"

void adv_init(Thread *thread);

void adv_fence(Thread *thread);

int adv_read32(Thread *thread, Addr addr);

void adv_write32(Thread *thread, Addr addr, int val);

void adv_thread_create(Thread *thread, Thread *child);

void adv_thread_join(Thread *thread, Thread *child);

#endif
