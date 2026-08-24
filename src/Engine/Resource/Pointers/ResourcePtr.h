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
// The subset of ResourcePool<T, KeyT>'s API that's handle-based rather than key-based - GetResource/LoadResource
// (the only places a pool's cache-key type, KeyT, actually matters) are deliberately excluded, since a
// ResourcePtr never calls them: it's handed an already-resolved ResourceHandle at construction (see
// ResourcePool::GetResource) and never looks anything up by key again. Depending on this narrow interface
// instead of ResourcePool<T, KeyT> directly means ResourcePtr<T> stays templated on T alone - a font's KeyT
// (FontKey - path+size, see FontPool.h) is purely an implementation detail of how FontPool caches things, not
// something every holder of a "pointer to a font" needs to know or spell out.
template <typename T>
class IResourceAccess
{
public:
	virtual ~IResourceAccess() = default;

	virtual T* Resolve(ResourceHandle handle)              = 0;
	virtual void AddRef(ResourceHandle handle)              = 0;
	virtual void Release(ResourceHandle handle)             = 0;
	virtual void Pin(ResourceHandle handle)                 = 0;
	virtual void Unpin(ResourceHandle handle)               = 0;
	virtual bool IsPinned(ResourceHandle handle) const      = 0;
	virtual size_t GetRefCount(ResourceHandle handle) const = 0;
	virtual bool EnsureResourceReady(GameContext& context, ResourceHandle handle) = 0;
};

// Resource pointer - works with both GPU and non-GPU resources. Holds a ResourceHandle (see Resource.h) rather
// than a cached T* - the pool's underlying storage is a SparseSet<T> (see Pools/SparseSet.h), which can relocate
// a live resource's address whenever some OTHER resource of the same type is erased (swap-and-pop), so every
// access below resolves through the pool fresh rather than trusting a pointer captured at construction time.
template <typename T>
class ResourcePtr
{
private:
	IResourceAccess<T>* m_pool = nullptr;
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

	ResourcePtr(IResourceAccess<T>* pool, ResourceHandle handle)
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

	// Keeps the underlying resource loaded even after every ResourcePtr (including this one) releases it - see
	// ResourcePool<T>::Pin. Lets a call site that re-fetches a resource fresh every frame (rather than holding a
	// persistent ResourcePtr somewhere) avoid reload/unload thrash: pin once up front, then just GetResource()
	// normally on every later access, and Unpin() (via any ResourcePtr to the same resource, e.g. another
	// GetResource() call - it doesn't have to be this exact instance) once it's no longer needed.
	void Pin() const
	{
		if (m_pool && m_handle.IsValid())
		{
			m_pool->Pin(m_handle);
		}
	}
	void Unpin() const
	{
		if (m_pool && m_handle.IsValid())
		{
			m_pool->Unpin(m_handle);
		}
	}
	bool IsPinned() const
	{
		return (m_pool && m_handle.IsValid()) ? m_pool->IsPinned(m_handle) : false;
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
