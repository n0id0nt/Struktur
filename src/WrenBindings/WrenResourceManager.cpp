#include "WrenResourceManager.h"

#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "wren.hpp"

// ============================================================================
// Texture Resource Handle
// ============================================================================

void wren_ResourceTextureAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenTextureHandle));
}

void wren_ResourceTextureFinalize(void* data)
{
	WrenTextureHandle* handle = (WrenTextureHandle*)data;
	handle->~WrenTextureHandle();
}

// Texture.load(path) -> TextureHandle
void wren_ResourceTextureLoad(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& resourceManager          = context->GetResourceManager();

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

// Texture.unload()
void wren_ResourceTextureUnload(WrenVM* vm)
{
	WrenTextureHandle* handle = (WrenTextureHandle*)wrenGetSlotForeign(vm, 0);
	handle->resource          = Struktur::Resource::ResourcePtr<Struktur::Resource::TextureResource>();
}

// texture.isValid -> Bool
void wren_ResourceTextureIsValid(WrenVM* vm)
{
	WrenTextureHandle* handle = (WrenTextureHandle*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, handle->resource.IsValid());
}

// texture.path -> String
void wren_ResourceTextureGetPath(WrenVM* vm)
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
void wren_ResourceTextureGetWidth(WrenVM* vm)
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
void wren_ResourceTextureGetHeight(WrenVM* vm)
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
void wren_ResourceTextureToString(WrenVM* vm)
{
	WrenTextureHandle* handle = (WrenTextureHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotString(vm, 0, "Texture(invalid)");
		return;
	}

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "Texture(%s, %dx%d)", handle->resource.GetFilePath().c_str(),
	         handle->resource->GetWidth(), handle->resource->GetHeight());

	wrenSetSlotString(vm, 0, buffer);
}

// ============================================================================
// Shader Resource Handle
// ============================================================================

void wren_ResourceShaderAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenShaderHandle));
}

void wren_ResourceShaderFinalize(void* data)
{
	WrenShaderHandle* handle = (WrenShaderHandle*)data;
	handle->~WrenShaderHandle();
}

// Shader.load(vsFilePath, fsFilePath) -> ShaderHandle
void wren_ResourceShaderLoad(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& resourceManager          = context->GetResourceManager();

	std::string vsFilePath = wrenGetSlotType(vm, 1) == WREN_TYPE_NULL ? std::string() : wrenGetSlotString(vm, 1);
	std::string fsFilePath = wrenGetSlotType(vm, 2) == WREN_TYPE_NULL ? std::string() : wrenGetSlotString(vm, 2);

	// Load Shader through resource manager
	Struktur::Resource::ResourcePtr<Struktur::Resource::ShaderResource> shader =
	    resourceManager.GetShader(vsFilePath, fsFilePath);

	if (!shader)
	{
		DEBUG_ERROR("Failed to load shader: %s, %s", vsFilePath, fsFilePath);
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create foreign object with resource pointer
	wrenGetVariable(vm, "resourceManager", "Shader", 1);  // Get class into slot 1
	WrenShaderHandle* handle = (WrenShaderHandle*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenShaderHandle));
	new (handle) WrenShaderHandle(shader);
}

// Shader.unload()
void wren_ResourceShaderUnload(WrenVM* vm)
{
	WrenShaderHandle* handle = (WrenShaderHandle*)wrenGetSlotForeign(vm, 0);
	handle->resource         = Struktur::Resource::ResourcePtr<Struktur::Resource::ShaderResource>();
}

// Shader.isValid -> Bool
void wren_ResourceShaderIsValid(WrenVM* vm)
{
	WrenShaderHandle* handle = (WrenShaderHandle*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, handle->resource.IsValid());
}

// Shader.path -> String
void wren_ResourceShaderGetPath(WrenVM* vm)
{
	WrenShaderHandle* handle = (WrenShaderHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotString(vm, 0, handle->resource.GetFilePath().c_str());
}

// Shader.vsFilePath -> string
void wren_ResourceShaderGetVSFilePath(WrenVM* vm)
{
	WrenShaderHandle* handle = (WrenShaderHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotString(vm, 0, handle->resource->GetVSFilePath().c_str());
}

// Shader.fsFilePath -> Num
void wren_ResourceShaderGetFSFilePath(WrenVM* vm)
{
	WrenShaderHandle* handle = (WrenShaderHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotString(vm, 0, handle->resource->GetFSFilePath().c_str());
}

// Shader.toString -> String
void wren_ResourceShaderToString(WrenVM* vm)
{
	WrenShaderHandle* handle = (WrenShaderHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotString(vm, 0, "Shader(invalid)");
		return;
	}

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "Shader(%s, %sx%s)", handle->resource.GetFilePath().c_str(),
	         handle->resource->GetVSFilePath().c_str(), handle->resource->GetFSFilePath().c_str());

	wrenSetSlotString(vm, 0, buffer);
}

// ============================================================================
// Font Resource Handle
// ============================================================================

void wren_ResourceFontAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenFontHandle));
}

void wren_ResourceFontFinalize(void* data)
{
	WrenFontHandle* handle = (WrenFontHandle*)data;
	handle->~WrenFontHandle();
}

// Font.load(path, size) -> FontHandle
void wren_ResourceFontLoad(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& resourceManager          = context->GetResourceManager();

	const char* path = wrenGetSlotString(vm, 1);
	int size         = static_cast<int>(wrenGetSlotDouble(vm, 2));

	// Load font through resource manager
	Struktur::Resource::ResourcePtr<Struktur::Resource::FontResource> font = resourceManager.GetFont(path, size);

	if (!font)
	{
		DEBUG_ERROR("Failed to load font: %s", path);
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create foreign object with resource pointer
	wrenGetVariable(vm, "resourceManager", "Font", 1);  // Get class into slot 1
	WrenFontHandle* handle = (WrenFontHandle*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenFontHandle));
	new (handle) WrenFontHandle(font);
}

// Font.unload()
void wren_ResourceFontUnload(WrenVM* vm)
{
	WrenFontHandle* handle = (WrenFontHandle*)wrenGetSlotForeign(vm, 0);
	handle->resource       = Struktur::Resource::ResourcePtr<Struktur::Resource::FontResource>();
}

// font.isValid -> Bool
void wren_ResourceFontIsValid(WrenVM* vm)
{
	WrenFontHandle* handle = (WrenFontHandle*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, handle->resource.IsValid());
}

// font.path -> String
void wren_ResourceFontGetPath(WrenVM* vm)
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
void wren_ResourceFontGetSize(WrenVM* vm)
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
void wren_ResourceFontToString(WrenVM* vm)
{
	WrenFontHandle* handle = (WrenFontHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotString(vm, 0, "Font(invalid)");
		return;
	}

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "Font(%s, %d)", handle->resource.GetFilePath().c_str(),
	         handle->resource->GetFontSize());

	wrenSetSlotString(vm, 0, buffer);
}

// ============================================================================
// Music Resource Handle
// ============================================================================

void wren_ResourceMusicAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenMusicHandle));
}

void wren_ResourceMusicFinalize(void* data)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)data;
	handle->~WrenMusicHandle();
}

// Music.load(path) -> MusicHandle
void wren_ResourceMusicLoad(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& resourceManager          = context->GetResourceManager();

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

// Music.unload()
void wren_ResourceMusicUnload(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	handle->resource        = Struktur::Resource::ResourcePtr<Struktur::Resource::MusicResource>();
}

// Music.isValid -> Bool
void wren_ResourceMusicIsValid(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, handle->resource.IsValid());
}

// Music.path -> String
void wren_ResourceMusicGetPath(WrenVM* vm)
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
void wren_ResourceMusicToString(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotString(vm, 0, "Music(invalid)");
		return;
	}

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "Music(%s)", handle->resource.GetFilePath().c_str());

	wrenSetSlotString(vm, 0, buffer);
}

// Music.play()
void wren_ResourceMusicPlay(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	::PlayMusicStream(handle->resource->music);
}

// Music.stop()
void wren_ResourceMusicStop(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	::StopMusicStream(handle->resource->music);
}

// Music.pause()
void wren_ResourceMusicPause(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	::PauseMusicStream(handle->resource->music);
}

// Music.resume()
void wren_ResourceMusicResume(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	::ResumeMusicStream(handle->resource->music);
}

// Music.isPlaying() -> bool
void wren_ResourceMusicIsPlaying(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	bool isPlaying          = ::IsMusicStreamPlaying(handle->resource->music);
	wrenSetSlotBool(vm, 0, isPlaying);
}

// Music.seek(position) -> bool
void wren_ResourceMusicSeek(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	float position          = (float)wrenGetSlotDouble(vm, 1);
	::SeekMusicStream(handle->resource->music, position);
}

// Music.setVolume(volume) -> bool
void wren_ResourceMusicSetVolume(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	float volume            = (float)wrenGetSlotDouble(vm, 1);
	::SetMusicVolume(handle->resource->music, volume);
}

// Music.setPitch(pitch) -> bool
void wren_ResourceMusicSetPitch(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	float pitch             = (float)wrenGetSlotDouble(vm, 1);
	::SetMusicPitch(handle->resource->music, pitch);
}

// Music.setPan(pan) -> bool
void wren_ResourceMusicSetPan(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	float pan               = (float)wrenGetSlotDouble(vm, 1);
	::SetMusicPan(handle->resource->music, pan);
}

// Music.getTimeLength() -> num
void wren_ResourceMusicGetTimeLength(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	double timeLength       = (double)::GetMusicTimeLength(handle->resource->music);
	wrenSetSlotDouble(vm, 0, timeLength);
}

// Music.getTimePlayed() -> num
void wren_ResourceMusicGetTimePlayed(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	double timePlayed       = (double)::GetMusicTimePlayed(handle->resource->music);
	wrenSetSlotDouble(vm, 0, timePlayed);
}

// Music.getLooping() -> bool
void wren_ResourceMusicGetLooping(WrenVM* vm)
{
	WrenMusicHandle* handle = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, handle->resource->music.looping);
}

// Music.setLooping(isLooping) -> bool
void wren_ResourceMusicSetLooping(WrenVM* vm)
{
	WrenMusicHandle* handle         = (WrenMusicHandle*)wrenGetSlotForeign(vm, 0);
	bool isLooping                  = wrenGetSlotBool(vm, 1);
	handle->resource->music.looping = isLooping;
}

// ============================================================================
// Sound Resource Handle
// ============================================================================

void wren_ResourceSoundAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenSoundHandle));
}

void wren_ResourceSoundFinalize(void* data)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)data;
	handle->~WrenSoundHandle();
}

// Sound.load(path) -> SoundHandle
void wren_ResourceSoundLoad(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& resourceManager          = context->GetResourceManager();

	const char* path = wrenGetSlotString(vm, 1);

	// Load sound through resource manager
	Struktur::Resource::ResourcePtr<Struktur::Resource::SoundResource> sound = resourceManager.GetSound(path);

	if (!sound)
	{
		DEBUG_ERROR("Failed to load sound: %s", path);
		wrenSetSlotNull(vm, 0);
		return;
	}

	if (!sound->LoadToHardware())
	{
		DEBUG_ERROR("Failed to load sound: %s", path);
		wrenSetSlotNull(vm, 0);
		return;
	}
	sound->UnloadFromDisk();

	// Create foreign object with resource pointer
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenSoundHandle));
	new (handle) WrenSoundHandle(sound);
}

// Sound.unload()
void wren_ResourceSoundUnload(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);
	handle->resource        = Struktur::Resource::ResourcePtr<Struktur::Resource::SoundResource>();
}

// Sound.isValid -> Bool
void wren_ResourceSoundIsValid(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, handle->resource.IsValid());
}

// Sound.path -> String
void wren_ResourceSoundGetPath(WrenVM* vm)
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
void wren_ResourceSoundToString(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);

	if (!handle->resource)
	{
		wrenSetSlotString(vm, 0, "Sound(invalid)");
		return;
	}

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "Sound(%s)", handle->resource.GetFilePath().c_str());

	wrenSetSlotString(vm, 0, buffer);
}

// Sound.play()
void wren_ResourceSoundPlay(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);
#ifdef EDITOR
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	// Get editor settings from context
	auto& debugSettings = context->GetEditor().GetSettings().debugRender;
	if (debugSettings.audio)
	{
#endif
		::PlaySound(handle->resource->sound);
#ifdef EDITOR
	}
#endif
}

// Sound.stop()
void wren_ResourceSoundStop(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);
	::StopSound(handle->resource->sound);
}

// Sound.pause()
void wren_ResourceSoundPause(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);
	::PauseSound(handle->resource->sound);
}

// Sound.resume()
void wren_ResourceSoundResume(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);
	::ResumeSound(handle->resource->sound);
}

// Sound.isPlaying() -> bool
void wren_ResourceSoundIsPlaying(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);
	bool isPlaying          = ::IsSoundPlaying(handle->resource->sound);
	wrenSetSlotBool(vm, 0, isPlaying);
}

// Sound.setVolume(volume) -> bool
void wren_ResourceSoundSetVolume(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);
	float volume            = (float)wrenGetSlotDouble(vm, 1);
	::SetSoundVolume(handle->resource->sound, volume);
}

// Sound.setPitch(pitch) -> bool
void wren_ResourceSoundSetPitch(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);
	float pitch             = (float)wrenGetSlotDouble(vm, 1);
	::SetSoundPitch(handle->resource->sound, pitch);
}

// Sound.setPan(pan) -> bool
void wren_ResourceSoundSetPan(WrenVM* vm)
{
	WrenSoundHandle* handle = (WrenSoundHandle*)wrenGetSlotForeign(vm, 0);
	float pan               = (float)wrenGetSlotDouble(vm, 1);
	::SetSoundPan(handle->resource->sound, pan);
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================
WREN_BINDING_MODULE(ResourceManager)
{
	// Register Texture foreign class
	WREN_FOREIGN_CLASS(registry, "resourceManager", "Texture", wren_ResourceTextureAllocate,
	                   wren_ResourceTextureFinalize, "Texture resource handle");

	WREN_CLASS_STATIC(registry, "resourceManager", "Texture", "load(_)", wren_ResourceTextureLoad,
	                  "Load texture from path");
	WREN_CLASS_METHOD(registry, "resourceManager", "Texture", "unload()", wren_ResourceTextureUnload,
	                  "Unload texture from path. Manually unload reference because even though wren's garbage "
	                  "collector will do it, it will happen at an unknown time.");
	WREN_CLASS_METHOD(registry, "resourceManager", "Texture", "isValid()", wren_ResourceTextureIsValid,
	                  "Check if texture is valid");
	WREN_CLASS_METHOD(registry, "resourceManager", "Texture", "path", wren_ResourceTextureGetPath, "Get texture path");
	WREN_CLASS_METHOD(registry, "resourceManager", "Texture", "width", wren_ResourceTextureGetWidth,
	                  "Get texture width");
	WREN_CLASS_METHOD(registry, "resourceManager", "Texture", "height", wren_ResourceTextureGetHeight,
	                  "Get texture height");
	WREN_CLASS_METHOD(registry, "resourceManager", "Texture", "toString()", wren_ResourceTextureToString,
	                  "Convert to string");

	// Register Shader foreign class
	WREN_FOREIGN_CLASS(registry, "resourceManager", "Shader", wren_ResourceShaderAllocate, wren_ResourceShaderFinalize,
	                   "Shader resource handle");

	WREN_CLASS_STATIC(registry, "resourceManager", "Shader", "load(_,_)", wren_ResourceShaderLoad,
	                  "Load Shader from path");
	WREN_CLASS_METHOD(registry, "resourceManager", "Shader", "unload()", wren_ResourceShaderUnload,
	                  "Unload Shader from path. Manually unload reference because even though wren's garbage collector "
	                  "will do it, it will happen at an unknown time.");
	WREN_CLASS_METHOD(registry, "resourceManager", "Shader", "isValid()", wren_ResourceShaderIsValid,
	                  "Check if Shader is valid");
	WREN_CLASS_METHOD(registry, "resourceManager", "Shader", "path", wren_ResourceShaderGetPath, "Get Shader path");
	WREN_CLASS_METHOD(registry, "resourceManager", "Shader", "vsFilePath", wren_ResourceShaderGetVSFilePath,
	                  "Get vertex shader");
	WREN_CLASS_METHOD(registry, "resourceManager", "Shader", "fsFilePath", wren_ResourceShaderGetFSFilePath,
	                  "Get fragment shader");
	WREN_CLASS_METHOD(registry, "resourceManager", "Shader", "toString()", wren_ResourceShaderToString,
	                  "Convert to string");

	// Register Font foreign class
	WREN_FOREIGN_CLASS(registry, "resourceManager", "Font", wren_ResourceFontAllocate, wren_ResourceFontFinalize,
	                   "Font resource handle");

	WREN_CLASS_STATIC(registry, "resourceManager", "Font", "load(_,_)", wren_ResourceFontLoad, "Load font from path");
	WREN_CLASS_METHOD(registry, "resourceManager", "Font", "unload()", wren_ResourceFontUnload,
	                  "Unload font from path. Manually unload reference because even though wren's garbage collector "
	                  "will do it, it will happen at an unknown time.");
	WREN_CLASS_METHOD(registry, "resourceManager", "Font", "isValid()", wren_ResourceFontIsValid,
	                  "Check if font is valid");
	WREN_CLASS_METHOD(registry, "resourceManager", "Font", "path", wren_ResourceFontGetPath, "Get font path");
	WREN_CLASS_METHOD(registry, "resourceManager", "Font", "size", wren_ResourceFontGetSize, "Get font size");
	WREN_CLASS_METHOD(registry, "resourceManager", "Font", "toString()", wren_ResourceFontToString,
	                  "Convert to string");

	// Register Music foreign class
	WREN_FOREIGN_CLASS(registry, "resourceManager", "Music", wren_ResourceMusicAllocate, wren_ResourceFontFinalize,
	                   "Font resource handle");

	WREN_CLASS_STATIC(registry, "resourceManager", "Music", "load(_)", wren_ResourceMusicLoad, "Load music from path");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "unload()", wren_ResourceMusicUnload,
	                  "Unload music from path. Manually unload reference because even though wren's garbage collector "
	                  "will do it, it will happen at an unknown time.");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "isValid()", wren_ResourceMusicIsValid,
	                  "Check if music is valid");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "path", wren_ResourceMusicGetPath, "Get music path");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "toString()", wren_ResourceMusicToString,
	                  "Convert to string");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "play()", wren_ResourceMusicPlay, "Play the music");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "stop()", wren_ResourceMusicStop, "Stop the music");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "pause()", wren_ResourceMusicPause, "Pause the music");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "resume()", wren_ResourceMusicResume, "Resume the music");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "isPlaying()", wren_ResourceMusicIsPlaying,
	                  "Check if music is playing");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "seek(_)", wren_ResourceMusicSeek,
	                  "Seek music to a certain position (in seconds)");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "setVolume(_)", wren_ResourceMusicSetVolume,
	                  "Set the musics volume");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "setPitch(_)", wren_ResourceMusicSetPitch,
	                  "Set the musics pitch");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "setPan(_)", wren_ResourceMusicSetPan,
	                  "Set the musics pan");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "getTimeLength()", wren_ResourceMusicGetTimeLength,
	                  "Get music time length (in seconds)");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "getTimePlayed()", wren_ResourceMusicGetTimePlayed,
	                  "Get current music time played (in seconds)");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "getLooping()", wren_ResourceMusicGetLooping,
	                  "Gets if the music should be looped");
	WREN_CLASS_METHOD(registry, "resourceManager", "Music", "setLooping(_)", wren_ResourceMusicSetLooping,
	                  "Gets if the music should be looped");

	// Register Sound foreign class
	WREN_FOREIGN_CLASS(registry, "resourceManager", "Sound", wren_ResourceSoundAllocate, wren_ResourceFontFinalize,
	                   "Font resource handle");

	WREN_CLASS_STATIC(registry, "resourceManager", "Sound", "load(_)", wren_ResourceSoundLoad, "Load sound from path");
	WREN_CLASS_METHOD(registry, "resourceManager", "Sound", "unload()", wren_ResourceSoundUnload,
	                  "Unload sound from path. Manually unload reference because even though wren's garbage collector "
	                  "will do it, it will happen at an unknown time.");
	WREN_CLASS_METHOD(registry, "resourceManager", "Sound", "isValid()", wren_ResourceSoundIsValid,
	                  "Check if sound is valid");
	WREN_CLASS_METHOD(registry, "resourceManager", "Sound", "path", wren_ResourceSoundGetPath, "Get sound path");
	WREN_CLASS_METHOD(registry, "resourceManager", "Sound", "toString()", wren_ResourceSoundToString,
	                  "Convert to string");
	WREN_CLASS_METHOD(registry, "resourceManager", "Sound", "play()", wren_ResourceSoundPlay, "Play the sound");
	WREN_CLASS_METHOD(registry, "resourceManager", "Sound", "stop()", wren_ResourceSoundStop, "Stop playing the sound");
	WREN_CLASS_METHOD(registry, "resourceManager", "Sound", "pause()", wren_ResourceSoundPause, "Pause the sound");
	WREN_CLASS_METHOD(registry, "resourceManager", "Sound", "resume()", wren_ResourceSoundResume,
	                  "Resume playing the sound");
	WREN_CLASS_METHOD(registry, "resourceManager", "Sound", "isPlaying()", wren_ResourceSoundIsPlaying,
	                  "Checks if sound is currently playing");
	WREN_CLASS_METHOD(registry, "resourceManager", "Sound", "setVolume(_)", wren_ResourceSoundSetVolume,
	                  "Set volume of the sound");
	WREN_CLASS_METHOD(registry, "resourceManager", "Sound", "setPitch(_)", wren_ResourceSoundSetPitch,
	                  "Set pitch of the sound");
	WREN_CLASS_METHOD(registry, "resourceManager", "Sound", "setPan(_)", wren_ResourceSoundSetPan,
	                  "Set pan of the sound");
}
