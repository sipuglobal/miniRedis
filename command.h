#pragma once

#include <string>
#include <vector>
#include "database.h"

// ============================================================
// Command：表示一条已解析的客户端命令
//
// 例如客户端发来的原始文本：
//   "SET name james\r\n"
//
// 解析后：
//   name = "SET"
//   args = ["name", "james"]
//
// 内存结构（均在堆上）：
//   Command
//   ├── name: std::string → 堆上的字符数组 "SET\0"
//   └── args: std::vector<std::string>
//             ├── [0]: std::string → "name\0"
//             └── [1]: std::string → "james\0"
// ============================================================
struct Command {
    std::string name;              // 命令名（已转为大写），如 "SET" / "GET"
    std::vector<std::string> args; // 参数列表（命令名之后的所有 token）
};

// ============================================================
// parseCommand：将一行原始文本解析为 Command 结构
//
// 输入示例：
//   "SET name james"
//   "GET name"
//   "KEYS"
//
// 解析步骤：
//   1. 按空格分割字符串（tokenize）
//   2. 第一个 token 作为命令名（转大写）
//   3. 其余 token 作为参数
// ============================================================
Command parseCommand(const std::string& line);

// ============================================================
// executeCommand：执行命令并返回响应字符串
//
// 响应格式（简化的文本协议）：
//   +OK\r\n           → 操作成功
//   $<len>\r\n        → 接下来是字符串值
//   <value>\r\n       → 字符串值本身
//   $-1\r\n           → 键不存在（nil）
//   :<number>\r\n     → 整数结果
//   -ERR <msg>\r\n    → 错误信息
// ============================================================
std::string executeCommand(const Command& cmd, Database& db);
