#include <iostream>
#include "../serializer.h"

using namespace std;

// example int type
void testInt(){
    Serializer sr;
    sr << 3;
    int x;
    sr >> x;
    if(x == 3){
        cout << "Int type  pass" << endl;
    }else{
        cout << "Int type not worry" << endl;
    }
}

// example String type
void testString(){
    Serializer sr;
    string st = "明月多情应笑我，笑我如今，幸负春心，独自闲愁独自鸣";
    sr << st;
    string t;
    sr >> t;
    if(st == t){
        cout << "String example pass" << endl;
    }else{
        cout << "String example not pass" << endl;
    }
}

// example self-define type
void testSelfDefine(){
    struct Test{
        int a;
        int b;
    };
    Serializer sr;
    Test test{};
    test.a = 2;
    test.b = 3;
    sr << test;
    Test r{};
    sr >> r;
    if(r.a == test.a && r.b == test.b){
        cout << "Self define data struct pass" << endl;
    }else{
        cout << "Self define data struct not pass" << endl;
    }
}

int main(){
    testInt();
    testString();
    testSelfDefine();

    return 0;
}