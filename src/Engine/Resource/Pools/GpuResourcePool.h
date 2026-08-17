#pragma once

#include <string>

#include "Debug/Assertions.h"
#include "Engine/Resource/Pools/ResourcePool.h"
#include "Engine/Resource/Resource.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
// GPU-specific resource pool
template <typename T>
class GpuResourcePool : public ResourcePool<T>
{
	static_assert(std::is_base_of_v<GpuResource, T>, "GpuResourcePool requires GpuResource-derived types");

private:
	size_t m_maxGpuMemory;
	size_t m_currentGpuMemory;

protected:
	// Only pool-level bookkeeping - the resource's own ~T() (via SparseSet::Erase, right after this runs) calls
	// UnloadFromGpu() itself, so this just keeps m_currentGpuMemory in sync before that happens.
	void UnloadResource(const std::string& filePath, T& resource) override
	{
		if (resource.gpuState == GpuResource::GpuState::LoadedToGpu)
		{
			m_currentGpuMemory -= resource.GetGpuMemoryUsage();
		}
	}

public:
	GpuResourcePool(size_t maxGpuMemory = 512 * 1024 * 1024)  // Default 512MB
	    : m_maxGpuMemory(maxGpuMemory),
	      m_currentGpuMemory(0)
	{
	}

	~GpuResourcePool() override = default;

	bool EnsureResourceReady(GameContext& context, ResourceHandle handle) override
	{
		T* resource = this->Resolve(handle);
		if (!resource)
		{
			return false;
		}

		// First ensure disk loading
		if (!resource->isLoaded && !resource->LoadFromDisk(context))
		{
			return false;
		}

		// Then ensure GPU loading
		if (resource->IsGpuReady())
		{
			return true;
		}

		if (resource->NeedsGpuReload())
		{
			size_t requiredMemory = resource->GetGpuMemoryUsage();

			// Free GPU memory if needed
			if (m_currentGpuMemory + requiredMemory > m_maxGpuMemory)
			{
				BREAK_MSG("The GPU's memory usage is full.");
				FreeUnusedGpuResources(requiredMemory);
			}

			// Load to GPU
			if (resource->LoadToGpu(context))
			{
				m_currentGpuMemory += requiredMemory;
				resource->gpuState = GpuResource::GpuState::LoadedToGpu;
				DEBUG_INFO("Loaded '%s' to GPU (%zu bytes)", resource->filePath, requiredMemory);
				return true;
			}
			else
			{
				BREAK_MSG("Failed to load '%s' to GPU", resource->filePath);
				return false;
			}
		}

		return false;
	}

	// Walks every loaded resource directly (packed, cache-friendly - see SparseSet) rather than a separately
	// maintained list of GPU-loaded pointers, which would go stale the moment any other resource in this pool
	// got erased and swap-and-pop relocated a live element.
	void FreeUnusedGpuResources(size_t neededMemory)
	{
		DEBUG_INFO("Freeing GPU memory (need %zu bytes)...", neededMemory);

		size_t freedMemory = 0;

		for (auto& entry : this->m_resources)
		{
			if (freedMemory >= neededMemory)
			{
				break;
			}

			T& resource = entry.resource;
			if (entry.refCount == 1 && resource.gpuState == GpuResource::GpuState::LoadedToGpu)
			{
				size_t memoryFreed = resource.GetGpuMemoryUsage();
				resource.UnloadFromGpu();
				resource.gpuState = GpuResource::GpuState::Unloaded;
				m_currentGpuMemory -= memoryFreed;
				freedMemory += memoryFreed;
				DEBUG_INFO("Freed '%s' from GPU (%zu bytes)", resource.filePath, memoryFreed);
			}
		}

		DEBUG_INFO("Freed '%zu' bytes from GPU", freedMemory);
	}

	void HandleGpuContextLost()
	{
		DEBUG_INFO("GPU context lost! Marking all GPU resources for reload...");

		for (auto& entry : this->m_resources)
		{
			if (entry.resource.gpuState == GpuResource::GpuState::LoadedToGpu)
			{
				entry.resource.gpuState = GpuResource::GpuState::GpuLost;
			}
		}
		m_currentGpuMemory = 0;
	}

	void ReloadAllGpuResources(GameContext& context)
	{
		DEBUG_INFO("Reloading all GPU resources after context restore...");

		for (auto& entry : this->m_resources)
		{
			T& resource = entry.resource;
			if (resource.gpuState == GpuResource::GpuState::GpuLost)
			{
				if (resource.LoadToGpu(context))
				{
					resource.gpuState = GpuResource::GpuState::LoadedToGpu;
					m_currentGpuMemory += resource.GetGpuMemoryUsage();
					DEBUG_INFO("Reloaded '%s' to GPU", resource.filePath);
				}
			}
		}
	}

	size_t GetGpuMemoryUsage() const
	{
		return m_currentGpuMemory;
	}
	size_t GetMaxGpuMemory() const
	{
		return m_maxGpuMemory;
	}
	float GetGpuMemoryUsagePercent() const
	{
		return m_maxGpuMemory > 0 ? (float)m_currentGpuMemory / m_maxGpuMemory * 100.0f : 0.0f;
	}
};
}  // namespace Resource
}  // namespace Struktur
