#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "mutex.h"

namespace mymuduo {

class Stock {
 public:
  explicit Stock(const std::string& key) : key_(key) {}
  std::string GetKey() const { return key_; }

 private:
  std::string key_;
};

/**
 * version 1
class StockFactory {
 public:
  using StockPtr = std::shared_ptr<Stock>;

  StockFactory() = default;

  StockFactory(const StockFactory&) = delete;
  StockFactory& operator=(const StockFactory&) = delete;

  StockPtr GetStock(const std::string& key);

 private:
  MutexLock mtx_;
  std::unordered_map<std::string, StockPtr> stock_factory_;
};
*/

/**
 * version 2
class StockFactory {
 public:
  using StockPtr = std::weak_ptr<Stock>;

  StockFactory() = default;

  StockFactory(const StockFactory&) = delete;
  StockFactory& operator=(const StockFactory&) = delete;

  StockPtr GetStock(const std::string& key);

 private:
  MutexLock mtx_;
  std::unordered_map<std::string, StockPtr> stock_factory_;
};
*/

class StockFactory {
 public:
  using StockPtr = std::weak_ptr<Stock>;

  StockFactory() = default;

  StockFactory(const StockFactory&) = delete;
  StockFactory& operator=(const StockFactory&) = delete;

  StockPtr GetStock(const std::string& key);

 private:
  void StockDeleter(Stock*);

 private:
  MutexLock mtx_;
  std::unordered_map<std::string, StockPtr> stock_factory_;
};

}  // namespace mymuduo
