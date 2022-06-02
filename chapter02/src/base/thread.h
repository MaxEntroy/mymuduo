#include <pthread.h>

#include "count_down_latch.h"

namespace mymuduo {

class Thread {
 public:
  Thread(const Thread&) = delete;
  Thread& operator=(const Thread&) = delete;
};

}  // namespace mymuduo
