// 三种线程同步机制分别封装为三个类
#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

// 信号量
class sem {
public:
    // 创建并初始化
    sem() {
        if (sem_init(&m_sem, 0, 0) != 0) {
            // 构造函数没有返回值,通过抛出异常报告错误
            throw std::exception();
        }
    }

    // 创建并初始化特定个数
    sem(int num) {
        if (sem_init(&m_sem, 0, num) != 0) {
            throw std::exception();
        }
    }

    // 销毁
    ~sem() {
        sem_destroy(&m_sem);
    }

    // 等待
    bool wait() {
        return sem_wait(&m_sem) == 0;
    }

    // 增加
    bool post() {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

// 互斥锁
class locker {
public:
    locker() {
        if (pthread_mutex_init(&m_mutex, NULL) != 0) {
            throw std::exception();
        }
    }

    ~locker() {
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock() {
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    pthread_mutex_t *get() {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

// 线程间同步共享数据的一种通知机制
// 与信号量一起使用,进入wait时解锁信号量,等待boardcast
// 这里写法与书中不同,可能是考虑迟到可以使用任意mutex吧
// 条件变量
class cond {
public:
    cond() {
        if (pthread_cond_init(&m_cond, NULL) != 0) {
            //pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }

    ~cond() {
        pthread_cond_destroy(&m_cond);
    }

    bool wait(pthread_mutex_t *m_mutex) {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, m_mutex);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }

    bool timewait(pthread_mutex_t *m_mutex, struct timespec t) {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }

    bool signal() {
        return pthread_cond_signal(&m_cond) == 0;
    }

    // 用来广播唤醒
    bool broadcast() {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    //static pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

#endif
