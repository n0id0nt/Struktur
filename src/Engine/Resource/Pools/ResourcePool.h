#pragma once

#include <format>
#include <string>
#include <unordered_map>

#include "Debug/Assertions.h"
#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/Resource.h"

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Resource
{
// Base resource pool
template <typename T>
class ResourcePool
{
   protected:
	// TODO might be a good idea to make these not pointers so the memory is stores in series and pass around the
	// pointers to these items
	struct ResourceEntry
	{
		T* resource      = nullptr;
		size_t* refCount = nullptr;

		ResourceEntry(T* res)
		    : resource(res),
		      refCount(new size_t(1))
		{
		}
	};

	std::unordered_map<std::string, ResourceEntry> m_loadedResources;

	virtual T* LoadResource(GameContext& context, const std::string& filePath) = 0;
	virtual void UnloadResource(const std::string& filePath, T* resource)
	{
		delete resource;
	}

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

	ResourcePtr<T> GetResource(GameContext& context, const std::string& name)
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
			T* newResource = LoadResource(context, name);
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

	virtual bool EnsureResourceReady(GameContext& context, const std::string& name)
	{
		auto it = m_loadedResources.find(name);
		if (it == m_loadedResources.end())
		{
			return false;
		}

		T* resource = it->second.resource;
		if (!resource->isLoaded)
		{
			return resource->LoadFromDisk(context);
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

	size_t GetLoadedCount() const
	{
		return m_loadedResources.size();
	}

	size_t GetTotalMemoryUsage() const
	{
		size_t total = 0;
		for (const auto& pair : m_loadedResources)
		{
			total += pair.second.resource->GetMemoryUsage();
		}
		return total;
	}

	class Iterator
	{
	   private:
		typename std::unordered_map<std::string, ResourceEntry>::iterator m_it;

	   public:
		using iterator_category = std::forward_iterator_tag;
		using value_type        = std::pair<const std::string&, T*>;
		using difference_type   = std::ptrdiff_t;
		using pointer           = value_type*;
		using reference         = value_type;

		Iterator(typename std::unordered_map<std::string, ResourceEntry>::iterator it)
		    : m_it(it)
		{
		}

		std::pair<const std::string&, T*> operator*() const
		{
			return {m_it->first, m_it->second.resource};
		}

		Iterator& operator++()
		{
			++m_it;
			return *this;
		}
		Iterator operator++(int)
		{
			Iterator tmp = *this;
			++(*this);
			return tmp;
		}
		bool operator==(const Iterator& other) const
		{
			return m_it == other.m_it;
		}
		bool operator!=(const Iterator& other) const
		{
			return m_it != other.m_it;
		}
	};

	// Const iterator
	class ConstIterator
	{
	   private:
		typename std::unordered_map<std::string, ResourceEntry>::const_iterator m_it;

	   public:
		using iterator_category = std::forward_iterator_tag;
		using value_type        = std::pair<const std::string&, const T*>;
		using difference_type   = std::ptrdiff_t;
		using pointer           = value_type*;
		using reference         = value_type;

		ConstIterator(typename std::unordered_map<std::string, ResourceEntry>::const_iterator it)
		    : m_it(it)
		{
		}

		std::pair<const std::string&, const T*> operator*() const
		{
			return {m_it->first, m_it->second.resource};
		}

		ConstIterator& operator++()
		{
			++m_it;
			return *this;
		}
		ConstIterator operator++(int)
		{
			ConstIterator tmp = *this;
			++(*this);
			return tmp;
		}
		bool operator==(const ConstIterator& other) const
		{
			return m_it == other.m_it;
		}
		bool operator!=(const ConstIterator& other) const
		{
			return m_it != other.m_it;
		}
	};

	Iterator begin()
	{
		return Iterator(m_loadedResources.begin());
	}
	Iterator end()
	{
		return Iterator(m_loadedResources.end());
	}
	ConstIterator begin() const
	{
		return ConstIterator(m_loadedResources.begin());
	}
	ConstIterator end() const
	{
		return ConstIterator(m_loadedResources.end());
	}
	ConstIterator cbegin() const
	{
		return ConstIterator(m_loadedResources.cbegin());
	}
	ConstIterator cend() const
	{
		return ConstIterator(m_loadedResources.cend());
	}
};
}  // namespace Resource
}  // namespace Struktur
