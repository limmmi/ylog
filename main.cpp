/*
异步日志
c++ 17
Windows, Linux
*/


#include<iostream>
#include<thread>
#include <chrono>

#include "Ylog.h"

using namespace utility;

int random_num(int a,int b){
    return (rand() % (b-a+1))+ a;
}

void f1(){
    //[0-4]
    while(1){
        int level = random_num(0,4);
        auto* inst=Ylog::get_instance();
        inst->log((Ylog::LEVEL)level,"t1 : This is log test!!");
        std::this_thread::sleep_for(std::chrono::milliseconds(random_num(100,1000)));
    }
}

void f2(){
    while(1){
        int a=0,b=4;
        int level = (rand() % (b-a+1))+ a;
        auto* inst=Ylog::get_instance();
        inst->log((Ylog::LEVEL)level,"t2 : This is log test!!");
        std::this_thread::sleep_for(std::chrono::milliseconds(random_num(100,1000)));
    }
}

int main(){
    /*
    windows 多线程如果有阻塞可能无法响应 ctrl+c 信号
    */
    #ifdef _WIN32
        SetConsoleCtrlHandler([](DWORD signal){
            if (signal == CTRL_C_EVENT) {
                exit(0);
                return TRUE;
            }
            return FALSE;
        }, TRUE);
    #endif

    Ylog::get_instance()->init("0",100);
    thread t1(f1),t2(f2);
    t1.detach();
    t2.detach();

    while(1){
        // main thread
    }
    return 0;
}