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

namespace version1 {

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

}  // namespace version1

namespace version2 {

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

}  // namespace version2

namespace version3 {

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

}  // namespace version4

namespace version4 {

class StockFactory : public std::enable_shared_from_this<StockFactory> {
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

}  // namespace version4

namespace version5 {

class StockFactory : public std::enable_shared_from_this<StockFactory> {
 public:
  using StockPtr = std::weak_ptr<Stock>;

  StockFactory() = default;

  StockFactory(const StockFactory&) = delete;
  StockFactory& operator=(const StockFactory&) = delete;

  StockPtr GetStock(const std::string& key);

 private:
  static void StockDeleter(const std::weak_ptr<StockFactory>& wptr, Stock* stock);
  void RemoveStock(Stock* stock);

 private:
  MutexLock mtx_;
  std::unordered_map<std::string, StockPtr> stock_factory_;
};

}  // namespace version5

}  // namespace mymuduo
