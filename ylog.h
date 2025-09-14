
/*c++ 17*/
/*
异步日志 
Windows,Linux 跨平台
多线程执行,保证线程安全
线程安全的缓冲区阻塞队列
[时间][等级][内容]
控制台根据等级用不同的颜色打印
*/
#pragma once

#include<iostream>
#include<string>
#include<queue>
#include<condition_variable>
#include<thread>
#include<atomic>
#include<fstream>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

namespace utility{

class Ylog{
public:
    enum LEVEL{
        DEBUG,
        INFO,
        WARNING,
        ERR,
        FATAL
    };
private:
    enum Color{
        BLACK = 0,
        RED = 1,      // 红色
        GREEN = 2,    // 绿色
        YELLOW = 3,   // 黄色
        BLUE = 4,     // 蓝色
        MAGENTA = 5,  // 品红
        CYAN = 6,     // 青色
        WHITE = 7,    // 白色
        DEFAULT = 8   // 
    };

public:
    static Ylog*get_instance(){
        // c++ 11 保证线程安全
        static Ylog instance;
        return &instance;
    } 

    // 不需要后缀名,已经默认为.log
    void init(const char*file_name,int max_cnt=10000,int buf_threshold=10);

    // 写入缓冲区和控制台
    void log(LEVEL level,const char* context);

private:
    Ylog();
    ~Ylog();
    string filename;                          // log文件路径,已经默认后缀名.log
    int threshold;                            // 缓冲区阈值
    int max_cnt;                              /* 一个日志文件最多记录多少条日志
                                              ,超过则自动创建新日志*/
    std::atomic<int> line_cnt;                // 现在有日志多少行
    std::atomic<int> log_num;                 // 现在的日志文件序号

    ofstream *m_fp;                           // 打开log的文件指针
    std::atomic<bool> write_action;           // 是否要写入文件
    std::queue<std::string> log_buffer;       // 日志缓冲区
    std::mutex buffer_mutex;                  // 保护缓冲区的互斥量
    std::condition_variable buffer_cond;      // 缓冲区条件变量
    std::thread writer_thread;                // 写入线程
    std::atomic<bool> running;                // 控制线程运行的标志

    void stop();                              // 停止log线程

    void write_loop();                        // 写入文件的线程
    static void setColor(Color foreground,
            Color background = DEFAULT);      // 设置文本颜色
    
    static void print(string&context,
            Color color);                     // 打印到控制台

private:
    #ifdef _WIN32
        static constexpr const char* NEWLINE = "\n";
    #else
        static constexpr const char* NEWLINE = "\n";
    #endif

};
}// end namespace
