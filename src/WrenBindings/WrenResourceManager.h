#include "Engine/Resource/TextureResource.h"
#include "Engine/Resource/ShaderResource.h"
#include "Engine/Resource/FontResource.h"
#include "Engine/Resource/MusicResource.h"
#include "Engine/Resource/SoundResource.h"

struct WrenTextureHandle
{
	Struktur::Resource::ResourcePtr<Struktur::Resource::TextureResource> resource;

	WrenTextureHandle() {}
	WrenTextureHandle(const Struktur::Resource::ResourcePtr<Struktur::Resource::TextureResource>& res) : resource(res) {}
};

struct WrenShaderHandle
{
	Struktur::Resource::ResourcePtr<Struktur::Resource::ShaderResource> resource;

	WrenShaderHandle() {}
	WrenShaderHandle(const Struktur::Resource::ResourcePtr<Struktur::Resource::ShaderResource>& res) : resource(res) {}
};

struct WrenFontHandle
{
	Struktur::Resource::ResourcePtr<Struktur::Resource::FontResource> resource;

	WrenFontHandle() {}
	WrenFontHandle(const Struktur::Resource::ResourcePtr<Struktur::Resource::FontResource>& res) : resource(res) {}
};

struct WrenMusicHandle
{
	Struktur::Resource::ResourcePtr<Struktur::Resource::MusicResource> resource;

	WrenMusicHandle() {}
	WrenMusicHandle(const Struktur::Resource::ResourcePtr<Struktur::Resource::MusicResource>& res) : resource(res) {}
};

struct WrenSoundHandle
{
	Struktur::Resource::ResourcePtr<Struktur::Resource::SoundResource> resource;

	WrenSoundHandle() {}
	WrenSoundHandle(const Struktur::Resource::ResourcePtr<Struktur::Resource::SoundResource>& res) : resource(res) {}
};
