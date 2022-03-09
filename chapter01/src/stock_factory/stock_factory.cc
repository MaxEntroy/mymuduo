#include "stock_factory.h"

namespace mymuduo {

namespace version1 {

StockFactory::StockPtr StockFactory::GetStock(const std::string& key) {
  MutexLockGuard mtx_guard(mtx_);
  auto it = stock_factory_.find(key);
  if (it == stock_factory_.end()) {
    it->second = std::make_shared<Stock>(key);
    // do something with new stock
  }
  return it->second;
}

}  // namespace version1

namespace version2 {

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

}  // namespace version2

namespace version3 {

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

}  // namespace version3

namespace version4 {

StockFactory::StockPtr StockFactory::GetStock(const std::string& key) {
  std::shared_ptr<Stock> local_ptr;
  MutexLockGuard mtx_guard(mtx_);
  auto& wptr = stock_factory_[key];
  local_ptr = wptr.lock();
  if (!local_ptr) {
    using namespace std::placeholders;
    local_ptr.reset(new Stock(key), std::bind(&StockFactory::StockDeleter, shared_from_this(), _1));
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

}  // namespace version4

namespace version5 {

StockFactory::StockPtr StockFactory::GetStock(const std::string& key) {
  std::shared_ptr<Stock> local_ptr;
  MutexLockGuard mtx_guard(mtx_);
  auto& wptr = stock_factory_[key];
  local_ptr = wptr.lock();
  if (!local_ptr) {
    using namespace std::placeholders;
    local_ptr.reset(new Stock(key), std::bind(&StockFactory::StockDeleter, weak_from_this(), _1));
    wptr = local_ptr;
  }
  return local_ptr;
}

void StockFactory::StockDeleter(const std::weak_ptr<StockFactory>& wptr, Stock* stock) {
  auto sptr = wptr.lock();
  if (sptr) {
    sptr->RemoveStock(stock);
  }
  delete stock;
}

void StockFactory::RemoveStock(Stock* stock) {
  if (stock) {
    MutexLockGuard mtx_guard(mtx_);
    stock_factory_.erase(stock->GetKey());
  }
}

}  // namespace version5

}  // namespace mymuduo
