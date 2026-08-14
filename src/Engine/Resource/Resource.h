#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "Debug/Assertions.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
// Opaque reference to a resource's own slot within whichever pool owns it (see ResourcePool<T>, which is backed
// by a SparseSet<T> - Engine/Resource/Pools/SparseSet.h). A plain, non-template type rather than
// SparseSet<T>::Handle itself so GameResource can hold one directly without pulling pool/template machinery into
// this header, and so ResourcePtr<T> can hold one without needing ResourcePool<T> to be a complete type (avoiding
// a circular include between ResourcePtr.h and ResourcePool.h) - ResourcePool<T> converts to/from its own
// SparseSet<T>::Handle (identical layout) at its public API boundary.
struct ResourceHandle
{
	static constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();

	uint32_t index      = kInvalidIndex;
	uint32_t generation = 0;

	bool IsValid() const
	{
		return index != kInvalidIndex;
	}
};

// Base resource class
class GameResource
{
   public:
	std::string filePath;
	bool isLoaded;
	// Set by whichever ResourcePool<T> owns this resource, right after it's emplaced - see ResourceHandle's own
	// comment. GameResource itself never interprets this beyond storing and moving it.
	ResourceHandle selfHandle;

	GameResource(const std::string& filePath)
	    : filePath(filePath),
	      isLoaded(false)
	{
	}
	virtual ~GameResource() = default;

	// Explicit rather than compiler-generated - a user-declared destructor (even one that's = default) suppresses
	// implicit move generation, and ResourcePool<T>'s SparseSet-backed storage needs T (and therefore every base
	// subobject) to be movable, both to relocate elements on Erase()'s swap-and-pop and for std::vector to grow
	// the dense array without falling back to copying.
	GameResource(GameResource&&)            = default;
	GameResource& operator=(GameResource&&) = default;
	GameResource(const GameResource&)       = delete;
	GameResource& operator=(const GameResource&) = delete;

	// Common resource management
	virtual bool LoadFromDisk(GameContext& context) = 0;
	virtual void UnloadFromDisk()         = 0;
	virtual size_t GetMemoryUsage() const = 0;
};

// GPU resource base class
class GpuResource : public GameResource
{
   public:
	enum class GpuState
	{
		Unloaded,     // Not loaded to GPU
		LoadedToGpu,  // Loaded to GPU and ready
		GpuLost       // GPU resource lost, need to reload
	};

	GpuState gpuState;

	GpuResource(const std::string& filePath)
	    : GameResource(filePath),
	      gpuState(GpuState::Unloaded)
	{
	}

	GpuResource(GpuResource&&)            = default;
	GpuResource& operator=(GpuResource&&) = default;

	// GPU-specific methods
	virtual bool LoadToGpu(GameContext& context) = 0;
	virtual void UnloadFromGpu()                 = 0;
	virtual bool IsGpuResourceValid() const  = 0;
	virtual size_t GetGpuMemoryUsage() const = 0;

	// State queries
	bool IsGpuReady() const
	{
		return gpuState == GpuState::LoadedToGpu && IsGpuResourceValid();
	}
	bool NeedsGpuReload() const
	{
		return gpuState == GpuState::GpuLost || (isLoaded && gpuState == GpuState::Unloaded);
	}
};

// CPU resource base class (for sounds, music, config files, etc.)
class CpuResource : public GameResource
{
   public:
	CpuResource(const std::string& filePath)
	    : GameResource(filePath)
	{
	}

	CpuResource(CpuResource&&)            = default;
	CpuResource& operator=(CpuResource&&) = default;

	// CPU resources might have hardware-specific loading (audio devices, etc.)
	virtual bool LoadToHardware(GameContext& context)
	{
		return true;
	}                                     // Default: no special hardware loading
	virtual void UnloadFromHardware() {}  // Default: no special hardware unloading
	virtual bool IsHardwareReady() const
	{
		return isLoaded;
	}
};
}  // namespace Resource
}  // namespace Struktur
