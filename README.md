## Lightweight Web Server
### Summary:
A simple, fast, multithreaded web server platform implemented using C++11.

###	Project Highlights:
- Combined I/O multiplexing technology epoll and thread pool to achieve a high concurrency server model that improved the performance of the system.
- Created timer based on min heap algorithm to close the dormant client http connection periodically that reduced memory footprint by 50%.
- Built the asynchronous logging system using singleton pattern and customed blocking queue to record the server running status, which helped server runs twice as fast as the synchronous one.

---

涉及技术：C++ + MySQL + I/O 复用技术 + Reactor

项目背景：为了巩固所学的 C++ 知识，基于 C++11 实现的高性能 WEB 服务器，可经过每秒上万次查询量的压力测试。

要点：

- 利用 IO 复用技术 Epoll 与线程池实现多线程的 Reactor 高并发服务器模型，解决了单个线程只能监听单个客户连接所造成线程资源浪费的问题，带来了减少开销的效果。
- 利用小根堆算法实现服务器定时器，周期性地关闭超时的休眠连接，解决了非活跃连接占用连接资源的问题，减轻服务器压力。
- 利用单例模式与自定义阻塞队列实现异步日志系统，记录服务器运行状态，解决了同步日志系统造成阻塞等待的问题，增强了服务器的并发能力。
- 使用标准库容器 vector 封装 char 数据类型，实现自动增长的读写缓冲区。 
- 利用 Webbench 对服务器进行压力测试，实现上万的并发连接。
