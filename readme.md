## 一个简单的异步日志系统---Ylog
- 多线程安全
- 跨平台(Windows,Linux)
- 效率还行
- 自动存储到文件

- 修改了ERROR和MingGW标准库宏定义重名问题
- 修改了Windows控制台log文本颜色不对问题
- 修改了Windows系统 ctrl+c 中断信号被阻塞导致无法响应退出问题
- 增加了stop()停止log函数,析构直接调用stop();
- 其它少量修改