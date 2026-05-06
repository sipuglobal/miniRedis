#pragma once

#include "database.h"

// ============================================================
// Server：负责网络监听、接受连接、读写数据
//
// 网络模型（单线程阻塞 IO）：
//
//   main() 创建 Server(6379)
//       ↓
//   Server::run()
//       ↓
//   socket() → 创建 server_fd_（监听套接字）
//       ↓
//   bind()   → 绑定 0.0.0.0:6379
//       ↓
//   listen() → 开始监听，内核维护连接队列
//       ↓
//   循环 accept() → 阻塞等待客户端连接
//       ↓（有客户端连接时）
//   返回 client_fd（每个客户端一个新 fd）
//       ↓
//   handleClient(client_fd) → 循环 recv/send
//       ↓（客户端断开时）
//   close(client_fd) → 关闭连接，回到 accept()
//
// LLDB 查看：
//   p server_fd_    // 查看监听套接字 fd
// ============================================================
class Server {
public:
    // port：监听端口（通常是 6379）
    Server(int port);

    // 启动服务器主循环（阻塞，不返回）
    void run();

private:
    int port_;       // 监听端口号
    int server_fd_;  // 监听套接字的文件描述符（由 socket() 返回）
    Database db_;    // 数据库实例（在 Server 对象内部，生命周期与 Server 相同）

    // 创建、绑定、监听 socket
    void setupSocket();

    // 处理单个客户端连接（阻塞，直到客户端断开）
    // client_fd：accept() 返回的已连接套接字
    void handleClient(int client_fd);
};
