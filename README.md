通过 C++ 17 编写的一个轻量级的 RPC 框架，不依赖第三方库，引入头文件就可以直接使用啦

# 1. 编译

因为网络传输部分基于 POSIX  API 的 Socket 编写，所以目前只支持 Linux / Unix 系统编译，在 Windows 上面可以通过 WSL 编译运行。编译工具采用的是 `cmake` ，可以直接使用 bash 脚本直接编译 `demo`

```shell
bash build.sh
```

## 2. 快速入门

可以看 `example` 文件夹中的 demo 了解基本的使用方法

