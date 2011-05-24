#ifndef HG_ADV_H
#define HG_ADV_H

#include "libvex_basictypes.h"
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

/* Adversarial memory */

void adv_track_address(Addr a);
void adv_init(void);
void adv_read32(Thread *t, Addr a);
void adv_write32(Thread *t, Addr a, UInt v);
void adv_fence(Thread *t);

#endif
