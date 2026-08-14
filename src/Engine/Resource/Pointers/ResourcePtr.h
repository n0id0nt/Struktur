#pragma once

#include <string>

#include "Debug/Assertions.h"
#include "Engine/Resource/Resource.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
template <typename T>
class ResourcePool;

// Resource pointer - works with both GPU and non-GPU resources. Holds a ResourceHandle (see Resource.h) rather
// than a cached T* - the pool's underlying storage is a SparseSet<T> (see Pools/SparseSet.h), which can relocate
// a live resource's address whenever some OTHER resource of the same type is erased (swap-and-pop), so every
// access below resolves through the pool fresh rather than trusting a pointer captured at construction time.
template <typename T>
class ResourcePtr
{
   private:
	ResourcePool<T>* m_pool = nullptr;
	ResourceHandle m_handle;

	void Release()
	{
		if (m_pool && m_handle.IsValid())
		{
			m_pool->Release(m_handle);
		}
		m_pool   = nullptr;
		m_handle = ResourceHandle{};
	}

   public:
	ResourcePtr() = default;

	ResourcePtr(ResourcePool<T>* pool, ResourceHandle handle)
	    : m_pool(pool),
	      m_handle(handle)
	{
	}

	ResourcePtr(const ResourcePtr& other)
	    : m_pool(other.m_pool),
	      m_handle(other.m_handle)
	{
		if (m_pool && m_handle.IsValid())
		{
			m_pool->AddRef(m_handle);
		}
	}

	ResourcePtr(ResourcePtr&& other) noexcept
	    : m_pool(other.m_pool),
	      m_handle(other.m_handle)
	{
		other.m_pool   = nullptr;
		other.m_handle = ResourceHandle{};
	}

	~ResourcePtr()
	{
		Release();
	}

	ResourcePtr& operator=(const ResourcePtr& other)
	{
		if (this != &other)
		{
			Release();
			m_pool   = other.m_pool;
			m_handle = other.m_handle;
			if (m_pool && m_handle.IsValid())
			{
				m_pool->AddRef(m_handle);
			}
		}
		return *this;
	}

	ResourcePtr& operator=(ResourcePtr&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			m_pool         = other.m_pool;
			m_handle       = other.m_handle;
			other.m_pool   = nullptr;
			other.m_handle = ResourceHandle{};
		}
		return *this;
	}

	T& operator*() const
	{
		return *Get();
	}
	T* operator->() const
	{
		return Get();
	}
	T* Get() const
	{
		return (m_pool && m_handle.IsValid()) ? m_pool->Resolve(m_handle) : nullptr;
	}

	size_t GetUseCount() const
	{
		return (m_pool && m_handle.IsValid()) ? m_pool->GetRefCount(m_handle) : 0;
	}
	const std::string& GetFilePath() const
	{
		static const std::string kEmpty;
		T* resource = Get();
		return resource ? resource->filePath : kEmpty;
	}
	bool IsValid() const
	{
		return Get() != nullptr;
	}
	explicit operator bool() const
	{
		return IsValid();
	}
	bool operator!() const
	{
		return !IsValid();
	}

	// Template methods that work for both GPU and non-GPU resources
	bool IsReady() const
	{
		T* resource = Get();
		if (!resource)
		{
			return false;
		}

		// Check if it's a GPU resource
		if constexpr (std::is_base_of_v<GpuResource, T>)
		{
			return resource->IsGpuReady();
		}
		else if constexpr (std::is_base_of_v<CpuResource, T>)
		{
			return resource->IsHardwareReady();
		}
		else
		{
			return resource->isLoaded;
		}
	}

	bool EnsureReady(GameContext& context) const
	{
		if (m_pool && m_handle.IsValid())
		{
			return m_pool->EnsureResourceReady(context, m_handle);
		}
		return false;
	}
};
}  // namespace Resource
}  // namespace Struktur
