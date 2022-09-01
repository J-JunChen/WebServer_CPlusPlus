
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <functional>


class ThreadPool
{
private:
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool isClosed;
        std::queue<std::function<void()>> tasks;
        /*
            std::function
        */

    };
    std::shared_ptr<Pool> pool_;

public:
    // head文件不建议把 构造函数 写在类外
    explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>())
    {
    /*
        make_shared 是标准库函数，此函数在动态内存中分配一个对象并初始化它，返回指向此对象的 shared_ptr。由于是通过shared_ptr管理内存，因此这是一种安全分配和使用动态内存的方法。
    */
        assert(threadCount > 0);
        for(size_t i = 0; i < threadCount; i++) {
            std::thread([pool = pool_]{
                std::unique_lock<std::mutex> locker(pool->mtx);
                while (true){
                    if(!pool->tasks.empty()){
                        auto task = std::move(pool->tasks.front()); // std::move?
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    else if(pool->isClosed) break;
                    else pool->cond.wait(locker);
                    /*
                    std::condition_variable::wait( std::unique_lock<std::mutex>& lock )
                        wait causes the current thread to block until the condition variable is notified or a spurious wakeup occurs, optionally looping until some predicate is satisfied
                    */
                }
            }).detach();
            /*
                .detach()的作用是将子线程和主线程的关联分离，也就是说detach()后子线程在后台独立继续运行，主线程无法再取得子线程的控制权，即使主线程结束，子线程未执行也不会结束。 当主线程结束时，由运行时库负责清理与子线程相关的资源。
            */
        }
    };

    ~ThreadPool(){
        if(static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            pool_->cond.notify_all();
        }
    };
    
    ThreadPool() = default;
    
    ThreadPool(ThreadPool&&) = default;

    template<class F>
    void AddTask(F&& task){
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            /*
                std::forward
            */
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond.notify_one();
    };
};


#endif