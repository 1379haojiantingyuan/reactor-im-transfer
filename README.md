# Unix 高级编程课程设计：基于 Reactor 模式的高并发文件传输与即时通讯系统

## 1. 项目概述 (Project Overview)

本项目是一个基于 Linux 平台的高性能及即时通讯与文件传输系统。采用 **Reactor 事件驱动模型**，结合 **Epoll IO 多路复用** 和 **线程池** 技术，实现了业务处理与网络 IO 的分离。同时，利用 **Zero-Copy (Sendfile)** 技术实现了 GB 级大文件的高速传输。

### 核心特性
*   **高并发模型**：基于 `Epoll` (LT模式) + 线程池 (`ThreadPool`) 的半同步/半反应堆架构。
*   **即时通讯**：支持多用户在线、群聊广播、私聊 (`/private`)。
*   **高速传输**：利用 Linux 内核 `sendfile` 实现零拷贝文件下载，极低 CPU 占用。
*   **终端界面**：基于 `ncurses` 库开发的可视化终端客户端 (TUI)。
*   **健壮性**：实现心跳检测机制，自动清理僵尸连接。

---

## 2. 目录结构 (Directory Structure)

```
.
├── bin/                 # 编译输出的可执行文件 (server, client)
├── build/               # 编译中间目标文件 (.o)
├── include/             # 头文件 (protocol.h, threadpool.h 等)
├── src/                 # 服务端核心源代码
├── client/              # 客户端源代码
├── file_storage/        # 服务端存放供下载文件的目录
├── tests/               # 单元测试代码
├── Makefile             # 自动化构建脚本
├── 使用说明.md           # 详细功能使用指南
├── 环境配置文档.md       # 环境依赖与安装指南
└── 设计文档.md           # 系统架构与详细设计
```

---

## 3. 环境准备 (Prerequisites)

请确保开发环境满足以下要求：

*   **操作系统**: Linux (推荐 Ubuntu 20.04+)
*   **编译器**: GCC/G++ (支持 C++17 标准)
*   **构建工具**: Make (GNU Make 4.0+)
*   **依赖库**: `libncurses-dev` (用于客户端 UI)

**快速安装依赖 (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install -y build-essential libncurses-dev
```

---

## 4. 编译构建 (Build)

项目提供了一键编译的 `Makefile`。

```bash
# 1. 编译项目 (生成 server 和 client)
make

# 2. 清理编译产物
make clean
```

编译成功后，可执行文件位于 `bin/` 目录下：
*   服务端：`bin/server`
*   客户端：`bin/client`

---

## 5. 快速开始 (Quick Start)

### 5.1 启动服务端
```bash
./bin/server
# 服务端默认监听 8080 端口，启动后将显示 Server Loop 日志
```

### 5.2 启动客户端
客户端启动时必须指定**用户名**。默认连接本地 localhost (127.0.0.1)。

```bash
# 语法: ./bin/client <用户名> [服务器IP]

# 示例1: 连接本地服务器
./bin/client Alice

# 示例2: 连接远程服务器
./bin/client Bob 192.168.1.100
```

---

## 6. 功能使用 (Usage)

进入客户端界面后：

*   **群聊**：直接输入文字并回车。
*   **私聊**：`/private <用户名> <消息>`
    *   示例: `/private Alice 你好`
*   **下载文件**：`/download <文件名>`
    *   示例: `/download test.txt`
    *(注: 文件必须存在于服务端的 `file_storage/` 目录下)*
*   **退出**：`/quit`

---

## 7. 协议设计 (Protocol)

系统采用自定义二进制协议解决 TCP 粘包问题：
*   **Header (12 bytes)**: Include `total_len`, `msg_type`, `crc32`.
*   **Body**: 变长数据体，根据 MsgType 解析 (JSON/Binary)。

---

## 8. License

本项目为课程设计项目，仅供学习交流使用。
