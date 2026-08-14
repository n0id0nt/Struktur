#include "ShaderPool.h"

Struktur::Resource::ShaderResource* Struktur::Resource::ShaderPool::LoadResource(GameContext& context,
                                                                                 const std::string& resourceString)
{
	std::string vsFilePath;
	std::string fsFilePath;
	size_t commerPos = resourceString.find_last_of(',');
	if (commerPos != std::string::npos)
	{
		vsFilePath = resourceString.substr(0, commerPos);
		fsFilePath = resourceString.substr(commerPos + 1);
	}

	auto* shader = new ShaderResource(vsFilePath, fsFilePath);

	if (!shader->LoadFromDisk(context))
	{
		delete shader;
		return nullptr;
	}

	return shader;
}
