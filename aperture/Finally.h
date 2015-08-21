#pragma once 

#include <functional>

class finally
{
    std::function<void(void)> functor;
public:
    finally(const std::function<void(void)> &functor) : functor(functor) {}
    ~finally()
    {
        functor();
    }
};