#include "FontPool.h"

Struktur::Resource::FontResource* Struktur::Resource::FontPool::LoadResource(GameContext& context,
                                                                             const std::string& resourceString)
{
	std::string filePath;
	int fontSize         = defaultFontSize;
	size_t underscorePos = resourceString.find_last_of('_');
	if (underscorePos != std::string::npos)
	{
		// Parse the number after the underscore
		int specifiedSize = std::stoi(resourceString.substr(underscorePos + 1));
		if (specifiedSize > 0 && specifiedSize <= 256)
		{  // Reasonable size limits
			fontSize = specifiedSize;

			// Remove size from path - get base name before underscore
			filePath = resourceString.substr(0, underscorePos);
		}
	}

	auto* font = new FontResource(filePath, fontSize);

	if (!font->LoadFromDisk(context))
	{
		delete font;
		return nullptr;
	}

	AddGpuResource(font);
	return font;
}
