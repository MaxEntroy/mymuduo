#include "current_thread.h"

namespace mymuduo {

namespace CurrentThread {

__thread int t_cached_tid = 0;

}  // namespace CurrentThread

}  // namespace mymuduo
