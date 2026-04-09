#include "MusicResource.h"

#include "Engine/Core/FileSystem.h"

Struktur::Resource::MusicResource::MusicResource(const std::string& filePath)
	: CpuResource(filePath)
{
	music.frameCount = 0;
}

Struktur::Resource::MusicResource::~MusicResource()
{
	UnloadFromHardware();
	UnloadFromDisk();
}

bool Struktur::Resource::MusicResource::LoadFromDisk()
{
	if (isLoaded) return true;

	auto result = FileSystem::ReadBytes(filePath);
	ASSERT_MSG(result.success, "Failed to load config: %s", result.errorMessage.c_str());
	buffer = std::move(result.value);
	const char* ext = ::GetFileExtension(filePath.c_str());
	music = ::LoadMusicStreamFromMemory(ext, buffer.data(), buffer.size());

	if (music.frameCount == 0) return false;

	isLoaded = true;
	DEBUG_INFO(std::format("Loaded music stream: {}", filePath).c_str());
	return true;
}

void Struktur::Resource::MusicResource::UnloadFromDisk()
{
	if (music.frameCount > 0)
	{
		UnloadMusicStream(music);
		music.frameCount = 0;
	}
	isLoaded = false;
}

bool Struktur::Resource::MusicResource::LoadToHardware()
{
	return LoadFromDisk(); // Music streams don't need separate hardware loading
}

bool Struktur::Resource::MusicResource::IsHardwareReady() const
{
	return music.frameCount > 0/* && IsMusicReady(music)*/;
}

size_t Struktur::Resource::MusicResource::GetMemoryUsage() const
{
	return isLoaded ? 1024 * 1024 : 0; // Rough estimate for streaming buffer
}

Struktur::Resource::MusicResource* Struktur::Resource::MusicPool::LoadResource(const std::string& filePath)
{
	auto* music = new MusicResource(filePath);

	if (!music->LoadFromDisk())
	{
		delete music;
		return nullptr;
	}

	return music;
}
