#pragma once

#include <unordered_map>
#include <string>
#include <format>

#include "Engine/Core/Resource/Resource.h"
#include "Engine/Core/Resource/ResourcePtr.h"

#include "Debug/Assertions.h"

namespace Struktur
{
	namespace Core
	{
		namespace Resource
		{
			// Base resource pool
			template<typename T>
			class ResourcePool 
			{
			protected:
				// TODO might be a good idea to make these not pointers so the memory is stores in series and pass around the pointers to these items
				struct ResourceEntry
				{
					T* resource = nullptr;
					size_t* refCount = nullptr;
					
					ResourceEntry(T* res) : resource(res), refCount(new size_t(1)) {}
				};
				
				std::unordered_map<std::string, ResourceEntry> m_loadedResources;
				
				virtual T* LoadResource(const std::string& filePath) = 0;
				virtual void UnloadResource(const std::string& filePath, T* resource) { delete resource; }

			public:
				virtual ~ResourcePool() 
				{
					Clear();
				}

				void Clear()
				{
					for (auto& pair : m_loadedResources) 
					{
						UnloadResource(pair.first, pair.second.resource);
						delete pair.second.refCount;
					}
					m_loadedResources.clear();
				}
    
				ResourcePtr<T> GetResource(const std::string& name) 
				{
					auto it = m_loadedResources.find(name);
					
					if (it != m_loadedResources.end()) 
					{
						DEBUG_INFO(std::format("Resource '{}' found in cache", name).c_str());	
						(*(it->second.refCount))++;
						return ResourcePtr<T>(it->second.resource, it->second.refCount, this, name);
					}
					else
					{
						DEBUG_INFO(std::format("Loading resource '{}'", name).c_str());	
						T* newResource = LoadResource(name);
						if (newResource)
						{
							ResourceEntry entry(newResource);
							m_loadedResources.emplace(name, entry);
							return ResourcePtr<T>(entry.resource, entry.refCount, this, name);
						}
						else
						{
							DEBUG_INFO(std::format("Failed to load resource '{}'", name).c_str());
							return ResourcePtr<T>();
						}
					}
				}
				
				virtual bool EnsureResourceReady(const std::string& name)
				{
					auto it = m_loadedResources.find(name);
					if (it == m_loadedResources.end()) return false;
					
					T* resource = it->second.resource;
					if (!resource->isLoaded)
					{
						return resource->LoadFromDisk();
					}
					return true;
				}
				
				void OnResourceUnreferenced(const std::string& name)
				{
					auto it = m_loadedResources.find(name);
					if (it != m_loadedResources.end())
					{
						DEBUG_INFO(std::format("Unloading unreferenced resource '{}'", name).c_str());
						UnloadResource(name, it->second.resource);
						delete it->second.refCount;
						m_loadedResources.erase(it);
					}
				}
				
				size_t GetLoadedCount() const { return m_loadedResources.size(); }
				
				size_t GetTotalMemoryUsage() const
				{
					size_t total = 0;
					for (const auto& pair : m_loadedResources)
					{
						total += pair.second.resource->getMemoryUsage();
					}
					return total;
				}
			};

			// GPU-specific resource pool
			template<typename T>
			class GpuResourcePool : public ResourcePool<T>
			{
			static_assert(std::is_base_of_v<GpuResource, T>, "GpuResourcePool requires GpuResource-derived types");
			private:
				std::vector<T*> m_gpuResources;
				size_t m_maxGpuMemory;
				size_t m_currentGpuMemory;

			protected:
				virtual void UnloadResource(const std::string& filePath, T* resource)
				{
					RemoveGpuResource(resource);
					delete resource;
				}

			public:
				GpuResourcePool(size_t maxGpuMemory = 512 * 1024 * 1024) // Default 512MB
					: m_maxGpuMemory(maxGpuMemory), m_currentGpuMemory(0) {}
					
				~GpuResourcePool() override
				{
					m_gpuResources.clear(); // Base class handles resource cleanup
				}
				
				bool EnsureResourceReady(const std::string& name) override
				{
					auto it = this->m_loadedResources.find(name);
					if (it == this->m_loadedResources.end()) return false;
					
					T* resource = it->second.resource;
					
					// First ensure disk loading
					if (!resource->isLoaded && !resource->LoadFromDisk())
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
						if (resource->LoadToGpu())
						{
							m_currentGpuMemory += requiredMemory;
							resource->gpuState = GpuResource::GpuState::LoadedToGpu;
							DEBUG_INFO(std::format("Loaded '{}' to GPU ({} bytes)", name, requiredMemory).c_str());
							return true;
						}
						else
						{
							BREAK_MSG(std::format("Failed to load '{}' to GPU", name).c_str());
							return false;
						}
					}
					
					return false;
				}
				
				void AddGpuResource(T* resource)
				{
					m_gpuResources.push_back(resource);
				}
				
				void RemoveGpuResource(T* resource)
				{
					auto it = std::find(m_gpuResources.begin(), m_gpuResources.end(), resource);
					if (it != m_gpuResources.end())
					{
						m_gpuResources.erase(it);
					}
					
					if (resource->gpuState == GpuResource::GpuState::LoadedToGpu)
					{
						m_currentGpuMemory -= resource->GetGpuMemoryUsage();
					}
				}
				
				void FreeUnusedGpuResources(size_t neededMemory)
				{
					DEBUG_INFO(std::format("Freeing GPU memory (need {} bytes)...", neededMemory).c_str());
					
					size_t freedMemory = 0;
					
					for (T* resource : m_gpuResources)
					{
						if (freedMemory >= neededMemory) break;
						
						auto resourceIt = std::find_if(this->m_loadedResources.begin(), this->m_loadedResources.end(),
							[resource](const auto& pair) { return pair.second.resource == resource; });
						
						if (resourceIt != this->m_loadedResources.end() && *(resourceIt->second.refCount) == 1)
						{
							if (resource->gpuState == GpuResource::GpuState::LoadedToGpu)
							{
								size_t memoryFreed = resource->GetGpuMemoryUsage();
								resource->UnloadFromGpu();
								resource->gpuState = GpuResource::GpuState::Unloaded;
								m_currentGpuMemory -= memoryFreed;
								freedMemory += memoryFreed;
								DEBUG_INFO(std::format("Freed '{}' from GPU ({} bytes)", resource->filePath, memoryFreed).c_str());
							}
						}
					}

					DEBUG_INFO(std::format("Freed '{}' bytes from GPU", freedMemory).c_str());
				}
				
				void HandleGpuContextLost()
				{
					DEBUG_INFO("GPU context lost! Marking all GPU resources for reload...");
					
					for (T* resource : m_gpuResources)
					{
						if (resource->gpuState == GpuResource::GpuState::LoadedToGpu)
						{
							resource->gpuState = GpuResource::GpuState::GpuLost;
						}
					}
					m_currentGpuMemory = 0;
				}
				
				void ReloadAllGpuResources()
				{
					DEBUG_INFO("Reloading all GPU resources after context restore...");
					
					for (T* resource : m_gpuResources)
					{
						if (resource->gpuState == GpuResource::GpuState::GpuLost)
						{
							if (resource->LoadToGpu())
							{
								resource->gpuState = GpuResource::GpuState::LoadedToGpu;
								m_currentGpuMemory += resource->GetGpuMemoryUsage();
								DEBUG_INFO(std::format("Reloaded '{}' to GPU", resource->filePath).c_str());
							}
						}
					}
				}
				
				size_t GetGpuMemoryUsage() const { return m_currentGpuMemory; }
				size_t GetMaxGpuMemory() const { return m_maxGpuMemory; }
				float GetGpuMemoryUsagePercent() const
				{ 
					return m_maxGpuMemory > 0 ? (float)m_currentGpuMemory / m_maxGpuMemory * 100.0f : 0.0f; 
				}
			};
		}
	}
}
