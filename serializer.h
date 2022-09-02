#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <utility>
#include <vector>
#include <memory>
#include <string>
#include <cstring>
#include <cassert>
#include <algorithm>

// 用于存储数据的类, 继承于vector<char>
class DataBuffer: public std::vector<char>{
    size_t cursorPos{};       // 用于存储当前字节流的位置
public:
    DataBuffer():cursorPos(0){
    }
    DataBuffer(const char* s, size_t len):cursorPos(0){
        insert(begin(), s, s+len);
    }
    typedef std::shared_ptr<DataBuffer> ptr;
    void append(const char* s, size_t len){     // 添加数据到容器中
        insert(end(), s, s+len);
    }
    size_t curSize() {        // 返回当前还剩余的空间
        return size() - cursorPos;
    }
    const char* curData(){      // 返回当前位置指向的数据数据
        return &(*this)[cursorPos];
    }
    void offset(int k){     // 将指针向后移动k个单位
        cursorPos += k;
    }
};

// 判断是否为小端字符序
static bool isLittleEndian(){
    static uint16_t flag = 1;
    static bool little_end_flag = *((uint8_t*)&flag) == 1;      // little end: 0x0100, big end: 0x0001
    return little_end_flag;
}


// 数据序列化类
class Serializer{
    DataBuffer::ptr buffer;     // 智能指针对象
    static void byteOrder(char* s, int len){        // 由于服务端于客户端采用数据存储编码方式可能不同，这里将其同一转换为大端编码
        if(isLittleEndian()){
            std::reverse(s, s+len);
        }
    }
    void input(const char* s, size_t len){
        buffer->append(s, len);
    }

    template <typename T>
    void inputTypes(T t);       // 用于处理各种类型的序列化数据的输入

    template <typename T>
    void outputTypes(T& t);      // 用于处理各种类型的序列化数据的输出
public:
    typedef std::shared_ptr<Serializer> ptr;            // 定义指向Serializer的智能指针
    Serializer(){       // 构造函数，初始化智能指针
        buffer = std::make_shared<DataBuffer>();
    }
    Serializer(const char* s, size_t len){
        buffer = std::make_shared<DataBuffer>();
        input(s, len);
    }
    explicit Serializer(DataBuffer::ptr buffer)
            : buffer(std::move(buffer)){}

    const char* data(){
        return buffer->curData();
    }
    size_t size(){
        return buffer->curSize();
    }

    template <typename Tuple, std::size_t Id>
    void getv(Serializer& ds, Tuple& t)
    {
        ds >> std::get<Id>(t);
    }

    template <typename Tuple, std::size_t... I>
    Tuple get_tuple(std::index_sequence<I...>)
    {
        Tuple t;
        std::initializer_list<int> { (getv<Tuple, I>(*this, t), 0)... };
        return t;
    }

    template <typename T>

    Serializer& operator << (T t){      // 写入序列化数据
        inputTypes(t);
        return *this;
    }

    template <typename T>
    Serializer& operator >> (T& t){      // 读取序列化数据
        outputTypes(t);
        return *this;
    }
};

template <typename T>
void Serializer::inputTypes(T t) {
    int len = sizeof(t);
    char* p = reinterpret_cast<char*>(&t);      // 强制类型转换
    byteOrder(p, len);      // 顺序转换
    input(p, len);
}

// 偏特化
template <>
void Serializer::inputTypes(std::string t) {
    // 先存入字符串长度
    uint16_t len = t.size();
    inputTypes(len);
    byteOrder(const_cast<char*>(t.c_str()), len);       // 强制类型转换， 去除const属性
    // 再存入字符串
    input(t.c_str(), len);
}

template <>
void Serializer::inputTypes(const char* t) {
    inputTypes<std::string>(std::string(t));
}

template <typename T>
void Serializer::outputTypes(T &t) {
    int len = sizeof(t);
    assert(size() >= len);      // 如果请求的空间小于剩余的空间就抛出错误
	memcpy(&t, data(), len);    // 将当前所指向的数据复制到t中
    buffer->offset(len);        // 将当前数据所在指针向后移动k个单位
    byteOrder(reinterpret_cast<char*>(&t), len);        // 进行强制类型转换
}

// 偏特化
template <>
void Serializer::outputTypes(std::string& t){
    // 先取出字符串长度
    uint16_t strLen = 0;
    outputTypes(strLen);
    t = std::string(data(), strLen);

    buffer->offset(strLen);
    byteOrder(const_cast<char*>(t.c_str()), strLen);
}


#endif // SERIALIZER_H
