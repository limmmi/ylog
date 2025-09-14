#include"Ylog.h"

#include<thread>
#include<fstream>
#include<string> 
#include<iostream>
#include<queue>
#include<condition_variable>
#include<atomic>
#include <ctime>
#include<chrono>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace utility;

void Ylog::init(const char*file_name,int max_cnt,int buf_threshold)
{
    if(running){
        return;
    }
    running=true;
    write_action=false;
    this->filename=file_name;
    this->filename+=".log";
    this->threshold=buf_threshold;
    this->max_cnt=max_cnt;
    this->line_cnt=0;
    this->log_num=0;

    // 不关闭文件流
    m_fp = new std::ofstream();
    m_fp->open(this->filename,ios::app);
    if (!m_fp->is_open()) {
        this->log(Ylog::ERR,"CAN NOT OPEN LOG FILE!");
    }
    writer_thread = thread(&Ylog::write_loop,this);
}

void Ylog::setColor(Color foreground,Color background)
{
    #ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        // 映射前景色
        WORD wForeground = 0;
        switch (foreground) {
            case BLACK:   wForeground = 0; break;
            case RED:     wForeground = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
            case GREEN:   wForeground = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case YELLOW:  wForeground = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case BLUE:    wForeground = FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            case MAGENTA: wForeground = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            case CYAN:    wForeground = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            case WHITE:   wForeground = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            case DEFAULT: 
            default:      wForeground = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // 灰色
        }
        // 映射背景色
        WORD wBackground = 0;
        switch (background) {
            case BLACK:   wBackground = 0; break;
            case RED:     wBackground = BACKGROUND_RED; break;
            case GREEN:   wBackground = BACKGROUND_GREEN; break;
            case YELLOW:  wBackground = BACKGROUND_RED | BACKGROUND_GREEN; break;
            case BLUE:    wBackground = BACKGROUND_BLUE; break;
            case MAGENTA: wBackground = BACKGROUND_RED | BACKGROUND_BLUE; break;
            case CYAN:    wBackground = BACKGROUND_GREEN | BACKGROUND_BLUE; break;
            case WHITE:   wBackground = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE; break;
            case DEFAULT: 
            default:      wBackground = 0; // 黑色背景
        }
        SetConsoleTextAttribute(hConsole, wForeground | wBackground);
    #else
        // ANSI转义序列
        std::cout << "\033[";
        // 处理前景色
        if (foreground != DEFAULT) {
            std::cout << "3" << static_cast<int>(foreground);
        } else {
            std::cout << "39"; // 默认前景色
        }
        
        // 处理背景色
        if (background != DEFAULT) {
            std::cout << ";4" << static_cast<int>(background);
        } else {
            std::cout << ";49"; // 默认背景色
        }
        std::cout << "m";
    #endif
}

void Ylog::print(string&context,Color color)
{
    setColor(color);
    std::cout<<context;
    setColor(Color::DEFAULT);
}

//[时间][等级][内容]
void Ylog::log(LEVEL level,const char* context)
{
    if(running==false){
        return;
    }
    time_t now = time(NULL);
    struct tm local_tm;

    // 跨平台时间转换
    #if defined(_WIN32)
        localtime_s(&local_tm, &now); // Windows安全版本
    #else
        localtime_r(&now, &local_tm); // POSIX安全版本
    #endif

    char time_buffer[80];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &local_tm);
    string log;
    std::lock_guard<std::mutex> locker(buffer_mutex); // 保护控制台,避免多线程同时输出
    switch (level)
    {
    case DEBUG:
        log = "["+string(time_buffer)+"][DEBUG]["+context + "]"+Ylog::get_instance()->NEWLINE;
        print(log,Ylog::Color::BLUE);
        break;
    case INFO:
        log = "["+string(time_buffer)+"][INFO ]["+context + "]"+Ylog::get_instance()->NEWLINE;
        print(log,Ylog::Color::GREEN);
        break;
    case WARNING:
        log = "["+string(time_buffer)+"][WARN ]["+context + "]"+Ylog::get_instance()->NEWLINE;
        print(log,Ylog::Color::YELLOW);
        break;
    case ERR:
        log = "["+string(time_buffer)+"][ERROR]["+context + "]"+Ylog::get_instance()->NEWLINE;
        print(log,Ylog::Color::RED);
        break;
    case FATAL:
        log = "["+string(time_buffer)+"][FATAL]["+context + "]"+Ylog::get_instance()->NEWLINE;
        print(log,Ylog::Color::MAGENTA);
        break;
    default:
        break;
    }
    log_buffer.push(std::move(log));
    // 如果大于阈值就通知写入线程把队列的日志写入文件
    if(log_buffer.size()>=threshold){
        write_action=true;
        buffer_cond.notify_one(); // 只有一个消费者线程
    }

    line_cnt++;
    if(line_cnt>=max_cnt){
        m_fp->flush();
        if (m_fp) {
            m_fp->close();
        }
        log_num++;
        m_fp->open(to_string(log_num)+"_"+filename,ios::app);
        if (!m_fp->is_open()) {
            log = "["+string(time_buffer)+"][ERROR]["+"CAN NOT OPEN LOG FILE" + "]"+Ylog::get_instance()->NEWLINE;
            print(log,Ylog::Color::RED);
        }
        line_cnt=0;
    }
}

// 写入线程函数
void Ylog::write_loop(){  
    while(running&&m_fp&&m_fp->is_open()){
        std::queue<std::string>temp_queue;    
        {
            std::unique_lock<std::mutex>buf_lck(buffer_mutex);
            // buffer_cond.wait(buf_lck,
            //     [this](){
            //     return write_action || !running;
            // });
            buffer_cond.wait_for(buf_lck,100ms,
                [this](){
                return write_action || !running;
            });
            std::swap(temp_queue, log_buffer);
            write_action=false;
        }
        // 批量写入
        std::string combined;
        while(!temp_queue.empty()){
            combined+=temp_queue.front();
            temp_queue.pop();
        }
        *m_fp <<combined;
        m_fp->flush();
    }
}

Ylog::Ylog(){

}

void Ylog::stop(){
    {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        running = false;
        buffer_cond.notify_one();
    }
    if (writer_thread.joinable()) {
        writer_thread.join();
    }
    if (m_fp) {
        m_fp->close();
    }
}

Ylog::~Ylog() {
    stop();
}
