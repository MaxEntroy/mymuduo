#pragma once

#include "common.h"

namespace mymuduo {

namespace CurrentThread {

extern __thread int t_cached_tid;
void CacheTid();

inline int Tid() {
  if (UNLIKELY(!t_cached_tid)) {
    CacheTid();
  }
  return t_cached_tid;
}

}  // namespace CurrentThread

}  // namespace mymuduo
