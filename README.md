# WEB_SERVER
[![license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://opensource.org/licenses/MIT)
## 高性能静态网页服务器
   这是一个 Web 服务器项目，目前支持 GET 和 HEAD 请求的解析，POST请求待更新，可以处理静态网页的请求，支持 HTTP 长/短连接，以异步日志的方式保存服务器的运行状态。（大部分源代码都有详尽的注释，值得学习，因水平有限，如有疏漏或错误，直接发在issues即可）
## 使用指南
 * git clone https://github.com/StormrageZ/WEB_SERVER.git
 * cd WEB_SERVER
 * ./build.sh
 * 运行成功后会出现一个build文件下，进入后找到WebServer文件
 * 终端以root权限运行 ./WebServer即可可通过 -p （端口） -t （线程数） -l（日志路径） 改变配置
 * 需要在WebServer的同级文件夹下面放一个index.html 文件
 * 浏览器打开127.0.0.1即可看到该网页
## 技术要点
* 采用 Reactor 的事件处理模式
* 使用 Epoll 的 ET 模式实现高效的 I/O 多路复用，NoblockIO
* 通过设置 TCP LINGER 选项支持优雅关闭连接
* 使用了 C++ 中 priority_queue 数据结构 ，以小根堆的形式管理定时器，处理超时连接 
* 使用了多线程处理并发连接，提高 CPU 的利用率，通过线程池管理工作线程，减少了线程创建和销毁的开销
* 使用了Linux内核中的 eventfd 进行线程间的通信， 实现线程的异步唤醒
* 参考陈硕的 muduo 网络库使用双缓冲区技术实现了异步日志系统，保存程序运行状态以及连接信息
* 通过 shared_ptr 智能指针的RAII机制减少内存泄露
* 使用状态机解析了 HTTP 请求,支持管线化
## Reactor 模式
  Reactor 是这样的一种模式，他要求主线程只负责监听文件描述上是否有时间发生，有的话就立即将该事件通知工作线程。除此之外，主线程不做任何其他实质性的工作。读写数据，接受新的连接，以及处理客户请求均在工作线程中完成。
  使用同步I/O模型（以 epoll 为例）实现的 Reactor 模式的工作流程是：
* 主线程往 epoll 内核事件表中注册 socket 读就绪事件(EPOLLIN）
* 主线程调用 epoll_wait 等待socket上的读就绪事件
* 当socket上有数据可读时，epoll_wait 通知主线程。主线程将socket可读事件放入请求队列。
* 睡眠在请求队列上的某个工作线程被唤醒，他从 socket 读取数据，并处理客户请求，然后往 epoll 内核事件表中注册该 socket 上的写就绪事件。
<div align="center"> <img src="https://github.com/StormrageZ/WEB_SERVER/blob/master/docs/Reactor.png" /> </div><br>

## One Loop per Thread
<div align="center"> <img src="https://github.com/StormrageZ/WEB_SERVER/blob/master/docs/threadloop.png" /> </div><br>

## 处理新连接的流程
* 根据配置的端口号、工作线程数和Log记录path开启服务器
* 初始化服务器（初始化主线程、初始化线程池、绑定listen_fd、INADDR_ANY和port）
* 开启线程池，Channel 给主线程分发对应的IO处理函数（处理新连接，RR分配给子线程）
* 开启主线程，开始事件循环，当新的客户端连接到来时，主线程epoll_fd监听的listen_fd出现可读事件，accept循环接受新的连接，并且以RR的方式分配给子线程。
