/*
    Ylog 测试程序,异步多线程测试,确保线程安全
    跨平台测试(Linux,Windows)
*/

#include<iostream>
#include<thread>
#include <chrono>

#include "ylog.h"

using namespace ylog;

int random_num(int a,int b){
    return (rand() % (b-a+1))+ a;
}

void f1(){
    //[0-4]
    while(1){
        int level = random_num(0,4);
        auto* inst=Ylog::get_instance();
        inst->log((Ylog::LEVEL)level,"This is log test!!");
        std::this_thread::sleep_for(std::chrono::milliseconds(random_num(100,1000)));
    }
}

void f2(){
    while(1){
        int a=0,b=4;
        int level = (rand() % (b-a+1))+ a;
        auto* inst=Ylog::get_instance();
        inst->log((Ylog::LEVEL)level,"This is log test!!");
        std::this_thread::sleep_for(std::chrono::milliseconds(random_num(100,1000)));
    }
}

void f3(){
    while(1){
        int a=0,b=4;
        int level = (rand() % (b-a+1))+ a;
        auto* inst=Ylog::get_instance();
        inst->log((Ylog::LEVEL)level,"This is log test!!");
        std::this_thread::sleep_for(std::chrono::milliseconds(random_num(100,1000)));
    }
}

int main(){
    Ylog::get_instance()->init("0",100);

    thread t1(f1);
    t1.detach();
    thread t2(f2);
    t2.detach();
    thread t3(f3);
    t3.detach();
    while(1){
        // main thread;
    }
    return 0;
}