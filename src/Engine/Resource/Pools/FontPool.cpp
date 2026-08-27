#include "FontPool.h"

Struktur::Resource::FontResource* Struktur::Resource::FontPool::LoadResource(GameContext& context,
                                                                             const FontKey& key)
{
	auto* font = new FontResource(key.path, key.size);

	if (!font->LoadFromDisk(context))
	{
		delete font;
		return nullptr;
	}

	return font;
}
