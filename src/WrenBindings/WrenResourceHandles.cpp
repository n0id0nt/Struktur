#include "WrenResourceHandles.h"

#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"

// ============================================================================
// Texture Resource Handle
// ============================================================================

void wren_TextureAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "resourceManager", "Texture", 1);  // Get class into slot 1
	WrenTextureHandle* handle = (WrenTextureHandle*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenTextureHandle));
	new (handle) WrenTextureHandle();
}

void wren_TextureFinalize(void* data)
{
	WrenTextureHandle* handle = (WrenTextureHandle*)data;
	handle->~WrenTextureHandle();
}

// Texture.load(path) -> TextureHandle
void wren_TextureLoad(WrenVM* vm)
{
	Struktur::resourceManagerContext* context = static_cast<Struktur::resourceManagerContext*>(wrenGetUserData(vm));
	auto& resourceManager = context->GetResourceManager();

	const char* path = wrenGetSlotString(vm, 1);

	// Load texture through resource manager
	Struktur::Resource::ResourcePtr<Struktur::Resource::TextureResource> texture = resourceManager.GetTexture(path);

	if (!texture)
	{
		DEBUG_ERROR("Failed to load texture: %s", path);
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create foreign object with resource pointer
	wrenGetVariable(vm, "resourceManager", "Texture", 1);  // Get class into slot 1
	WrenTextureHandle* handle = (WrenTextureHandle*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenTextureHandle));
	new (handle) WrenTextureHandle(texture);
}

// texture.isValid -> Bool
void wren_TextureIsValid(WrenVM* vm)
{
	WrenTextureHandle* handle = (WrenTextureHandle*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, handle->resource.IsValid());
}

// texture.path -> String
void wren_TextureGetPath(WrenVM* vm)
{
	WrenTextureHandle* handle = (WrenTextureHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotString(vm, 0, handle->resource.GetFilePath().c_str());
}

// texture.width -> Num
void wren_TextureGetWidth(WrenVM* vm)
{
	WrenTextureHandle* handle = (WrenTextureHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotDouble(vm, 0, 0);
		return;
	}

	wrenSetSlotDouble(vm, 0, handle->resource->GetWidth());
}

// texture.height -> Num
void wren_TextureGetHeight(WrenVM* vm)
{
	WrenTextureHandle* handle = (WrenTextureHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotDouble(vm, 0, 0);
		return;
	}

	wrenSetSlotDouble(vm, 0, handle->resource->GetHeight());
}

// texture.toString -> String
void wren_TextureToString(WrenVM* vm)
{
	WrenTextureHandle* handle = (WrenTextureHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotString(vm, 0, "Texture(invalid)");
		return;
	}

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "Texture(%s, %dx%d)",
		handle->resource.GetFilePath().c_str(),
		handle->resource->GetWidth(),
		handle->resource->GetHeight());

	wrenSetSlotString(vm, 0, buffer);
}

// Register Texture foreign class
WREN_FOREIGN_CLASS("resourceManager", "Texture", wren_TextureAllocate, wren_TextureFinalize, "Texture resource handle");

WREN_CLASS_STATIC("resourceManager", "Texture", "load(_)", wren_TextureLoad, "Load texture from path");
WREN_CLASS_METHOD("resourceManager", "Texture", "isValid()", wren_TextureIsValid, "Check if texture is valid");
WREN_CLASS_METHOD("resourceManager", "Texture", "path", wren_TextureGetPath, "Get texture path");
WREN_CLASS_METHOD("resourceManager", "Texture", "width", wren_TextureGetWidth, "Get texture width");
WREN_CLASS_METHOD("resourceManager", "Texture", "height", wren_TextureGetHeight, "Get texture height");
WREN_CLASS_METHOD("resourceManager", "Texture", "toString()", wren_TextureToString, "Convert to string");

// ============================================================================
// Font Resource Handle
// ============================================================================

void wren_FontAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "resourceManager", "Font", 1);  // Get class into slot 1
	WrenFontHandle* handle = (WrenFontHandle*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenFontHandle));
	new (handle) WrenFontHandle();
}

void wren_FontFinalize(void* data)
{
	WrenFontHandle* handle = (WrenFontHandle*)data;
	handle->~WrenFontHandle();
}

// Font.load(path) -> FontHandle
void wren_FontLoad(WrenVM* vm)
{
	Struktur::resourceManagerContext* context = static_cast<Struktur::resourceManagerContext*>(wrenGetUserData(vm));
	auto& resourceManager = context->GetResourceManager();

	const char* path = wrenGetSlotString(vm, 1);

	// Load texture through resource manager
	Struktur::Resource::ResourcePtr<Struktur::Resource::FontResource> texture = resourceManager.GetFontResource(path);

	if (!texture)
	{
		DEBUG_ERROR("Failed to load texture: %s", path);
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create foreign object with resource pointer
	wrenGetVariable(vm, "resourceManager", "Font", 1);  // Get class into slot 1
	WrenFontHandle* handle = (WrenFontHandle*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenFontHandle));
	new (handle) WrenFontHandle(texture);
}

// font.isValid -> Bool
void wren_FontIsValid(WrenVM* vm)
{
	WrenFontHandle* handle = (WrenFontHandle*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, handle->resource.IsValid());
}

// font.path -> String
void wren_FontGetPath(WrenVM* vm)
{
	WrenFontHandle* handle = (WrenFontHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotString(vm, 0, handle->resource.GetFilePath().c_str());
}

// font.size -> Num
void wren_FontGetSize(WrenVM* vm)
{
	WrenFontHandle* handle = (WrenFontHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotDouble(vm, 0, 0);
		return;
	}

	wrenSetSlotDouble(vm, 0, handle->resource->GetFontSize());
}

// font.toString -> String
void wren_FontToString(WrenVM* vm)
{
	WrenFontHandle* handle = (WrenFontHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotString(vm, 0, "Font(invalid)");
		return;
	}

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "Font(%s, %d)",
		handle->resource.GetFilePath().c_str(),
		handle->resource->GetFontSize());

	wrenSetSlotString(vm, 0, buffer);
}

// Register Font foreign class
WREN_FOREIGN_CLASS("resourceManager", "Font", wren_FontAllocate, wren_FontFinalize, "Font resource handle");

WREN_CLASS_STATIC("resourceManager", "Font", "load(_)", wren_FontLoad, "Load font from path");
WREN_CLASS_METHOD("resourceManager", "Font", "isValid()", wren_FontIsValid, "Check if font is valid");
WREN_CLASS_METHOD("resourceManager", "Font", "path", wren_FontGetPath, "Get font path");
WREN_CLASS_METHOD("resourceManager", "Font", "size", wren_FontGetSize, "Get font size");
WREN_CLASS_METHOD("resourceManager", "Font", "toString()", wren_FontToString, "Convert to string");


// ============================================================================
// Music Resource Handle
// ============================================================================

void wren_MusicAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "resourceManager", "Music", 1);  // Get class into slot 1
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMusicHandle));
	new (handle) WrenMusicHandle();
}

void wren_MusicFinalize(void* data)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)data;
	handle->~WrenMusicHandle();
}

// Music.load(path) -> MusicHandle
void wren_MusicLoad(WrenVM* vm)
{
	Struktur::resourceManagerContext* context = static_cast<Struktur::resourceManagerContext*>(wrenGetUserData(vm));
	auto& resourceManager = context->GetResourceManager();

	const char* path = wrenGetSlotString(vm, 1);

	// Load music through resource manager
	Struktur::Resource::ResourcePtr<Struktur::Resource::MusicResource> music = resourceManager.GetMusic(path);

	if (!music)
	{
		DEBUG_ERROR("Failed to load music: %s", path);
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create foreign object with resource pointer
	wrenGetVariable(vm, "resourceManager", "Music", 1);  // Get class into slot 1
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenMusicHandle));
	new (handle) WrenMusicHandle(music);
}

// Music.isValid -> Bool
void wren_MusicIsValid(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, handle->resource.IsValid());
}

// Music.path -> String
void wren_MusicGetPath(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotString(vm, 0, handle->resource.GetFilePath().c_str());
}

// Music.toString -> String
void wren_MusicToString(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotString(vm, 0, "Music(invalid)");
		return;
	}

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "Music(%s)",
		handle->resource.GetFilePath().c_str());

	wrenSetSlotString(vm, 0, buffer);
}

// Register Music foreign class
WREN_FOREIGN_CLASS("resourceManager", "Music", wren_MusicAllocate, wren_FontFinalize, "Font resource handle");

WREN_CLASS_STATIC("resourceManager", "Music", "load(_)", wren_MusicLoad, "Load music from path");
WREN_CLASS_METHOD("resourceManager", "Music", "isValid()", wren_MusicIsValid, "Check if music is valid");
WREN_CLASS_METHOD("resourceManager", "Music", "path", wren_MusicGetPath, "Get music path");
WREN_CLASS_METHOD("resourceManager", "Music", "toString()", wren_MusicToString, "Convert to string");

// ============================================================================
// Sound Resource Handle
// ============================================================================

void wren_SoundAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "resourceManager", "Sound", 1);  // Get class into slot 1
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenSoundHandle));
	new (handle) WrenSoundHandle();
}

void wren_SoundFinalize(void* data)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)data;
	handle->~WrenSoundHandle();
}

// Sound.load(path) -> SoundHandle
void wren_SoundLoad(WrenVM* vm)
{
	Struktur::resourceManagerContext* context = static_cast<Struktur::resourceManagerContext*>(wrenGetUserData(vm));
	auto& resourceManager = context->GetResourceManager();

	const char* path = wrenGetSlotString(vm, 1);

	// Load sound through resource manager
	Struktur::Resource::ResourcePtr<Struktur::Resource::SoundResource> sound = resourceManager.GetSound(path);

	if (!sound)
	{
		DEBUG_ERROR("Failed to load sound: %s", path);
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create foreign object with resource pointer
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenSoundHandle));
	new (handle) WrenSoundHandle(sound);
}

// Sound.isValid -> Bool
void wren_SoundIsValid(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, handle->resource.IsValid());
}

// Sound.path -> String
void wren_SoundGetPath(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotString(vm, 0, handle->resource.GetFilePath().c_str());
}

// Sound.toString -> String
void wren_SoundToString(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotString(vm, 0, "Sound(invalid)");
		return;
	}

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "Sound(%s)",
		handle->resource.GetFilePath().c_str());

	wrenSetSlotString(vm, 0, buffer);
}

// Register Sound foreign class
WREN_FOREIGN_CLASS("resourceManager", "Sound", wren_SoundAllocate, wren_FontFinalize, "Font resource handle");

WREN_CLASS_STATIC("resourceManager", "Sound", "load(_)", wren_SoundLoad, "Load sound from path");
WREN_CLASS_METHOD("resourceManager", "Sound", "isValid()", wren_SoundIsValid, "Check if sound is valid");
WREN_CLASS_METHOD("resourceManager", "Sound", "path", wren_SoundGetPath, "Get sound path");
WREN_CLASS_METHOD("resourceManager", "Sound", "toString()", wren_SoundToString, "Convert to string");
