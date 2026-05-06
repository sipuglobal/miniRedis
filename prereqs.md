# 前置知识：读懂这个项目需要哪些 API

本文档列出项目中实际用到的所有 API，每个配一个最小可运行示例。
建议读代码之前先扫一遍，遇到不认识的函数再回来查。

---

## 一、POSIX Socket API

### `socket()` — 向内核申请一个套接字

```c
#include <sys/socket.h>

int fd = socket(AF_INET, SOCK_STREAM, 0);
// AF_INET    = IPv4
// SOCK_STREAM = TCP（有连接、可靠）
// 返回值：文件描述符（整数），失败返回 -1
```

fd 本质上是一个整数（比如 3、4、5），内核用它找到对应的 socket 结构体。

---

### `setsockopt()` — 设置套接字选项

```c
int opt = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
// SOL_SOCKET   = 在 socket 层设置
// SO_REUSEADDR = 允许重用处于 TIME_WAIT 的端口
// 不设置这个，服务器重启后会报 "Address already in use"
```

---

### `bind()` — 绑定地址和端口

```c
#include <netinet/in.h>

struct sockaddr_in addr;
memset(&addr, 0, sizeof(addr));
addr.sin_family      = AF_INET;
addr.sin_addr.s_addr = INADDR_ANY;   // 0.0.0.0，监听所有网卡
addr.sin_port        = htons(6379);  // 端口，必须转成网络字节序

bind(fd, (struct sockaddr*)&addr, sizeof(addr));
```

---

### `htons()` / `ntohs()` — 字节序转换

```c
#include <arpa/inet.h>

uint16_t net_port = htons(6379);   // host to network short：主机序→网络序
uint16_t host_port = ntohs(net_port); // network to host short：网络序→主机序
```

x86 是小端（低字节在低地址），网络协议是大端。端口号必须转换，否则对端解析出来的端口是错的。

---

### `listen()` — 开始监听，建立连接队列

```c
listen(fd, 128);
// 128 = 内核排队等待 accept() 处理的连接数上限
// 超过这个数的新连接会被内核直接拒绝
```

`listen()` 之后，内核开始接受 TCP 握手，但程序还没有拿到连接，直到调用 `accept()`。

---

### `accept()` — 从队列里取出一个已完成的连接

```c
struct sockaddr_in client_addr;
socklen_t client_len = sizeof(client_addr);

int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
// server_fd  继续监听新连接（不变）
// client_fd  专门用于和这个客户端通信（新的 fd）
// client_addr 里有客户端的 IP 和端口
```

`accept()` 是阻塞调用：没有新连接时进程睡眠，有连接时内核唤醒。

---

### `recv()` — 从连接中读取字节

```c
char buf[1024];
ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
// 返回值：
//   > 0  实际读到的字节数（可能少于 sizeof(buf)）
//   = 0  对端关闭连接（EOF）
//   < 0  出错
```

TCP 是流协议，一次 `recv()` 不保证读到完整的"一条消息"。需要自己判断边界（比如找 `\n`）。

---

### `send()` — 向连接写入字节

```c
const char* msg = "+OK\r\n";
send(client_fd, msg, strlen(msg), 0);
// 返回值是实际发送的字节数
// 对于小数据，通常一次发完；大数据需要循环发送
```

---

### `close()` — 关闭套接字，释放内核资源

```c
close(client_fd);
// 发送 FIN，通知对端连接关闭
// 不调用 close() 会导致 fd 泄漏（fd 数量是有上限的）
```

---

### `inet_ntoa()` — 把二进制 IP 转成字符串

```c
#include <arpa/inet.h>

// client_addr.sin_addr 是 4 字节的二进制 IP
printf("%s\n", inet_ntoa(client_addr.sin_addr));
// 输出如 "127.0.0.1"
```

---

### `memset()` — 将内存块清零

```c
#include <cstring>

struct sockaddr_in addr;
memset(&addr, 0, sizeof(addr));
// 把 addr 的所有字节设为 0
// 必须清零，否则结构体里的填充字节是随机值
```

---

## 二、STL 标准库

### `std::unordered_map` — 哈希表（核心数据结构）

```cpp
#include <unordered_map>
#include <string>

std::unordered_map<std::string, std::string> m;

// 插入 / 覆盖
m["name"] = "james";

// 查找（不会插入）
auto it = m.find("name");
if (it != m.end()) {
    std::string& val = it->second;  // it->first 是 key
}

// 删除，返回删除数量（0 或 1）
m.erase("name");

// 检查是否存在（不插入）
bool exists = m.count("name") > 0;

// 遍历
for (const auto& pair : m) {
    // pair.first = key, pair.second = value
}

// 元素数量
size_t n = m.size();
```

**注意**：`m["key"]` 在 key 不存在时会自动插入空字符串。查找时用 `find()`，避免意外修改 map。

---

### `std::string` — 字符串

```cpp
#include <string>

std::string s = "hello";

s.size();          // 字节数：5
s.empty();         // 是否为空：false
s += " world";     // 追加
s.back();          // 最后一个字符：'d'
s.pop_back();      // 删除最后一个字符
s.clear();         // 清空
s.c_str();         // 返回 const char*，用于 C 接口（如 send()）
s.substr(1, 3);    // 从下标 1 起取 3 个字符
s[0];              // 下标访问

// 数字转字符串
std::string n = std::to_string(42);   // "42"
```

---

### `std::vector` — 动态数组

```cpp
#include <vector>

std::vector<std::string> v;

v.push_back("a");   // 追加元素
v.size();           // 元素数量
v[0];               // 下标访问
v.empty();          // 是否为空

// 范围 for
for (const std::string& item : v) { }

// 用另一个 vector 的区间初始化
// parts 是已有的 vector，跳过第一个元素
v.assign(parts.begin() + 1, parts.end());
```

---

### `std::istringstream` — 把字符串当流读，用于分词

```cpp
#include <sstream>

std::string line = "SET name james";
std::istringstream ss(line);
std::string token;

while (ss >> token) {
    // 依次得到 "SET", "name", "james"
    // >> 自动按空白符（空格、tab）分割
}
```

这是项目里解析命令的核心工具，比手动找空格位置简洁得多。

---

### `std::transform` + `::toupper` — 字符串转大写

```cpp
#include <algorithm>
#include <cctype>

std::string s = "set";
std::transform(s.begin(), s.end(), s.begin(), ::toupper);
// s 变成 "SET"

// transform 的参数：
//   输入起点，输入终点，输出起点，对每个元素调用的函数
```

用于让 `set` 和 `SET` 都能被识别为同一条命令。

---

### `std::stoi` — 字符串转整数

```cpp
#include <string>

std::string s = "*3";
int n = std::stoi(s.substr(1));  // 取 '*' 后面的部分，得到 3

// stoi 会忽略前导空格，遇到非数字字符停止
// 如果字符串不是数字，抛出 std::invalid_argument 异常
```

项目里用于解析 RESP 协议的 `*N`（参数数量）和 `$N`（字符串长度）。

---

## 三、关键数据类型

| 类型 | 含义 | 典型值 |
|------|------|--------|
| `int` | socket 文件描述符 | 3, 4, 5... |
| `ssize_t` | 有符号的字节数（recv/send 返回值） | -1, 0, 1, 512... |
| `socklen_t` | 地址结构体的长度 | `sizeof(sockaddr_in)` |
| `struct sockaddr_in` | IPv4 地址（IP + 端口） | |
| `INADDR_ANY` | `0.0.0.0`，监听所有网卡 | |

---

## 四、头文件速查

```cpp
#include <sys/socket.h>  // socket, bind, listen, accept, recv, send, setsockopt
#include <netinet/in.h>  // sockaddr_in, INADDR_ANY, htons, ntohs
#include <arpa/inet.h>   // inet_ntoa
#include <unistd.h>      // close
#include <cstring>       // memset

#include <string>        // std::string
#include <vector>        // std::vector
#include <unordered_map> // std::unordered_map
#include <sstream>       // std::istringstream
#include <algorithm>     // std::transform
#include <cctype>        // ::toupper
```
