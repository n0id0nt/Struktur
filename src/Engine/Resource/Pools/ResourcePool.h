#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "Debug/Assertions.h"
#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/Pools/SparseSet.h"
#include "Engine/Resource/Resource.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
// Base resource pool - loaded resources live packed/contiguous in a SparseSet<ResourceEntry> (see
// Pools/SparseSet.h), cache-friendly to iterate, instead of individually heap-allocated and referenced by
// pointer. GetResource() looks up by path via m_nameToHandle on a cache hit/miss check; every access after that
// (refcounting, dereferencing through ResourcePtr<T>) goes through the resource's own ResourceHandle (see
// GameResource::selfHandle, set once right after it's loaded) rather than hashing its path again.
template <typename T>
class ResourcePool
{
   protected:
	struct ResourceEntry
	{
		T resource;
		size_t refCount = 1;
		// Independent from refCount - a pinned entry survives refCount dropping to 0 instead of being evicted (see
		// Release()/EvictIfUnused()). Lets call sites that don't want to hold a persistent ResourcePtr (e.g. a
		// splash/loading screen re-fetching its font from cache every frame) keep a resource resident by pinning
		// it once, rather than needing some long-lived owner elsewhere just to hold a reference open.
		bool pinned = false;
		std::string name;

		ResourceEntry(T&& res)
		    : resource(std::move(res))
		{
		}
	};

	// Shared by Release() and Unpin() - a resource is only actually evicted once neither is holding it open.
	void EvictIfUnused(typename SparseSet<ResourceEntry>::Handle internalHandle, ResourceEntry& entry)
	{
		if (entry.refCount > 0 || entry.pinned)
		{
			return;
		}

		DEBUG_INFO("Unloading unreferenced resource '%s'", entry.name);
		UnloadResource(entry.name, entry.resource);
		m_nameToHandle.erase(entry.name);
		m_resources.Erase(internalHandle);
	}

	SparseSet<ResourceEntry> m_resources;
	std::unordered_map<std::string, typename SparseSet<ResourceEntry>::Handle> m_nameToHandle;

	virtual T* LoadResource(GameContext& context, const std::string& filePath) = 0;
	// Called for every entry right before it's erased (Release() dropping the last ref, or Clear()) - a hook for
	// subclasses (GpuResourcePool) needing to react before the resource itself is destroyed. The resource's own
	// destructor (run by SparseSet::Erase) already handles its own GPU/hardware teardown (UnloadFromGpu,
	// UnloadFromHardware, etc. - see ~TextureResource() and friends); this is for pool-level bookkeeping only.
	virtual void UnloadResource(const std::string& filePath, T& resource)
	{
	}

	// ResourcePtr<T> holds the plain, non-template ResourceHandle (see Resource.h - avoids needing ResourcePool<T>
	// complete just to declare a member of this type, which would create a circular include with ResourcePtr.h).
	// Internally, storage is keyed by the SparseSet's own Handle - same {index, generation} layout, distinct type.
	static typename SparseSet<ResourceEntry>::Handle ToInternal(ResourceHandle handle)
	{
		return typename SparseSet<ResourceEntry>::Handle{handle.index, handle.generation};
	}
	static ResourceHandle ToExternal(typename SparseSet<ResourceEntry>::Handle handle)
	{
		return ResourceHandle{handle.index, handle.generation};
	}

   public:
	virtual ~ResourcePool()
	{
		Clear();
	}

	void Clear()
	{
		for (auto& entry : m_resources)
		{
			UnloadResource(entry.name, entry.resource);
		}
		m_resources.Clear();
		m_nameToHandle.clear();
	}

	ResourcePtr<T> GetResource(GameContext& context, const std::string& name)
	{
		auto it = m_nameToHandle.find(name);
		if (it != m_nameToHandle.end())
		{
			DEBUG_INFO("Resource '%s' found in cache", name);
			m_resources[it->second].refCount++;
			return ResourcePtr<T>(this, ToExternal(it->second));
		}

		DEBUG_INFO("Loading resource '%s'", name);
		T* loaded = LoadResource(context, name);
		if (!loaded)
		{
			DEBUG_INFO("Failed to load resource '%s'", name);
			return ResourcePtr<T>();
		}

		// LoadResource still hands back a heap-allocated T* (unchanged per-type LoadResource overrides) - moved
		// into the SparseSet's packed storage and the now-moved-from shell discarded. Only happens on a cache
		// miss (first load, or reload after full eviction), not on every GetResource call.
		typename SparseSet<ResourceEntry>::Handle internalHandle = m_resources.Emplace(std::move(*loaded));
		delete loaded;

		ResourceHandle handle      = ToExternal(internalHandle);
		ResourceEntry& entry       = m_resources[internalHandle];
		entry.name                 = name;
		entry.resource.selfHandle  = handle;
		m_nameToHandle.emplace(name, internalHandle);

		return ResourcePtr<T>(this, handle);
	}

	virtual bool EnsureResourceReady(GameContext& context, ResourceHandle handle)
	{
		auto internalHandle = ToInternal(handle);
		if (!m_resources.IsValid(internalHandle))
		{
			return false;
		}

		T& resource = m_resources[internalHandle].resource;
		if (!resource.isLoaded)
		{
			return resource.LoadFromDisk(context);
		}
		return true;
	}

	T* Resolve(ResourceHandle handle)
	{
		ResourceEntry* entry = m_resources.Resolve(ToInternal(handle));
		return entry ? &entry->resource : nullptr;
	}

	void AddRef(ResourceHandle handle)
	{
		auto internalHandle = ToInternal(handle);
		if (m_resources.IsValid(internalHandle))
		{
			m_resources[internalHandle].refCount++;
		}
	}

	void Release(ResourceHandle handle)
	{
		auto internalHandle  = ToInternal(handle);
		ResourceEntry* entry = m_resources.Resolve(internalHandle);
		if (!entry)
		{
			return;
		}

		entry->refCount--;
		EvictIfUnused(internalHandle, *entry);
	}

	// Keeps the resource resident even once refCount drops to 0 - see ResourceEntry::pinned. A plain flag, not a
	// count: pinning an already-pinned resource or unpinning an already-unpinned one is a harmless no-op, but two
	// independent call sites pinning the same resource need to coordinate their own unpin timing themselves,
	// since either one's Unpin() clears it for both.
	void Pin(ResourceHandle handle)
	{
		auto internalHandle = ToInternal(handle);
		if (m_resources.IsValid(internalHandle))
		{
			m_resources[internalHandle].pinned = true;
		}
	}

	void Unpin(ResourceHandle handle)
	{
		auto internalHandle  = ToInternal(handle);
		ResourceEntry* entry = m_resources.Resolve(internalHandle);
		if (!entry || !entry->pinned)
		{
			return;
		}

		entry->pinned = false;
		EvictIfUnused(internalHandle, *entry);
	}

	bool IsPinned(ResourceHandle handle) const
	{
		const ResourceEntry* entry = m_resources.Resolve(ToInternal(handle));
		return entry && entry->pinned;
	}

	size_t GetRefCount(ResourceHandle handle) const
	{
		const ResourceEntry* entry = m_resources.Resolve(ToInternal(handle));
		return entry ? entry->refCount : 0;
	}

	size_t GetLoadedCount() const
	{
		return m_resources.Size();
	}

	size_t GetTotalMemoryUsage() const
	{
		size_t total = 0;
		for (const auto& entry : m_resources)
		{
			total += entry.resource.GetMemoryUsage();
		}
		return total;
	}
};
}  // namespace Resource
}  // namespace Struktur
