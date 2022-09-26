/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */ 
#include "heaptimer.h"

void HeapTimer::siftup_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;
    while(j >= 0) {
        if(heap_[j] < heap_[i]) { break; }
        SwapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

void HeapTimer::SwapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
} 

bool HeapTimer::siftdown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1; // 左孩子
    while(j < n) {
        if(j + 1 < n && heap_[j + 1] < heap_[j]) j++; // 看左右孩子哪个更大
        if(heap_[i] < heap_[j]) break;
        SwapNode_(i, j);
        i = j;  // 一直往下，如果是左子树，就往左支路比较；如果是在右子树，就往右支路比较。
        j = i * 2 + 1;
    }
    return i > index; // 是否有调整
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    assert(id >= 0);
    size_t i;
    if(ref_.count(id) == 0) {
        /* 新节点：堆尾插入，调整堆 */
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});
        siftup_(i);
    } 
    else {  // 通常不执行 else 语句，因为通常加进来都是新的连接
        /* 已有结点：调整堆 */
        i = ref_[id];
        heap_[i].expires = Clock::now() + MS(timeout);
        heap_[i].cb = cb;
        if(!siftdown_(i, heap_.size())) { // 往下没有调整，就往上调整
            siftup_(i);
        }
    }
}

void HeapTimer::del_(size_t index) {  // 通常是删除堆顶，即 index = 0
    /* 删除指定位置的结点 */
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if(i < n) {
        SwapNode_(i, n);
        if(!siftdown_(i, n)) { // 往下没有调整，就往上调整，注意是 n=heap_.size() - 1;
            siftup_(i);
        }
    }
    /* 队尾元素删除 */
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::adjust(int id, int timeout) {
    /* 调整指定id的结点 */
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);;
    siftdown_(ref_[id], heap_.size()); // 因为是小根堆，所以超时时间增加，只需要往下调整
}

void HeapTimer::tick() {
    /* 清除超时结点 */
    if(heap_.empty()) {
        return;
    }
    while(!heap_.empty()) { // 不断比较堆顶元素是否超时
        TimerNode node = heap_.front(); // 堆顶元素
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) { 
            break; 
        }
        node.cb(); // 执行回调函数
        pop();
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);  // 把堆顶和堆尾元素交换，删除最后一个元素，然后从0开始siftdown
}

void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

int HeapTimer::GetNextTick() {
    tick(); // 清除超时结点
    size_t res = -1;
    if(!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count(); // MS： milliseconds; count() 表示多少次MS。
        if(res < 0) { res = 0; }
    }
    return res;  // 返回下次堆顶的超时时间，这样每次都能保证清除至少一个连接
    // 由于如果有事件发生，那么堆顶有可能改变，也只是做一次检查，不一定清除元素。
    /* 
        timeMS = timer_->GetNextTick(); 
        int eventCnt = epoller_->Wait(timeMS); // epoller_wait 只会阻塞 timeMS 时间
    */

}