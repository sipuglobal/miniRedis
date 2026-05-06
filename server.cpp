#include "server.h"
#include "command.h"

#include <iostream>
#include <string>
#include <cstring>      // memset
#include <algorithm>    // std::transform
#include <cctype>       // ::toupper

// POSIX socket 相关头文件
#include <sys/socket.h> // socket(), bind(), listen(), accept(), recv(), send()
#include <netinet/in.h> // sockaddr_in, INADDR_ANY, htons()
#include <arpa/inet.h>  // inet_ntoa()（用于打印客户端 IP）
#include <unistd.h>     // close()

// ============================================================
// Server 构造函数
//
// 初始化端口和 fd，server_fd_ 先设为 -1（无效值），
// 后续 setupSocket() 会赋值。
// ============================================================
Server::Server(int port) : port_(port), server_fd_(-1) {
    // db_ 由 Database 默认构造函数初始化（空的 unordered_map）
}

// ============================================================
// setupSocket：创建并配置监听套接字
//
// 流程：
//   socket()  → 向内核申请一个 socket 文件描述符
//   setsockopt() → 设置 SO_REUSEADDR，避免重启时 "Address already in use"
//   bind()    → 把 socket 绑定到 0.0.0.0:port
//   listen()  → 告诉内核开始接受连接，设置等待队列长度
//
// 文件描述符（fd）本质上是一个整数（如 3、4、5），
// 内核通过它找到对应的 socket 结构体。
// ============================================================
void Server::setupSocket() {
    // AF_INET  = IPv4
    // SOCK_STREAM = TCP（面向连接，可靠传输）
    // 0 = 自动选择协议（对 SOCK_STREAM 默认就是 TCP）
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "socket() 失败\n";
        exit(1);
    }

    // 设置 SO_REUSEADDR：服务器重启时可以立即重用端口
    // 否则 TIME_WAIT 状态的连接会占用端口约 2 分钟
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // sockaddr_in：IPv4 地址结构体
    // 内存布局：sin_family(2字节) + sin_port(2字节) + sin_addr(4字节) + 填充
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;         // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;      // 0.0.0.0，接受所有网卡的连接
    addr.sin_port        = htons(port_);    // htons：主机字节序 → 网络字节序（大端）

    // bind：将 socket 与地址绑定
    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind() 失败，端口 " << port_ << " 可能已被占用\n";
        exit(1);
    }

    // listen：开始监听，128 是内核等待队列的最大长度
    if (listen(server_fd_, 128) < 0) {
        std::cerr << "listen() 失败\n";
        exit(1);
    }

    std::cout << "Mini Redis 启动，监听端口 " << port_ << "\n";
    std::cout << "使用 `nc 127.0.0.1 " << port_ << "` 连接\n";
}

// ============================================================
// run：主循环，不断接受新的客户端连接
//
// accept() 是阻塞调用：
//   - 没有新连接时，进程挂起（睡眠），不占用 CPU
//   - 有新连接时，内核唤醒进程，返回新的 client_fd
//
// 由于是单线程，同一时刻只能服务一个客户端。
// 当前客户端断开后，才能 accept 下一个。
//
// LLDB 调试建议：
//   b server.cpp:run    // 在 accept() 处下断点
//   p client_fd         // 查看新连接的 fd
//   p client_addr       // 查看客户端地址信息
// ============================================================
void Server::run() {
    setupSocket();

    while (true) {
        // client_addr 用于接收客户端的 IP 和端口
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // accept()：从内核的完成连接队列取出一个连接
        // 返回值 client_fd 是专门用于与这个客户端通信的新 fd
        // server_fd_ 继续监听新连接，client_fd 用于读写数据
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "accept() 失败，继续等待...\n";
            continue;
        }

        // inet_ntoa 把二进制 IP 地址转成 "x.x.x.x" 字符串
        std::cout << "新连接: " << inet_ntoa(client_addr.sin_addr)
                  << ":" << ntohs(client_addr.sin_port)
                  << "  (fd=" << client_fd << ")\n";

        // 处理这个客户端（阻塞，直到客户端断开）
        handleClient(client_fd);

        // 客户端断开后关闭 fd，释放内核资源
        close(client_fd);
        std::cout << "连接关闭 (fd=" << client_fd << ")\n";
    }
}

// ============================================================
// readLine：从 fd 逐字节读取，直到遇到 '\n'，返回去掉 \r\n 的行内容
//
// 返回 false 表示连接已关闭或出错。
// ============================================================
static bool readLine(int fd, std::string& line) {
    line.clear();
    char ch;
    while (true) {
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0) return false;   // 连接关闭或出错
        if (ch == '\n') {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return true;
        }
        line += ch;
    }
}

// ============================================================
// readExact：从 fd 精确读取 count 个字节，存入 out
//
// RESP 协议中 $<len> 后面紧跟 len 个字节的数据，
// 必须精确读取，不能多也不能少。
// ============================================================
static bool readExact(int fd, int count, std::string& out) {
    out.resize(count);
    int received = 0;
    while (received < count) {
        ssize_t n = recv(fd, &out[received], count - received, 0);
        if (n <= 0) return false;
        received += static_cast<int>(n);
    }
    return true;
}

// ============================================================
// handleClient：与单个客户端进行命令交互
//
// 支持两种输入格式，自动识别：
//
// 1. 纯文本（nc / telnet）：
//      "SET name james\r\n"
//      → 直接按空格分词解析
//
// 2. RESP 多行格式（redis-cli）：
//      "*3\r\n$3\r\nSET\r\n$4\r\nname\r\n$5\r\njames\r\n"
//      → 先读 *N 得到参数数量
//      → 再循环读 $len + 数据
//
// 判断依据：第一行首字符是否为 '*'
//
// LLDB 调试建议：
//   b server.cpp:handleClient
//   p client_fd
//   p first_line        // 观察收到的第一行，判断走哪个分支
// ============================================================
void Server::handleClient(int client_fd) {
    while (true) {
        // 读取第一行，用来判断协议类型
        std::string first_line;
        if (!readLine(client_fd, first_line)) break;  // 连接断开
        if (first_line.empty()) continue;             // 空行跳过

        Command cmd;

        if (first_line[0] == '*') {
            // ------------------------------------------------
            // RESP 多行格式（redis-cli 使用）
            //
            // 格式：
            //   *3\r\n          ← 3 个参数
            //   $3\r\n          ← 第 1 个参数长度为 3
            //   SET\r\n         ← 第 1 个参数内容
            //   $4\r\n          ← 第 2 个参数长度为 4
            //   name\r\n        ← 第 2 个参数内容
            //   $5\r\n          ← 第 3 个参数长度为 5
            //   james\r\n       ← 第 3 个参数内容
            // ------------------------------------------------
            int argc = std::stoi(first_line.substr(1));  // 取 '*' 后面的数字

            std::vector<std::string> parts;
            for (int i = 0; i < argc; i++) {
                std::string len_line;
                if (!readLine(client_fd, len_line)) goto done;  // 读 $len 行
                if (len_line.empty() || len_line[0] != '$') goto done;

                int len = std::stoi(len_line.substr(1));  // 取 '$' 后的长度

                std::string data;
                if (!readExact(client_fd, len, data)) goto done;  // 读 len 个字节

                std::string trailing;
                if (!readLine(client_fd, trailing)) goto done;    // 吃掉 \r\n

                parts.push_back(data);
            }

            if (!parts.empty()) {
                // 第一个 token 是命令名，转大写
                std::transform(parts[0].begin(), parts[0].end(), parts[0].begin(), ::toupper);
                cmd.name = parts[0];
                cmd.args.assign(parts.begin() + 1, parts.end());
            }

        } else {
            // ------------------------------------------------
            // 纯文本格式（nc / telnet 使用）
            // 直接把这一行作为命令解析
            // ------------------------------------------------
            cmd = parseCommand(first_line);
        }

        {
            // 执行命令，发送响应
            std::string response = executeCommand(cmd, db_);
            if (!response.empty()) {
                send(client_fd, response.c_str(), response.size(), 0);
            }
        }
    }

done:
    ; // 连接关闭，退出
}
