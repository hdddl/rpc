#ifndef _RPC_H_
#define _RPC_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <map>
#include <functional>
#include <tuple>
#include <memory>
#include "serializer.h"

template<typename T>
struct type_xx { typedef T type; };

template<>
struct type_xx<void> { typedef int8_t type; };

// 打包帮助模板
template<typename Tuple, std::size_t... Index>
void package_Args_impl(Serializer& sr, const Tuple& t, std::index_sequence<Index...>)
{
    std::initializer_list<int> { (sr << std::get<Index>(t), 0) ... };
}

template<typename... Args>
void package_Args(Serializer& sr, const std::tuple<Args...>& t)
{

    package_Args_impl(sr, t, std::index_sequence_for<Args...>{});
}

// 用tuple做参数调用函数模板类
template<typename Function, typename Tuple, std::size_t... Index>
decltype(auto) invoke_impl(Function&& func, Tuple&& t, std::index_sequence<Index...>)
{
    return func(std::get<Index>(std::forward<Tuple>(t))...);
}

template<typename Function, typename Tuple>
decltype(auto) invoke(Function&& func, Tuple&& t)
{
    constexpr auto size = std::tuple_size<typename std::decay<Tuple>::type>::value;
    return invoke_impl(std::forward<Function>(func), std::forward<Tuple>(t), std::make_index_sequence<size>{});
}

template<typename R, typename F, typename ArgsTuple>
typename std::enable_if<!std::is_same<R, void>::value, typename type_xx<R>::type >::type
call_helper(F f, ArgsTuple args)
{
    return invoke(f, args);
}

enum rpc_role {
    RPC_CLIENT,
    RPC_SERVER
};
enum rpc_errcode {
    RPC_ERR_SUCCESS = 0,
    RPC_ERR_FUNCTION_NOT_REGUIST,
    RPC_ERR_RECV_TIMEOUT
};

template <typename T>
class response_t {
public:
    response_t()
            : code_(RPC_ERR_SUCCESS)
    {
        message_.clear();
    }
    T value() const { return value_; }

    void set_value(const T& val) { value_ = val; }
    void set_code(int code) { code_ = (int)code; }
    void set_message(const std::string& msg) { message_ = msg; }

    friend Serializer& operator>>(Serializer& in, response_t<T>& d)
    {
        in >> d.code_ >> d.message_;
        if (d.code_ == 0)
            in >> d.value_;
        return in;
    }
    friend Serializer& operator<<(Serializer& out, response_t<T>& d)
    {
        out << d.code_ << d.message_ << d.value_;
        return out;
    }

private:
    int code_;
    std::string message_;
    T value_;
};

class rpc {
public:
    size_t asClient(int port){
        cliFd = socket(AF_INET, SOCK_STREAM, 0);

        cliAddr.sin_family = AF_INET;                                  // IPV4
        cliAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);     // use localhost
        cliAddr.sin_port = htons(port);

        if(connect(cliFd, (struct sockaddr *)&cliAddr, sizeof(cliAddr)) != 0){
            std::cout << "connect to server failed" << std::endl;
            return 1;
        }

        return 0;
    }

    size_t asServer(int port){
        listenFD = socket(AF_INET, SOCK_STREAM, 0);

        if(listenFD < 0){
            std::cout << "create listen file descriptor fail" << std::endl;
            return 1;
        }

        serveAddr.sin_family = AF_INET;                                  // IPV4
        serveAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);     // use localhost
        serveAddr.sin_port = htons(port);                      // set listen port


        if(bind(listenFD, (struct sockaddr *)&serveAddr, sizeof(serveAddr))){
            std::cout << "bind failed" << std::endl;
            return 1;
        }
        return 0;
    }

    // 发送消息
    void send(const char* buf, size_t len){
        sendto(cliFd, buf, len, 0, (struct sockaddr *)&cliAddr, sizeof(cliAddr));
    }

    // 接受消息
    ssize_t receive(char* buf, size_t len){
        return recvfrom(cliFd, buf, len, 0, (struct sockaddr*)&cliAddr, &cliAddrLen);
    }

    // 开始运行
    size_t run(){
        // begin to listening
        if(listen(listenFD, 128) < 0){
            std::cout << "Could not listen on socket" << std::endl;
            return 1;
        }

        std::cout << "RPC server is running" << std::endl;
        while(true){
            cliAddrLen = sizeof(cliAddr);
            cliFd = accept(listenFD, (struct sockaddr *)&cliAddr, &cliAddrLen);
            if(cliFd < 0){
                std::cout << "creat client file description failed" << std::endl;
                return 1;
            }

            std::cout << inet_ntoa(cliAddr.sin_addr) << " is connected"  << std::endl;
            char buf[buffSize];
            while(true){
                ssize_t len = receive(buf, buffSize);
                if(!len)break;          // EOF

                DataBuffer::ptr buffer = std::make_shared<DataBuffer>((char *)buf, len);
                Serializer sr(buffer);
                std::string functionName;
                sr >> functionName;

                Serializer::ptr rPtr = call_(functionName, sr.data(), sr.size());
                send(rPtr->data(), rPtr->size());

            }
            std::cout << std::endl;
            std::cout << inet_ntoa(cliAddr.sin_addr) << " is disconnected"  << std::endl;
        }
    }

    Serializer::ptr call_(const std::string& name, const char* data, int len){          // 服务端调用
        Serializer::ptr sp = std::make_shared<Serializer>();
        if (mapFunctions_.find(name) == mapFunctions_.end()){       // 函数未注册
            std::cout << "function not regist, name=" << name << std::endl;
            *sp.get() << static_cast<int>(RPC_ERR_FUNCTION_NOT_REGUIST);
            *sp.get() << std::string("function not bind: " + name);
            return sp;
        }
        auto func = mapFunctions_[name];
        func(sp.get(), data, len);
        return sp;
    }

    template <typename R, typename... Args>
    response_t<R> call(const std::string& name, Args... args)       // 客户端调用函数
    {
        using args_type = std::tuple<typename std::decay<Args>::type ...>;
        args_type as = std::make_tuple(args...);
        Serializer sr;
        sr << name;
        package_Args(sr, as);
        return net_call<R>(sr);
    }

    template <typename F>
    void regist(const std::string& name, F func){           // 服务端：函数注册
        mapFunctions_[name] = std::bind(&rpc::callproxy<F>, this, func, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    template<typename F, typename S>
    void regist(const std::string& name, F func, S* s)
    {
        mapFunctions_[name] = std::bind(&rpc::callproxy<F, S>, this, func, s, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }

    template<typename F>
    void callproxy(F func, Serializer* pr, const char* data, int len )
    {
        callproxy_(func, pr, data, len);
    }
    template<typename F, typename S>
    void callproxy(F func, S * s, Serializer * pr, const char * data, int len)
    {
        callproxy_(func, s, pr, data, len);
    }

    // 函数指针
    template<typename R, typename... Args>
    void callproxy_(R(*func)(Args...), Serializer* pr, const char* data, int len) {
        callproxy_(std::function<R(Args...)>(func), pr, data, len);
    }

    template<typename R, typename C, typename S, typename... Args>
    void callproxy_(R(C::* func)(Args...), S* s, Serializer* pr, const char* data, int len) {

        using args_type = std::tuple<typename std::decay<Args>::type...>;

        Serializer sr(std::make_shared<DataBuffer>(data, len));
        args_type as = sr.get_tuple<args_type>(std::index_sequence_for<Args...>{});

        auto ff = [=](Args... args)->R {
            return (s->*func)(args...);
        };
        typename type_xx<R>::type r = call_helper<R>(ff, as);

        response_t<R> response;
        response.set_code(RPC_ERR_SUCCESS);
        response.set_message("success");
        response.set_value(r);
        (*pr) << response;
    }

    template<typename R, typename... Args>
    void callproxy_(std::function<R(Args... args)> func, Serializer* pr, const char* data, int len)
    {
        using args_type = std::tuple<typename std::decay<Args>::type...>;

        Serializer sr(std::make_shared<DataBuffer>(data, len));
        args_type as = sr.get_tuple<args_type> (std::index_sequence_for<Args...>{});

        typename type_xx<R>::type r = call_helper<R>(func, as);

        response_t<R> response;
        response.set_code(RPC_ERR_SUCCESS);
        response.set_message("success");
        response.set_value(r);
        (*pr) << response;
    }


    template<typename R>
    response_t<R> net_call(Serializer& sr)          // 发送已经完成序列化的数据
    {
        send(sr.data(), sr.size());
        char buf[buffSize];
        auto len = receive(buf, buffSize);

        response_t<R> val;
        if(!len){
            val.set_code(RPC_ERR_RECV_TIMEOUT);
            val.set_message("recv timeout");
            return val;
        }
        Serializer res(buf, len);
        res >> val;
        return val;
    }

private:
    std::map<std::string, std::function<void(Serializer*, const char*, int)>> mapFunctions_;
    int listenFD, cliFd;
    int buffSize = 200;
    struct sockaddr_in serveAddr, cliAddr;
    socklen_t cliAddrLen = 0;

};

#endif