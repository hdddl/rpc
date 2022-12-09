#include <string>
#include <iostream>
#include "../rpc.h"

int main()
{
    rpc client;
    client.asClient(5555);
    std::string str = client.call<std::string>("Strcat", "even", 24).value();
    std::cout << "str=" << str << std::endl;
    int result = client.call<int>("add", 3, 2).value();
    std::cout << "result=" << result << std::endl;
    return 0;
}
