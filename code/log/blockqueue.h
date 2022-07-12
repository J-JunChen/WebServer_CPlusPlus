/*************************************************************
*循环数组实现的阻塞队列，m_back = (m_back + 1) % m_max_size;  
*线程安全，每个操作前都要先加互斥锁，操作完后，再解锁
**************************************************************/

#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <deque>
#include <assert.h>
#include <mutex>

template<class T>
class BlockDeque //阻塞双端队列
{
private:
    size_t capacity_;
    bool isClose_;

    std::deque<T> deq_; // 双端队列
    std::mutex mtx_;

    std::condition_variable condConsumer_;
    std::condition_variable condProducer_;

public:
    explicit BlockDeque(size_t);
    /* explicit的作用？
       标明类的构造函数是显式的，不能进行隐式转换。*/
    ~BlockDeque();

    void Close();

    void flush();

};

template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity): capacity_(MaxCapacity)
{
    assert(MaxCapacity > 0);
    isClose_ = false;
};

template<class T>
BlockDeque<T>::~BlockDeque(){
    Close();
};

template<class T>
void BlockDeque<T>::Close(){
    {
        /* When a lock_guard object is created, it attempts to take ownership of the mutex it is given. When control leaves the scope in which the lock_guard object was created, the lock_guard is destructed and the mutex is released. */
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear();
        isClose_ = true;
    }
    
}

template<class T>
void BlockDeque<T>::flush() {
    /*
        Notify one:
        Unblocks one of the threads currently waiting for this condition.

        If no threads are waiting, the function does nothing.
        If more than one, it is unspecified which of the threads is selected.

        notify_one()：因为只唤醒等待队列中的第一个线程；不存在锁争用，所以能够立即获得锁。其余的线程不会被唤醒
    */
    condConsumer_.notify_one();
};


#endif // BLOCKQUEUE_H