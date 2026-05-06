# LLDB 调试指南

## 启动调试

```bash
lldb ./mini_redis
```

---

## 常用断点

### 1. 在 accept() 处断住，观察新连接

```
(lldb) b server.cpp:55        # accept() 返回后
(lldb) run
```

客户端连接后触发断点，查看 fd：

```
(lldb) p client_fd            # 查看客户端 fd（通常是 4、5...）
(lldb) p client_addr          # 查看客户端 IP 结构体
(lldb) p ntohs(client_addr.sin_port)   # 客户端端口
```

---

### 2. 在 recv() 处断住，观察收到的字节

```
(lldb) b server.cpp:handleClient
(lldb) run
```

连接后用 nc 输入命令：

```
(lldb) p ch                   # 查看刚收到的单个字节
(lldb) p pending              # 查看已累积的行内容
```

---

### 3. 在命令解析处断住

```
(lldb) b command.cpp:parseCommand
(lldb) run
```

触发后：

```
(lldb) p line                 # 查看原始命令行，如 "SET name james"
(lldb) finish                 # 执行完 parseCommand，查看返回值
(lldb) p cmd.name             # 查看命令名
(lldb) p cmd.args             # 查看参数列表
(lldb) p cmd.args[0]          # 查看第一个参数
```

---

### 4. 在 executeCommand 处断住，观察分发逻辑

```
(lldb) b command.cpp:executeCommand
```

```
(lldb) p cmd.name             # "SET" / "GET" / ...
(lldb) p cmd.args             # 参数
(lldb) p &db                  # 查看 Database 地址
```

---

### 5. 查看 unordered_map（数据库内容）

```
(lldb) b database.cpp:set
```

SET 命令执行后：

```
(lldb) p data_                 # 打印整个 map（条目多时输出较长）
(lldb) p data_.size()          # 查看条目数量
(lldb) expr data_["name"]      # 查看指定 key 的 value
```

> 注：lldb 对 unordered_map 的打印支持依赖 formatter，
> 若打印不完整可用：
> ```
> (lldb) script print(lldb.frame.FindVariable("data_"))
> ```

---

### 6. 查看 send 发出的响应内容

```
(lldb) b server.cpp:send      # 在 send 调用处断
```

```
(lldb) p response             # 查看即将发送的字符串
(lldb) p response.c_str()     # 查看底层 char* 指针
(lldb) p response.size()      # 查看字节数
```

---

### 7. 查看内存中 string 的堆结构

std::string 在堆上存储字符数组，可以这样查看：

```
(lldb) p val                          # val 是 const std::string*
(lldb) p *val                         # 解引用查看 string 内容
(lldb) p val->c_str()                 # 底层 char* 地址
(lldb) memory read val->c_str()       # 查看堆上的原始字节
```

---

## 完整调试会话示例

```bash
$ lldb ./mini_redis
(lldb) b server.cpp:handleClient
(lldb) b command.cpp:parseCommand
(lldb) b database.cpp:set
(lldb) run
```

另开终端：

```bash
$ nc 127.0.0.1 6379
SET name james
```

LLDB 依次在三个断点停下：

1. `handleClient` → 观察 client_fd
2. `parseCommand` → 观察 line = "SET name james"
3. `database.cpp:set` → 观察 key="name", value="james"，以及 data_ 的变化

---

## 快捷键

| 命令        | 含义                   |
|-------------|------------------------|
| `n`         | 单步（不进入函数）      |
| `s`         | 单步（进入函数）        |
| `finish`    | 执行完当前函数并返回   |
| `c`         | 继续运行               |
| `bt`        | 查看调用栈             |
| `frame var` | 查看当前帧所有局部变量 |
| `q`         | 退出 lldb              |
