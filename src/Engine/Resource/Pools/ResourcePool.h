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
// Identity->display-string conversion for a pool's cache key. Found by ADL: this generic overload covers the
// default KeyT=std::string case (Texture/Sound/Music, where the key already IS the display name); a struct key
// (FontKey, ShaderKey - see FontPool.h/ShaderPool.h) provides its own overload next to its definition. Used only
// once, when a resource is first loaded (see GetResource) - ResourceEntry::name stores the resulting string, not
// KeyT itself, so nothing downstream (logging, PoolResourceEventCallback, the resource manager editor window)
// needs to know or care that a key might not be a plain string.
inline std::string ToDisplayName(const std::string& key)
{
	return key;
}

// Base resource pool - loaded resources live packed/contiguous in a SparseSet<ResourceEntry> (see
// Pools/SparseSet.h), cache-friendly to iterate, instead of individually heap-allocated and referenced by
// pointer. GetResource() looks up by key via m_nameToHandle on a cache hit/miss check; every access after that
// (refcounting, dereferencing through ResourcePtr<T>) goes through the resource's own ResourceHandle (see
// GameResource::selfHandle, set once right after it's loaded) rather than hashing its key again.
//
// KeyT is the cache identity - std::string by default (a file path is the whole identity), but a resource type
// that needs more than one field (Font: path+size, Shader: vs+fs paths) can use a small struct instead (see
// FontKey/ShaderKey) rather than encoding everything into one string and parsing it back apart in LoadResource.
//
// Implements IResourceAccess<T> (ResourcePtr.h) so that ResourcePtr<T> - which only ever needs handle-based
// operations, never GetResource/LoadResource - can hold a pointer to this pool without being templated on KeyT
// itself. KeyT is purely this pool's own cache-lookup detail.
template <typename T, typename KeyT = std::string>
class ResourcePool : public IResourceAccess<T>
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
		KeyT key;          // cache identity - what m_nameToHandle is actually keyed by, needed to erase on unload
		std::string name;  // ToDisplayName(key) - for logging/events/the resource manager editor window only

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
		m_nameToHandle.erase(entry.key);
		m_resources.Erase(internalHandle);
	}

	SparseSet<ResourceEntry> m_resources;
	std::unordered_map<KeyT, typename SparseSet<ResourceEntry>::Handle> m_nameToHandle;

#ifdef EDITOR
	PoolResourceEventCallback m_eventCallback;
#endif

	virtual T* LoadResource(GameContext& context, const KeyT& key) = 0;
	// Called for every entry right before it's erased (Release() dropping the last ref, or Clear()) - a hook for
	// subclasses (GpuResourcePool) needing to react before the resource itself is destroyed. The resource's own
	// destructor (run by SparseSet::Erase) already handles its own GPU/hardware teardown (UnloadFromGpu,
	// UnloadFromHardware, etc. - see ~TextureResource() and friends); this is for pool-level bookkeeping only.
	// Takes the display name (not KeyT) - it's purely informational (the actual erase already has the resolved
	// SparseSet handle, no lookup happens here), and a human-readable string is more useful to a hook than a raw
	// key struct would be.
	virtual void UnloadResource(const std::string& name, T& resource) {}

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

	ResourcePtr<T> GetResource(GameContext& context, const KeyT& key)
	{
		auto it = m_nameToHandle.find(key);
		if (it != m_nameToHandle.end())
		{
			DEBUG_INFO("Resource '%s' found in cache", m_resources[it->second].name);
			m_resources[it->second].refCount++;
			return ResourcePtr<T>(this, ToExternal(it->second));
		}

		std::string displayName = ToDisplayName(key);
		DEBUG_INFO("Loading resource '%s'", displayName);
#ifdef EDITOR
		// Brackets only the disk-load half (LoadResource -> LoadFromDisk) - GPU/hardware upload is a separate,
		// lazy step timed independently in GpuResourcePool<T>::EnsureResourceReady/SoundPool::EnsureResourceReady,
		// since it may happen much later than this call (see ResourceEventType::ReadyForUse's own comment).
		auto loadStart = std::chrono::steady_clock::now();
#endif
		T* loaded = LoadResource(context, key);
#ifdef EDITOR
		double loadSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - loadStart).count();
#endif
		if (!loaded)
		{
			DEBUG_INFO("Failed to load resource '%s'", displayName);
			return ResourcePtr<T>();
		}

		// LoadResource still hands back a heap-allocated T* (unchanged per-type LoadResource overrides) - moved
		// into the SparseSet's packed storage and the now-moved-from shell discarded. Only happens on a cache
		// miss (first load, or reload after full eviction), not on every GetResource call.
		typename SparseSet<ResourceEntry>::Handle internalHandle = m_resources.Emplace(std::move(*loaded));
		delete loaded;

		ResourceHandle handle     = ToExternal(internalHandle);
		ResourceEntry& entry      = m_resources[internalHandle];
		entry.key                 = key;
		entry.name                = std::move(displayName);
		entry.resource.selfHandle = handle;
		m_nameToHandle.emplace(key, internalHandle);

#ifdef EDITOR
		if (m_eventCallback)
		{
			m_eventCallback(ResourceEventType::Loaded, entry.name, entry.resource.GetMemoryUsage(), loadSeconds);
		}
#endif

		return ResourcePtr<T>(this, handle);
	}

	bool EnsureResourceReady(GameContext& context, ResourceHandle handle) override
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

	T* Resolve(ResourceHandle handle) override
	{
		ResourceEntry* entry = m_resources.Resolve(ToInternal(handle));
		return entry ? &entry->resource : nullptr;
	}

	void AddRef(ResourceHandle handle) override
	{
		auto internalHandle = ToInternal(handle);
		if (m_resources.IsValid(internalHandle))
		{
			m_resources[internalHandle].refCount++;
		}
	}

	void Release(ResourceHandle handle) override
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
	void Pin(ResourceHandle handle) override
	{
		auto internalHandle = ToInternal(handle);
		if (m_resources.IsValid(internalHandle))
		{
			m_resources[internalHandle].pinned = true;
		}
	}

	void Unpin(ResourceHandle handle) override
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

	bool IsPinned(ResourceHandle handle) const override
	{
		const ResourceEntry* entry = m_resources.Resolve(ToInternal(handle));
		return entry && entry->pinned;
	}

	size_t GetRefCount(ResourceHandle handle) const override
	{
		const ResourceEntry* entry = m_resources.Resolve(ToInternal(handle));
		return entry ? entry->refCount : 0;
	}

	// The pool's own display name for this handle (ToDisplayName(key), computed once in GetResource) - NOT always
	// the same as resource.filePath (e.g. ShaderResource's own filePath is independently built from its vs/fs
	// paths - see ShaderResource's constructor). Lets a caller (GpuResourcePool<T>::EnsureResourceReady firing a
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
