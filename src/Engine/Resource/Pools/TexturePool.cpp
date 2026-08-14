#include "TexturePool.h"

Struktur::Resource::TextureResource* Struktur::Resource::TexturePool::LoadResource(GameContext& context,
                                                                                   const std::string& filePath)
{
	auto* texture = new TextureResource(filePath);

	if (!texture->LoadFromDisk(context))
	{
		delete texture;
		return nullptr;
	}

	AddGpuResource(texture);
	return texture;
}
