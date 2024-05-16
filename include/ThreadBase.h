#pragma once
// ThreadBase.h

#ifndef THREADBASE_H
#define THREADBASE_H

#include <thread>
#include <iostream>

class ThreadBase {
public:
    ThreadBase() {}
    virtual ~ThreadBase() {}

    void start() {
        thread_ = std::thread(&ThreadBase::run, this);
    }

    void join() {
        if (thread_.joinable())
            thread_.join();
    }

    virtual void run() = 0; // 纯虚函数，派生类需要实现

private:
    std::thread thread_;
};

#endif
