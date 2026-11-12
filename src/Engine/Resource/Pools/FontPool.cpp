#include "FontPool.h"

Struktur::Resource::FontResource* Struktur::Resource::FontPool::LoadResource(GameContext& context,
                                                                             const FontKey& key)
{
	auto* font = new FontResource(key.path, key.size, key.pointSampled);

	if (!font->LoadFromDisk(context))
	{
		delete font;
		return nullptr;
	}

	return font;
}
