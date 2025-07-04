#include "ResourcePool.h"

Struktur::Core::ResourcePool::ResourcePool() : m_images()
{
}

Struktur::Core::ResourcePool::~ResourcePool()
{
	Clear();
}

void Struktur::Core::ResourcePool::CreateTexture(const std::string& path)
{
	std::string key = path;
	auto it = m_images.find(key);
	if (it == m_images.end())
	{
		m_images[key] = RefGPU<Image, Texture2D>{ LoadImage(path.c_str()), Texture2D{}, false };
	}
	else
	{
		assert(false); // image already exists
	}
}

bool Struktur::Core::ResourcePool::IsTextureLoadedInGPU(const std::string& path) const
{
	std::string key = path;
	auto it = m_images.find(key);
	if (it == m_images.end())
	{
		assert(true); // texture does not exist
		return false;
	}
	return it->second.loadedInGPU;
}

void Struktur::Core::ResourcePool::LoadTextureInGPU(const std::string& path)
{
	std::string key = path;
	auto it = m_images.find(key);
	if (it == m_images.end())
	{
		assert(true); // texture does not exist
		return;
	}
	if (it->second.loadedInGPU)
	{
		assert(true); // texture is already loaded
		return;
	}
	Image& image = it->second.referenceRAM;
	Texture2D texture = LoadTextureFromImage(image);
	//TODO get the success of the load somehow, and assert when unsuccessful
	it->second.referenceVRAM = texture;
	it->second.loadedInGPU = true;
}

Texture2D Struktur::Core::ResourcePool::RetrieveTexture(const std::string& path) const
{
	std::string key = path;
	auto it = m_images.find(key);
	if (it == m_images.end())
	{
		assert(true); // texture does not exist
		Texture2D empty{};
		return empty;
	}
	if (!it->second.loadedInGPU)
	{
		assert(true); // texture is not loaded
	}
	return it->second.referenceVRAM;
}

Image Struktur::Core::ResourcePool::RetrieveImage(const std::string& path) const
{
	std::string key = path;
	auto it = m_images.find(key);
	if (it == m_images.end())
	{
		assert(true); // texture does not exist
		Image empty{};
		return empty;
	}
	return it->second.referenceRAM;
}

void Struktur::Core::ResourcePool::UnloadTextureGPU(const std::string& path)
{
	std::string key = path;
	auto it = m_images.find(key);
	if (it == m_images.end())
	{
		assert(true); // texture does not exist
		return;
	}
	if (!it->second.loadedInGPU)
	{
		assert(true); // texture is not loaded
		return;
	}
	Texture2D& texture = it->second.referenceVRAM;

	UnloadTexture(texture);
	//TODO get the success of the unload somehow, and assert when unsuccessful
	it->second.referenceVRAM = Texture2D{};
	it->second.loadedInGPU = false;
}

void Struktur::Core::ResourcePool::ReleaseTexture(const std::string& path)
{
	std::string key = path;
	auto it = m_images.find(key);
	if (it == m_images.end())
	{
		assert(true); // texture does not exist
		return;
	}
	if (IsTextureLoadedInGPU(path))
	{
		UnloadTextureGPU(path);
	}
	Image& image = it->second.referenceRAM;
	UnloadImage(image);
	m_images.erase(it);
}

void Struktur::Core::ResourcePool::CreateSound(const std::string& path)
{
	std::string key = path;
	auto it = m_sounds.find(key);
	if (it == m_sounds.end())
	{
		m_sounds[key] = LoadSound(path.c_str());
	}
	else
	{
		assert(false); // sound already exists
	}
}

Sound Struktur::Core::ResourcePool::RetrieveSound(const std::string& path)
{
	std::string key = path;
	auto it = m_sounds.find(key);
	if (it == m_sounds.end())
	{
		assert(true); // texture does not exist
		Sound empty{};
		return empty;
	}
	return it->second;
}

void Struktur::Core::ResourcePool::ReleaseSound(const std::string& path)
{
	std::string key = path;
	auto it = m_sounds.find(key);
	if (it == m_sounds.end())
	{
		assert(true); // sound does not exist
		return;
	}
	Sound& sound = it->second;
	UnloadSound(sound);
	m_sounds.erase(it);
}

void Struktur::Core::ResourcePool::Clear()
{
	for (auto it = m_images.begin(); it != m_images.end(); it++)
	{
		if (IsTextureLoadedInGPU(it->first))
		{
			UnloadTextureGPU(it->first);
		}
		Image& image = it->second.referenceRAM;
		UnloadImage(image);
	}
	m_images.clear();
	for (auto it = m_sounds.begin(); it != m_sounds.end(); it++)
	{
		Sound& sound = it->second;
		UnloadSound(sound);
	}
	m_sounds.clear();
}
