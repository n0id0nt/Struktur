#include "SoundResource.h"

#include <format>
#include "Engine/Core/FileSystem.h"

Struktur::Resource::SoundResource::SoundResource(const std::string& filePath)
	: CpuResource(filePath)
{
	sound.frameCount = 0;
	m_waveData.frameCount = 0;
}

Struktur::Resource::SoundResource::~SoundResource()
{
	UnloadFromHardware();
	UnloadFromDisk();
}

bool Struktur::Resource::SoundResource::LoadFromDisk()
{
	if (isLoaded) return true;

	auto result = FileSystem::ReadBytes(filePath);
	ASSERT_MSG(result.success, "Failed to load config: %s", result.errorMessage.c_str());
	const char* ext = ::GetFileExtension(filePath.c_str());

	m_waveData = ::LoadWaveFromMemory(ext, result.value.data(), result.value.size());
	if (m_waveData.frameCount == 0) return false;

	isLoaded = true;
	DEBUG_INFO(std::format("Loaded sound from disk: {}", filePath).c_str());
	return true;
}

void Struktur::Resource::SoundResource::UnloadFromDisk()
{
	if (m_waveData.frameCount > 0)
	{
		::UnloadWave(m_waveData);
		m_waveData.frameCount = 0;
	}
	isLoaded = false;
}

bool Struktur::Resource::SoundResource::LoadToHardware()
{
	if (!LoadFromDisk()) return false;
	if (sound.frameCount > 0) return true; // Already loaded

	sound = ::LoadSoundFromWave(m_waveData);
	DEBUG_INFO(std::format("Loaded sound to audio hardware: {}", filePath).c_str());
	return sound.frameCount > 0;
}

void Struktur::Resource::SoundResource::UnloadFromHardware()
{
	if (sound.frameCount > 0)
	{
		::UnloadSound(sound);
		sound.frameCount = 0;
		DEBUG_INFO(std::format("Unloaded sound from audio hardware: {}", filePath).c_str());
	}
}

bool Struktur::Resource::SoundResource::IsHardwareReady() const
{
	return sound.frameCount > 0/* && IsSoundReady(sound)*/;
}

size_t Struktur::Resource::SoundResource::GetMemoryUsage() const
{
	return isLoaded ? (m_waveData.frameCount * m_waveData.channels * (m_waveData.sampleSize / 8)) : 0;
}

Struktur::Resource::SoundResource* Struktur::Resource::SoundPool::LoadResource(const std::string& filePath)
{
	auto* sound = new SoundResource(filePath);

	if (!sound->LoadFromDisk())
	{
		delete sound;
		return nullptr;
	}

	return sound;
}

bool Struktur::Resource::SoundPool::EnsureResourceReady(const std::string& filePath)
{
	auto it = m_loadedResources.find(filePath);
	if (it == m_loadedResources.end()) return false;

	SoundResource* sound = it->second.resource;

	// Load from disk first
	if (!sound->isLoaded && !sound->LoadFromDisk())
	{
		return false;
	}

	// Then load to audio hardware
	return sound->LoadToHardware();
}
