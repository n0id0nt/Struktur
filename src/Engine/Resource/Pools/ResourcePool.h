#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "Debug/Assertions.h"
#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/Pools/SparseSet.h"
#include "Engine/Resource/Resource.h"
#include "Engine/Resource/ResourceEvents.h"

#ifdef EDITOR
#include <chrono>
#endif

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
#ifdef EDITOR
		if (m_eventCallback)
		{
			// Nothing to time on an unload, so 0.0 seconds - see PoolResourceEventCallback's own comment for why
			// this callback doesn't carry a category (ResourceManager tags that on, one layer up).
			m_eventCallback(ResourceEventType::Unloaded, entry.name, entry.resource.GetMemoryUsage(), 0.0);
		}
#endif
		UnloadResource(entry.name, entry.resource);
		m_nameToHandle.erase(entry.name);
		m_resources.Erase(internalHandle);
	}

	SparseSet<ResourceEntry> m_resources;
	std::unordered_map<std::string, typename SparseSet<ResourceEntry>::Handle> m_nameToHandle;

#ifdef EDITOR
	PoolResourceEventCallback m_eventCallback;
#endif

	virtual T* LoadResource(GameContext& context, const std::string& filePath) = 0;
	// Called for every entry right before it's erased (Release() dropping the last ref, or Clear()) - a hook for
	// subclasses (GpuResourcePool) needing to react before the resource itself is destroyed. The resource's own
	// destructor (run by SparseSet::Erase) already handles its own GPU/hardware teardown (UnloadFromGpu,
	// UnloadFromHardware, etc. - see ~TextureResource() and friends); this is for pool-level bookkeeping only.
	virtual void UnloadResource(const std::string& filePath, T& resource) {}

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
#ifdef EDITOR
		// Brackets only the disk-load half (LoadResource -> LoadFromDisk) - GPU/hardware upload is a separate,
		// lazy step timed independently in GpuResourcePool<T>::EnsureResourceReady/SoundPool::EnsureResourceReady,
		// since it may happen much later than this call (see ResourceEventType::ReadyForUse's own comment).
		auto loadStart = std::chrono::steady_clock::now();
#endif
		T* loaded = LoadResource(context, name);
#ifdef EDITOR
		double loadSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - loadStart).count();
#endif
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

		ResourceHandle handle     = ToExternal(internalHandle);
		ResourceEntry& entry      = m_resources[internalHandle];
		entry.name                = name;
		entry.resource.selfHandle = handle;
		m_nameToHandle.emplace(name, internalHandle);

#ifdef EDITOR
		if (m_eventCallback)
		{
			m_eventCallback(ResourceEventType::Loaded, entry.name, entry.resource.GetMemoryUsage(), loadSeconds);
		}
#endif

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

	// The pool's own cache key for this handle - NOT always the same as resource.filePath (e.g. FontPool strips
	// the "_<size>" suffix off before constructing FontResource, ShaderPool splits "vs,fs" into two separate
	// paths - see each pool's own LoadResource). Lets a caller (GpuResourcePool<T>::EnsureResourceReady firing a
	// ReadyForUse event, see ResourceEvents.h) report the same name the table/Loaded/Unloaded events already use,
	// instead of a resource-type-specific filePath that can disagree with it.
	std::string GetName(ResourceHandle handle) const
	{
		const ResourceEntry* entry = m_resources.Resolve(ToInternal(handle));
		return entry ? entry->name : std::string();
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

#ifdef EDITOR
	// Read-only walk over every currently-cached entry - for the resource manager editor window's live table
	// (ResourceManagerWindow.cpp). fn is called once per entry with (name, resource, refCount, pinned); m_resources
	// (a SparseSet) already supports const range-for internally (see GetTotalMemoryUsage above), this just
	// surfaces that publicly instead of leaving it protected.
	template <typename F>
	void ForEachEntry(F&& fn) const
	{
		for (const auto& entry : m_resources)
		{
			fn(entry.name, entry.resource, entry.refCount, entry.pinned);
		}
	}

	// Registers a sink for Loaded/Unloaded events fired by this pool (ReadyForUse is fired by subclasses that
	// have a GPU/hardware-upload step - see GpuResourcePool<T>/SoundPool). No category is included - see
	// PoolResourceEventCallback's own comment; ResourceManager::SetResourceEventCallback is the category-aware
	// public entry point everything outside this file should actually use.
	void SetResourceEventCallback(PoolResourceEventCallback cb)
	{
		m_eventCallback = std::move(cb);
	}
#endif
};
}  // namespace Resource
}  // namespace Struktur
