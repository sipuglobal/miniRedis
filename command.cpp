#include "command.h"

#include <sstream>
#include <algorithm>
#include <cctype>

// ============================================================
// parseCommand：文本 → Command 结构
//
// 解析流程：
//   原始文本: "SET name james"
//        ↓ std::istringstream 分词
//   tokens: ["SET", "name", "james"]
//        ↓ 第一个 token 转大写
//   Command { name="SET", args=["name","james"] }
//
// LLDB 调试建议：
//   在 `return cmd;` 处下断点，查看解析结果：
//   p cmd.name
//   p cmd.args
// ============================================================
Command parseCommand(const std::string& line) {
    Command cmd;

    // std::istringstream 把字符串当作流来读，>> 自动按空格分词
    std::istringstream ss(line);
    std::string token;

    // 读取第一个 token 作为命令名
    if (!(ss >> token)) {
        // 空行，返回空命令
        return cmd;
    }

    // 命令名转大写，这样 "set" 和 "SET" 都能识别
    std::transform(token.begin(), token.end(), token.begin(), ::toupper);
    cmd.name = token;

    // 读取剩余 token 作为参数
    while (ss >> token) {
        cmd.args.push_back(token);
    }

    return cmd;
}

// ============================================================
// executeCommand：根据命令名分发到对应的数据库操作
//
// 所有命令说明：
//
//   SET key value  → 存入键值对，返回 +OK
//   GET key        → 查找键，返回值或 $-1（nil）
//   DEL key        → 删除键，返回删除数量
//   EXISTS key     → 检查键是否存在，返回 0 或 1
//   KEYS           → 返回所有键名
//
// LLDB 调试建议：
//   b command.cpp:executeCommand   // 在函数入口下断点
//   p cmd.name                     // 查看命令名
//   p cmd.args                     // 查看参数
// ============================================================
std::string executeCommand(const Command& cmd, Database& db) {
    // 命令名为空，忽略
    if (cmd.name.empty()) {
        return "";
    }

    // --------------------------------------------------------
    // SET key value
    // 将 key→value 写入内存哈希表
    // --------------------------------------------------------
    if (cmd.name == "SET") {
        if (cmd.args.size() < 2) {
            return "-ERR wrong number of arguments for 'SET'\r\n";
        }
        const std::string& key   = cmd.args[0];
        const std::string& value = cmd.args[1];
        db.set(key, value);
        return "+OK\r\n";
    }

    // --------------------------------------------------------
    // GET key
    // 从内存哈希表查找 key 对应的 value
    // --------------------------------------------------------
    if (cmd.name == "GET") {
        if (cmd.args.size() < 1) {
            return "-ERR wrong number of arguments for 'GET'\r\n";
        }
        const std::string& key = cmd.args[0];
        const std::string* val = db.get(key);

        if (val == nullptr) {
            // key 不存在，返回 nil（$-1 是 Redis 协议中表示 null bulk string 的方式）
            return "$-1\r\n";
        }

        // 返回值的长度和内容
        // 格式：$<字节数>\r\n<值>\r\n
        return "$" + std::to_string(val->size()) + "\r\n" + *val + "\r\n";
    }

    // --------------------------------------------------------
    // DEL key
    // 从内存哈希表删除 key
    // --------------------------------------------------------
    if (cmd.name == "DEL") {
        if (cmd.args.size() < 1) {
            return "-ERR wrong number of arguments for 'DEL'\r\n";
        }
        const std::string& key = cmd.args[0];
        int deleted = db.del(key);
        // 返回整数：:1 表示删除了 1 个，:0 表示 key 不存在
        return ":" + std::to_string(deleted) + "\r\n";
    }

    // --------------------------------------------------------
    // EXISTS key
    // 检查 key 是否在内存哈希表中
    // --------------------------------------------------------
    if (cmd.name == "EXISTS") {
        if (cmd.args.size() < 1) {
            return "-ERR wrong number of arguments for 'EXISTS'\r\n";
        }
        const std::string& key = cmd.args[0];
        bool found = db.exists(key);
        return ":" + std::to_string(found ? 1 : 0) + "\r\n";
    }

    // --------------------------------------------------------
    // KEYS
    // 列出内存哈希表中所有的 key
    // 返回格式：*<count>\r\n$<len>\r\n<key>\r\n ...
    // --------------------------------------------------------
    if (cmd.name == "KEYS") {
        std::vector<std::string> all_keys = db.keys();

        // 构造 Redis Array 格式响应
        std::string response = "*" + std::to_string(all_keys.size()) + "\r\n";
        for (const std::string& k : all_keys) {
            response += "$" + std::to_string(k.size()) + "\r\n";
            response += k + "\r\n";
        }
        return response;
    }

    // --------------------------------------------------------
    // PING（redis-cli 连接时会发送，返回 +PONG）
    // --------------------------------------------------------
    if (cmd.name == "PING") {
        return "+PONG\r\n";
    }

    // --------------------------------------------------------
    // 未知命令
    // --------------------------------------------------------
    return "-ERR unknown command '" + cmd.name + "'\r\n";
}
