#include "database.h"

// ============================================================
// set：向哈希表写入键值对
//
// 内部流程：
//   data_[key] = value;
//
// 如果 key 已存在，operator[] 会直接覆盖对应的 string 值。
// 如果 key 不存在，operator[] 会先插入一个空 string，再赋值。
//
// 堆上的变化：
//   - std::string key   的字符数组分配在堆上
//   - std::string value 的字符数组分配在堆上
//   - unordered_map 的桶数组也在堆上
// ============================================================
void Database::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}

// ============================================================
// get：查找键对应的值
//
// 使用 find() 而不是 operator[]，原因：
//   - operator[] 在 key 不存在时会自动插入空值（副作用）
//   - find() 只查找，不修改 map
//
// 返回 iterator，如果等于 end() 表示没找到，否则返回指向 value 的指针。
// ============================================================
const std::string* Database::get(const std::string& key) {
    // it 的类型是 unordered_map<string, string>::iterator
    auto it = data_.find(key);
    if (it == data_.end()) {
        // key 不存在，返回空指针
        return nullptr;
    }
    // it->second 就是 value，取其地址返回
    return &it->second;
}

// ============================================================
// del：删除键值对
//
// erase() 返回删除的元素数量（0 表示 key 不存在，1 表示删除成功）
// ============================================================
int Database::del(const std::string& key) {
    return static_cast<int>(data_.erase(key));
}

// ============================================================
// exists：检查键是否存在
//
// count() 对于 unordered_map 只会返回 0 或 1（因为 key 唯一）
// ============================================================
bool Database::exists(const std::string& key) {
    return data_.count(key) > 0;
}

// ============================================================
// keys：返回所有键的列表
//
// 遍历 map，把每个 pair 的 first（key）收集到 vector 里。
// 注意：unordered_map 不保证顺序，所以返回顺序不固定。
// ============================================================
std::vector<std::string> Database::keys() {
    std::vector<std::string> result;
    // pair.first = key, pair.second = value
    for (const auto& pair : data_) {
        result.push_back(pair.first);
    }
    return result;
}
