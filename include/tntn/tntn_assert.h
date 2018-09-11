#pragma once

#include <cassert>

#if !defined(TNTN_ASSERT_ACTIVE)
#    if(defined(DEBUG) || defined(TNTN_DEBUG)) && !defined(NDEBUG)
#        define TNTN_ASSERT_ACTIVE 1
#    else
#        define TNTN_ASSERT_ACTIVE 0
#    endif
#endif

#if TNTN_ASSERT_ACTIVE
#    define TNTN_ASSERT(x) assert(x)
#else
#    define TNTN_ASSERT(x)
#endif
