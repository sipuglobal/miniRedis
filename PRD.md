```text
你现在是一名经验丰富的 C++ 系统程序员。

请帮我从 0 到 1 编写一个“教学型 Mini Redis”项目。

目标不是高性能，也不是工业级 Redis，而是：

1. 用最容易理解的方式实现
2. 强调内存结构、网络交互、业务逻辑
3. 代码适合配合 LLDB 调试学习
4. 尽量避免过度抽象
5. 不要使用复杂设计模式
6. 不要过度封装
7. 优先保证代码直观

技术要求：

- 使用 C++
- 允许使用 STL
  - std::unordered_map
  - std::string
  - std::vector
  - std::list
  - std::set
- 不需要自己实现 hash table
- 不需要自己实现 string
- 当前重点：
  - socket 网络
  - 命令解析
  - 内存中的数据组织
  - Redis 基础业务逻辑
- 不要引入第三方库
- 不要使用 boost
- 不要使用协程
- 不要使用多线程
- 不要使用 epoll
- 先使用阻塞 IO
- 平台默认 macOS / Linux
- 使用 POSIX socket API

项目目标：

实现一个“极简 Redis Server”。

支持 telnet / nc 连接：

例如：

nc 127.0.0.1 6379

然后输入：

SET name james
GET name
DEL name
EXISTS name
KEYS

返回结果。

第一阶段只实现：

1. string 类型
2. 内存存储
3. 文本协议
4. 单线程
5. 阻塞 socket

数据结构建议：

使用：

std::unordered_map<std::string, std::string>

作为数据库。

项目结构要求：

请拆分为多个文件：

main.cpp
server.cpp
server.h
command.cpp
command.h
database.cpp
database.h

要求：

- 每个文件职责清晰
- 不要把所有代码写进 main.cpp
- 使用最简单的类设计
- 不要过度 OOP
- 不要搞复杂继承

代码风格要求：

1. 所有代码添加中文注释
2. 注释重点解释：
   - socket 工作流程
   - accept / recv / send
   - 命令解析
   - 数据存储
   - 内存中的结构关系
3. 适合初学系统编程的人阅读
4. 适合配合 LLDB 调试

非常重要：

请在关键位置解释：

- 数据在堆上的存储关系
- std::unordered_map 的作用
- recv 收到的数据如何解析
- string 如何流转
- client fd 如何管理

请额外输出：

1. 项目目录结构
2. 完整源码
3. Makefile
4. 编译命令
5. 运行方式
6. nc 测试方法
7. LLDB 调试示例

LLDB 示例要求包含：

- 如何下断点
- 如何查看变量
- 如何查看 map 内容
- 如何查看 socket fd
- 如何查看 recv buffer

最终目标：

让我能够：

1. 运行 mini redis
2. 用 nc 连接
3. 执行 SET/GET
4. 用 LLDB 观察整个程序的内存和调用过程

代码风格：

- 教学型
- 直观
- 易读
- 少魔法
- 少模板技巧
- 少宏
- 少隐藏逻辑

不要省略代码。

请直接输出完整项目。
```
