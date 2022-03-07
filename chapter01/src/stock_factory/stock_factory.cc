#include "stock_factory.h"

namespace mymuduo {

/**
 * version 1
StockFactory::StockPtr StockFactory::GetStock(const std::string& key) {
  MutexLockGuard mtx_guard(mtx_);
  auto it = stock_factory_.find(key);
  if (it == stock_factory_.end()) {
    it->second = std::make_shared<Stock>(key);
    // do something with new stock
  }
  return it->second;
}
*/

/** version 2
StockFactory::StockPtr StockFactory::GetStock(const std::string& key) {
  std::shared_ptr<Stock> local_ptr;
  MutexLockGuard mtx_guard(mtx_);
  auto& wptr = stock_factory_[key];
  local_ptr = wptr.lock();
  if (!local_ptr) {
    local_ptr.reset(new Stock(key));
    wptr = local_ptr;
  }
  return local_ptr;
}
*/

StockFactory::StockPtr StockFactory::GetStock(const std::string& key) {
  std::shared_ptr<Stock> local_ptr;
  MutexLockGuard mtx_guard(mtx_);
  auto& wptr = stock_factory_[key];
  local_ptr = wptr.lock();
  if (!local_ptr) {
    using namespace std::placeholders;
    local_ptr.reset(new Stock(key), std::bind(&StockFactory::StockDeleter, this, _1));
    wptr = local_ptr;
  }
  return local_ptr;
}

void StockFactory::StockDeleter(Stock* stock) {
  if (stock) {
    {
      MutexLockGuard mtx_guard(mtx_);
      stock_factory_.erase(stock->GetKey());
    }
    delete stock;
  }
}

}  // namespace mymuduo
