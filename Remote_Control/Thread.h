#pragma once

#include "pch.h"
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>
#include <Windows.h>

// 线程函数基类
class ThreadFuncBase {};

// 成员函数指针类型定义
// - 返回值 int 用于控制 worker 生命周期（<0 释放线程，>=0 继续占用）
typedef int (ThreadFuncBase::* FUNCTYPE)();
class ThreadWorker {
public:
    ThreadWorker() : thiz(NULL), func(NULL) {}

    // 带参构造函数
    ThreadWorker(void* obj, FUNCTYPE f)
        : thiz((ThreadFuncBase*)obj), func(f) {
    }

    // 拷贝构造函数
    ThreadWorker(const ThreadWorker& worker) {
        thiz = worker.thiz;
        func = worker.func;
    }

    // 拷贝赋值运算符
    ThreadWorker& operator=(const ThreadWorker& worker) {
        if (this != &worker) {
            thiz = worker.thiz;
            func = worker.func;
        }
        return *this;
    }

    // 函数调用运算符
    int operator()() {
        if (IsValid()) {
            return (thiz->*func)();
        }
        return -1;
    }

    // 判断 worker 是否有效
    bool IsValid() const {
        return (thiz != NULL) && (func != NULL);
    }

private:
    ThreadFuncBase* thiz; // 指向具体业务对象的指针
    FUNCTYPE func;        // 成员函数指针
};

class CThread {
public:
    CThread()
        : m_bStatus(false) {
    }

    ~CThread() {
        Stop();
    }

    // 启动线程并返回是否成功
    bool Start() {
        std::lock_guard<std::mutex> lk(m_stateLock);

        if (m_bStatus) return true;

        m_bStatus = true;
        m_thread = std::thread(&CThread::ThreadWorkerLoop, this);

        // 如果 thread 启动失败会抛异常
        return IsValid();
    }

    //返回 true 表示线程仍在运行
    bool IsValid() {
        return m_bStatus.load()/*以原子方式读取当前值*/ 
            && m_thread.joinable()/*thread 对象里是否真的有一个可 join 的系统线程*/;
    }

    // 让线程退出并等待结束，然后清理 worker
    bool Stop() {
        {
            std::lock_guard<std::mutex> lk(m_stateLock);
            if (m_bStatus == false) return true;
            m_bStatus = false;
        }

        // 唤醒线程，让它能及时退出
        m_cv.notify_all();

        if (m_thread.joinable()) {
            m_thread.join();
        }
        // 清 worker
        UpdateWorker();
        return true;
    }

    //worker 有效 => 设置 worker， worker 无效 => 清空 worker
    void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {
        std::lock_guard<std::mutex> lk(m_workerLock);

        if (!worker.IsValid()) {
            m_worker.reset();// release resource and convert to empty shared_ptr object
        }
        else {
            m_worker = std::make_shared<::ThreadWorker>(worker);
        }

        // 通知线程：有新 worker 或 worker 被清空
        m_cv.notify_one();
    }

    // true 表示空闲
    bool IsIdle() {
        std::lock_guard<std::mutex> lk(m_workerLock);
		if (!m_worker) return true;// worker不存在
		return !m_worker->IsValid();// worker无效
    }

private:
    // 线程循环
    void ThreadWorkerLoop() {
        while (m_bStatus.load()) {

            std::shared_ptr<::ThreadWorker> localWorker;

            // 1) 等待：直到出现有效 worker 或线程被停止
            {
                std::unique_lock<std::mutex> lk(m_workerLock);
                m_cv.wait(lk, [&]() {
                    // 被 Stop() 或 有 worker 才醒
					if (!m_bStatus.load()) return true;// Stop 请求退出
                    return (m_worker && m_worker->IsValid());
                    });

                if (!m_bStatus.load()) {
                    break; // Stop 请求退出
                }
                // 拷贝 shared_ptr 到局部，避免执行时持锁
                localWorker = m_worker;
            }
            // 2) 执行任务
            if (localWorker && localWorker->IsValid()) {
                int ret = (*localWorker)();
                // ret < 0 才清 worker，ret==0 则持续占用，线程认为自己在忙
                if (ret < 0) {
                    std::lock_guard<std::mutex> lk(m_workerLock);
                    m_worker.reset();
                }
            }
        }
    }

private:
    // 线程状态
    std::atomic<bool> m_bStatus; // false 表示线程将要关闭 true 表示线程正在运行
    std::thread m_thread;

    // worker 数据
    std::mutex m_workerLock;              // 保护 m_worker
    std::shared_ptr<::ThreadWorker> m_worker;

    // Start/Stop 状态锁
    std::mutex m_stateLock;

    // 唤醒机制
    std::condition_variable m_cv;
};


class CThreadPool {
public:
    CThreadPool(size_t size) {
        m_threads.resize(size);
        for (size_t i = 0; i < size; i++) {
            m_threads[i] = new CThread();
        }
    }

    CThreadPool() {}

    ~CThreadPool() {
        Stop();
        for (size_t i = 0; i < m_threads.size(); i++) {
            delete m_threads[i];
            m_threads[i] = NULL;
        }
        m_threads.clear();
    }

    // Invoke: 启动所有线程
    bool Invoke() {
        bool ret = true;
        for (size_t i = 0; i < m_threads.size(); i++) {
            if (m_threads[i]->Start() == false) {
                ret = false;
                break;
            }
        }

        // 若失败则回滚：停止全部
        if (ret == false) {
            for (size_t i = 0; i < m_threads.size(); i++) {
                m_threads[i]->Stop();
            }
        }
        return ret;
    }

    // Stop: 停止所有线程
    void Stop() {
        for (size_t i = 0; i < m_threads.size(); i++) {
            if (m_threads[i]) m_threads[i]->Stop();
        }
    }

    //-1表示分配失败，所有线程都在忙; >=0 表示第n个线程被分配
    int DispatchWorker(const ThreadWorker& worker) {
        int index = -1;
        std::lock_guard<std::mutex> lk(m_lock);
        for (size_t i = 0; i < m_threads.size(); i++) {
            if (m_threads[i] != nullptr && m_threads[i]->IsIdle()) {
                m_threads[i]->UpdateWorker(worker);
                index = (int)i;
                break;
            }
        }
        return index;
    }

	// 检查指定线程是否有效
    bool CheckThreadValid(size_t index) {
        if (index < m_threads.size()) {
            return m_threads[index]->IsValid();
        }
        return false;
    }

private:
    std::mutex m_lock;
    std::vector<CThread*> m_threads;
};
