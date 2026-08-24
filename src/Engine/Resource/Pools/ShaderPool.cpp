#include "ShaderPool.h"

Struktur::Resource::ShaderResource* Struktur::Resource::ShaderPool::LoadResource(GameContext& context,
                                                                                 const ShaderKey& key)
{
	auto* shader = new ShaderResource(key.vsFilePath, key.fsFilePath);

	if (!shader->LoadFromDisk(context))
	{
		delete shader;
		return nullptr;
	}

	return shader;
}
