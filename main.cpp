#include "server.h"
#include <iostream>

// ============================================================
// main：程序入口
//
// 职责：
//   1. 创建 Server 对象（绑定端口）
//   2. 调用 server.run() 进入主循环
//
// Server 对象在栈上创建，但它内部的 db_（Database）
// 包含 unordered_map，map 的桶数组在堆上。
//
// 整体对象关系：
//   栈帧 main()
//   └── Server server（栈上）
//       ├── port_      = 6379（int，栈上）
//       ├── server_fd_ = 3（int，栈上，由 socket() 内核分配）
//       └── db_（Database，内嵌在 Server 中）
//           └── data_（unordered_map，桶数组在堆上）
//               ├── "name" → "james"
//               └── "age"  → "25"
// ============================================================
int main() {
    std::cout << "=== Mini Redis（教学版）===\n";

    // 创建服务器，监听 6379 端口
    Server server(6379);

    // 进入主循环（不会返回）
    server.run();

    return 0;
}
