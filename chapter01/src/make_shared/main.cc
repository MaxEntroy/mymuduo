#include <iostream>
#include <memory>

struct ReallyBigType {};

// The reference count controls the lifetime of the pointed-to-object.
// The weak count does not, but does control (or participate in control of) the lifetime of the control block.
void test_make_shared() {
  std::weak_ptr<ReallyBigType> wp;
  {
    auto sp = std::make_shared<ReallyBigType>(); // ref count = 1;
    wp = sp;  // weak count = 1;
  }
  // The reference count becomes zero, object is destroyed.
  // But the weak count is not zero, control block remains still.
  // Because make_shared might perform an optimisation where it allocates the storage for the object and the control block in one go.
  // This means that the storage for the int can’t be deallocated until the storage for the control block can also be deallocated.
  // The lifetime of the object ends when the reference count hits 0, but the storage for it is not deallocated until the weak count hits 0.
}

void test_new() {
  std::weak_ptr<ReallyBigType> wp;
  {
    std::shared_ptr<ReallyBigType> sp(new ReallyBigType); // ref count = 1;
    wp = sp;  // weak count = 1;
  }
  // during this period, only memory for the control block
  // remains allocated.
}

int main(void) {
  return 0;
}
