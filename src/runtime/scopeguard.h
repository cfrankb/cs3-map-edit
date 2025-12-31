#pragma once

#include <iostream>
#include <functional>

// Simple ScopeGuard implementation (example, production code would be more robust)
class ScopeGuard
{
public:
    explicit ScopeGuard(std::function<void()> onExit = nullptr) : m_onExit(std::move(onExit)) {}
    ~ScopeGuard()
    {
        if (m_onExit)
            m_onExit();
    }
    // Disable copy/move to prevent the cleanup from being called multiple times
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard &operator=(const ScopeGuard &) = delete;

private:
    std::function<void()> m_onExit = nullptr;
};
