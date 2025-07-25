#pragma once

#include <string>

#include "Engine/Core/Resource/Resource.h"

namespace Struktur
{
	namespace Core
	{
		namespace Resource
		{
            template<typename T> class ResourcePool;
            // Resource pointer - works with both GPU and non-GPU resources
            template<typename T>
            class ResourcePtr 
            {
            private:
                T* m_ptr;
                size_t* m_refCount;
                ResourcePool<T>* m_pool;
                std::string m_filePath;
                
                void Release() 
                {
                    if (m_refCount) 
                    {
                        (*m_refCount)--;
                        if (*m_refCount == 0) 
                        {
                            if (m_pool) 
                            {
                                m_pool->OnResourceUnreferenced(m_filePath);
                            }
                            delete m_refCount;
                        }
                    }
                }

            public:
                ResourcePtr() : m_ptr(nullptr), m_refCount(nullptr), m_pool(nullptr) {}
                
                ResourcePtr(T* p, size_t* existingRefCount, ResourcePool<T>* resourcePool, const std::string& filePath) 
                    : m_ptr(p), m_refCount(existingRefCount), m_pool(resourcePool), m_filePath(filePath) {}
                
                ResourcePtr(const ResourcePtr& other) 
                    : m_ptr(other.m_ptr), m_refCount(other.m_refCount), m_pool(other.m_pool), m_filePath(other.m_filePath)
                {
                    if (m_refCount) 
                    {
                        (*m_refCount)++;
                    }
                }
                
                ResourcePtr(ResourcePtr&& other) noexcept 
                    : m_ptr(other.m_ptr), m_refCount(other.m_refCount), m_pool(other.m_pool), m_filePath(std::move(other.m_filePath))
                {
                    other.m_ptr = nullptr;
                    other.m_refCount = nullptr;
                    other.m_pool = nullptr;
                }
                
                ~ResourcePtr() { Release(); }
                
                ResourcePtr& operator=(const ResourcePtr& other)
                {
                    if (this != &other)
                    {
                        Release();
                        m_ptr = other.m_ptr;
                        m_refCount = other.m_refCount;
                        m_pool = other.m_pool;
                        m_filePath = other.m_filePath;
                        if (m_refCount)
                        {
                            (*m_refCount)++;
                        }
                    }
                    return *this;
                }
                
                ResourcePtr& operator=(ResourcePtr&& other) noexcept
                {
                    if (this != &other) {
                        Release();
                        m_ptr = other.m_ptr;
                        m_refCount = other.m_refCount;
                        m_pool = other.m_pool;
                        m_filePath = std::move(other.m_filePath);
                        other.m_ptr = nullptr;
                        other.m_refCount = nullptr;
                        other.m_pool = nullptr;
                    }
                    return *this;
                }
                
                T& operator*() const { return *m_ptr; }
                T* operator->() const { return m_ptr; }
                T* Get() const { return m_ptr; }
                
                size_t GetUseCount() const { return m_refCount ? *m_refCount : 0; }
                const std::string& GetFilePath() const { return m_filePath; }
                bool IsValid() const { return m_ptr != nullptr; }
                explicit operator bool() const { return IsValid(); }
                bool operator!() const { return !IsValid(); }
                
                // Template methods that work for both GPU and non-GPU resources
                bool IsReady() const
                { 
                    if (!m_ptr) return false;
                    
                    // Check if it's a GPU resource
                    if constexpr (std::is_base_of_v<GpuResource, T>)
                    {
                        return m_ptr->IsGpuReady();
                    }
                    else if constexpr (std::is_base_of_v<CpuResource, T>)
                    {
                        return m_ptr->IsHardwareReady();
                    }
                    else
                    {
                        return m_ptr->isLoaded;
                    }
                }
                
                bool EnsureReady() const
                { 
                    if (m_ptr && m_pool)
                    {
                        return m_pool->EnsureResourceReady(m_filePath);
                    }
                    return false; 
                }
            };
        }
    }
}
