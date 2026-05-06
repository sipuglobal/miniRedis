#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// ============================================================
// Database：Mini Redis 的核心数据存储层
//
// 内存结构示意：
//
//   data_ (std::unordered_map)
//   ┌─────────────────────────────────────────┐
//   │  "name"  →  "james"                    │
//   │  "age"   →  "25"                       │
//   │  "city"  →  "beijing"                  │
//   └─────────────────────────────────────────┘
//
// std::unordered_map 在堆上分配内存，通过哈希桶存储 key-value 对。
// key 和 value 都是 std::string，string 内部也在堆上分配字符数组。
//
// LLDB 查看方式：
//   p db.data_          // 查看整个 map
//   p db.data_["name"]  // 查看某个 key 的值
// ============================================================

class Database {
public:
    // 设置键值对（key 不存在则插入，存在则覆盖）
    void set(const std::string& key, const std::string& value);

    // 获取值，返回指向 string 的指针；key 不存在时返回 nullptr
    // 注意：返回的指针指向 map 内部的 string，不要在 map 修改后继续使用
    const std::string* get(const std::string& key);

    // 删除 key，返回实际删除的数量（0 或 1）
    int del(const std::string& key);

    // 检查 key 是否存在
    bool exists(const std::string& key);

    // 返回所有 key 的列表
    std::vector<std::string> keys();

private:
    // 核心存储：哈希表
    // key   → std::string（客户端传来的键名）
    // value → std::string（客户端传来的值）
    std::unordered_map<std::string, std::string> data_;
};
