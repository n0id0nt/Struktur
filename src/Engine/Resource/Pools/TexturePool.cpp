#include "TexturePool.h"

#include <algorithm>

#include "Debug/Assertions.h"
#include "Engine/GameContext.h"
#include "Engine/Jobs/JobSystem.h"

void Struktur::Resource::TexturePool::AddDecoded(const std::string& filePath, TextureResource::DecodedImage&& image)
{
	if (m_nameToHandle.count(filePath) == 0 && m_prefetched.count(filePath) == 0)
	{
		m_prefetched.emplace(filePath, std::move(image));
	}
}

void Struktur::Resource::TexturePool::Prefetch(GameContext& context, const std::vector<std::string>& filePaths)
{
	std::vector<std::string> wanted;
	for (const std::string& path : filePaths)
	{
		if (m_nameToHandle.count(path) == 0 && m_prefetched.count(path) == 0 &&
		    std::find(wanted.begin(), wanted.end(), path) == wanted.end())
		{
			wanted.push_back(path);
		}
	}
	if (wanted.empty())
	{
		return;
	}
	std::vector<TextureResource::DecodedImage> images(wanted.size());
	std::vector<std::string> errors(wanted.size());
	std::vector<char> decoded(wanted.size(), 0);
	Jobs::JobSystem& jobs = context.GetJobSystem();
	jobs.Wait(jobs.ParallelFor(0, wanted.size(), 1,
	                           [&wanted, &images, &errors, &decoded](size_t first, size_t last)
	                           {
		                           for (size_t index = first; index < last; ++index)
		                           {
			                           decoded[index] = TextureResource::Decode(wanted[index], images[index], errors[index]);
		                           }
	                           }));
	for (size_t index = 0; index < wanted.size(); ++index)
	{
		if (decoded[index])
		{
			m_prefetched.emplace(wanted[index], std::move(images[index]));
		}
		else
		{
			// Left for GetResource() to load (and report) the usual way.
			DEBUG_WARNING("Texture prefetch failed: %s (%s)", wanted[index], errors[index].c_str());
		}
	}
	DEBUG_INFO("Prefetched %d textures on %d workers", (int)wanted.size(), jobs.GetWorkerCount());
}

Struktur::Resource::TextureResource* Struktur::Resource::TexturePool::LoadResource(GameContext& context,
                                                                                   const std::string& filePath)
{
	auto* texture = new TextureResource(filePath);

	auto prefetched = m_prefetched.find(filePath);
	if (prefetched != m_prefetched.end())
	{
		texture->SetDecoded(std::move(prefetched->second));
		m_prefetched.erase(prefetched);
		DEBUG_INFO("Loaded texture from prefetch: %s (%dx%d)", filePath, texture->GetWidth(), texture->GetHeight());
		return texture;
	}

	if (!texture->LoadFromDisk(context))
	{
		delete texture;
		return nullptr;
	}

	return texture;
}
