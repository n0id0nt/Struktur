#pragma once

// Editor-only resource load/unload event plumbing - see ResourcePool<T>::SetResourceEventCallback /
// ResourceManager::SetResourceEventCallback. Kept in its own header (rather than folded into ResourcePool.h)
// so pools/ResourceManager can share one definition of "what an event looks like" without ResourcePool.h
// needing to know which pool it's part of - ResourceManager tags each pool's events with its own
// ResourceCategory when it forwards them (see ResourceManager::SetResourceEventCallback). Entirely compiled
// out of non-editor builds, matching the #ifdef EDITOR block Engine/Core/FileSystem.h already uses for its
// own editor-only methods - zero footprint, zero behavior change for any non-editor code path.
#ifdef EDITOR

#include <functional>
#include <string>

namespace Struktur
{
namespace Resource
{
enum class ResourceCategory
{
	Texture,
	Sound,
	Music,
	Font,
	Shader
};

enum class ResourceEventType
{
	Loaded,       // Finished ResourcePool<T>::GetResource()'s disk-load path (a cache miss) - `seconds` is the
	              // disk-load duration.
	ReadyForUse,  // Just finished GPU upload (GpuResourcePool<T>::EnsureResourceReady) or audio-hardware load
	              // (SoundPool::EnsureResourceReady) - `seconds` is that upload/hardware-load duration.
	Unloaded      // Evicted (ResourcePool<T>::EvictIfUnused, refCount hit 0 and not pinned) - `seconds` is 0.0,
	              // nothing to time.
};

// memoryBytes is the resource's own GetMemoryUsage() (CPU-side) at the moment of the event - cheap to read
// again from the table's own polling for a live value, but useful here so the event log can show it without
// the window needing to hold a reference back into the pool.
//
// Two callback shapes: ResourcePool<T> itself has no notion of "which pool am I" (TexturePool/SoundPool/etc.
// are just template instantiations, see ResourcePool.h), so it fires the category-less
// PoolResourceEventCallback. ResourceManager::SetResourceEventCallback (the public, editor-facing API) takes
// the full ResourceEventCallback and wraps one category-tagging lambda per pool around it - see its own
// comment. A caller outside ResourceManager should only ever need ResourceEventCallback.
using PoolResourceEventCallback =
    std::function<void(ResourceEventType type, const std::string& name, size_t memoryBytes, double seconds)>;
using ResourceEventCallback = std::function<void(ResourceCategory category, ResourceEventType type,
                                                  const std::string& name, size_t memoryBytes, double seconds)>;
}  // namespace Resource
}  // namespace Struktur

#endif  // EDITOR
