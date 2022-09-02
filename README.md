用C++写的一个简单RPC框架

# 1. 编译

本项目基于[zmq](https://github.com/zeromq), 通过`cmake`构建, 可以直接使用以下命令继续编译

```shell
git clone https://github.com/hdddl/rpc.git
cd rpc
mkdir build
cmake ..
make
```



# 2. 序列化与反序列化

数据序列化就是将原始的结构化数据结构保存为二进制数据，反序列化则是将二进制数据重新编程结构化数据，这里实现一个简单的数据序列化与反序列的类

1. 如何存储序列化的数据？

   在进行具体的数据序列化操作之前首先需要考虑的是序列化之后的数据应该如何存储，这里采用STL中的`vector<char>`来存储结构化的数据。

2. 如何添加序列化的数据

   这里采用C++中`<<`于`>>`操作符来添加和取出序列化数据，所以需要对着两个操作进行重写

# 3. RPC

1. 远程调用的流程是什么？

   客户端首先需要将函数名，参数全部序列化，并向序列化数据通过网络传输给服务端

   服务端将序列化数据进行反序列化，并执行对应的函数

2. 如何将函数参数全部打包？

   ```cpp
   // 打包帮助模板
   template<typename Tuple, std::size_t... Index>
   void package_Args_impl(Serializer& sr, const Tuple& t, std::index_sequence<Index...>)
   {
       std::initializer_list<int> { (sr << std::get<Index>(t), 0)... };
   }
   
   template<typename... Args>
   void package_Args(Serializer& sr, const std::tuple<Args...>& t)
   {
       package_Args_impl(sr, t, std::index_sequence_for<Args...>{});
   }
   
   template <typename R, typename... Args>
   response_t<R> call(const string& name, Args... args){
       using args_type = std::tuple<typename std::decay<Args>::type...>;
       args_type as = std::make_tuple(args...);
       Serializer sr;
       sr << name;     // 将函数名序列化
       package_Args(sr, as);
   	return net_call<R>(sr);
   }
   ```

   这部分就是当客户端使用`call`函数调用远程函数对应的核心代码.

