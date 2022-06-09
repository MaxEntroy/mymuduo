#include "thread.h"

#include <unistd.h>
#include <sys/syscall.h>

#include "current_thread.h"

namespace mymuduo {

namespace internal {

pid_t GetTid() {
  return static_cast<pid_t>(::syscall(SYS_gettid));
}

}  // namespace internal

void CurrentThread::CacheTid() {
  if (!t_cached_tid) {
    t_cached_tid = internal::GetTid();
  }
}

}  // namespace mymuduo
