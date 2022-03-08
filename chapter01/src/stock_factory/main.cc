#include "stock_factory.h"

int main(void) {
  mymuduo::version1::StockFactory s1;
  mymuduo::version2::StockFactory s2;
  mymuduo::version3::StockFactory s3;
  auto s4 = std::make_shared<mymuduo::version4::StockFactory>();
  auto s5 = std::make_shared<mymuduo::version5::StockFactory>();
  return 0;
}
