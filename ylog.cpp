#include"ylog.h"

#include<thread>
#include<fstream>
#include<string> 
#include<iostream>
#include<queue>
#include<condition_variable>
#include<atomic>
#include <ctime>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace ylog;

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
        this->log(Ylog::ERROR,"CAN NOT OPEN LOG FILE!");
    }
    writer_thread = thread(&Ylog::write_loop);
}

void Ylog::setColor(Color foreground,Color background)
{
    #ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD wColor = (static_cast<WORD>(background) << 4) | static_cast<WORD>(foreground);
        SetConsoleTextAttribute(hConsole, wColor);
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
}

//[时间][等级][内容]
void Ylog::log(LEVEL level,const char* context)
{
    if(running==false){
        return;
    }
    time_t now = time(NULL);
    struct tm local_tm; // 存储时间的独立副本

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
        log = "["+string(time_buffer)+"][DEBUG]["+context + "]"+"\r\n";
        print(log,Ylog::Color::BLUE);
        break;
    case INFO:
        log = "["+string(time_buffer)+"][INFO ]["+context + "]"+"\r\n";
        print(log,Ylog::Color::GREEN);
        break;
    case WARNING:
        log = "["+string(time_buffer)+"][WARN ]["+context + "]"+"\r\n";
        print(log,Ylog::Color::YELLOW);
        break;
    case ERROR:
        log = "["+string(time_buffer)+"][ERROR]["+context + "]"+"\r\n";
        print(log,Ylog::Color::RED);
        break;
    case FATAL:
        log = "["+string(time_buffer)+"][FATAL]["+context + "]"+"\r\n";
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
            log = "["+string(time_buffer)+"][ERROR]["+"CAN NOT OPEN LOG FILE" + "]"+"\r\n";
            print(log,Ylog::Color::RED);
        }
        line_cnt=0;
    }
}

// 写入线程函数
void Ylog::write_loop(){
    auto* inst = Ylog::get_instance();   
    while(inst->running&&inst->m_fp->is_open()){
        std::queue<std::string>temp_queue;    
        {
            std::unique_lock<std::mutex>buf_lck(inst->buffer_mutex);
            inst->buffer_cond.wait(buf_lck,
                [inst](){
                return inst->write_action.load() || !inst->running;
            });
            std::swap(temp_queue, inst->log_buffer);
            inst->write_action=false;
        }
        // 批量写入
        std::string combined;
        while(!temp_queue.empty()){
            combined+=temp_queue.front();
            temp_queue.pop();
        }
        *inst->m_fp <<combined;
        inst->m_fp->flush();
    }
}

Ylog::Ylog(){

}

Ylog::~Ylog() {
    running = false;
    {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        buffer_cond.notify_one();
    }
    if (writer_thread.joinable()) {
        writer_thread.join();
    }
    if (m_fp) {
        m_fp->close();
    }
}