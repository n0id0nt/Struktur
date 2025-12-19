
// ============================================================================
// Texture Resource Handle
// ============================================================================

void wren_TextureAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "Texture", 1);  // Get class into slot 1
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
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
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
	wrenGetVariable(vm, "game", "Texture", 1);  // Get class into slot 1
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
WREN_FOREIGN_CLASS("game", "Texture", wren_TextureAllocate, wren_TextureFinalize,
	"Texture resource handle");

WREN_CLASS_STATIC("game", "Texture", "load(_)", wren_TextureLoad, "Load texture from path");
WREN_CLASS_METHOD("game", "Texture", "isValid", wren_TextureIsValid, "Check if texture is valid");
WREN_CLASS_METHOD("game", "Texture", "path", wren_TextureGetPath, "Get texture path");
WREN_CLASS_METHOD("game", "Texture", "width", wren_TextureGetWidth, "Get texture width");
WREN_CLASS_METHOD("game", "Texture", "height", wren_TextureGetHeight, "Get texture height");
WREN_CLASS_METHOD("game", "Texture", "toString", wren_TextureToString, "Convert to string");

// ============================================================================
// Font Resource Handle
// ============================================================================

#include "Engine/Resource/FontResource.h"

struct WrenFontHandle
{
	Struktur::Resource::ResourcePtr<Struktur::Resource::FontResource> resource;

	WrenFontHandle() {}
	WrenFontHandle(const Struktur::Resource::ResourcePtr<Struktur::Resource::FontResource>& res) : resource(res) {}
};

void wren_FontAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "Font", 1);  // Get class into slot 1
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
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
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
	wrenGetVariable(vm, "game", "Font", 1);  // Get class into slot 1
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
WREN_FOREIGN_CLASS("game", "Font", wren_FontAllocate, wren_FontFinalize,
	"Font resource handle");

WREN_CLASS_STATIC("game", "Font", "load(_)", wren_FontLoad, "Load font from path");
WREN_CLASS_METHOD("game", "Font", "isValid", wren_FontIsValid, "Check if font is valid");
WREN_CLASS_METHOD("game", "Font", "path", wren_FontGetPath, "Get font path");
WREN_CLASS_METHOD("game", "Font", "size", wren_FontGetSize, "Get font size");
WREN_CLASS_METHOD("game", "Font", "toString", wren_FontToString, "Convert to string");

// ============================================================================
// Music Resource Handle
// ============================================================================

#include "Engine/Resource/MusicResource.h"

struct WrenMusicHandle
{
	Struktur::Resource::ResourcePtr<Struktur::Resource::MusicResource> resource;

	WrenMusicHandle() {}
	WrenMusicHandle(const Struktur::Resource::ResourcePtr<Struktur::Resource::MusicResource>& res) : resource(res) {}
};

void wren_MusicAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "Music", 1);  // Get class into slot 1
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
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
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
	wrenGetVariable(vm, "game", "Music", 1);  // Get class into slot 1
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
WREN_FOREIGN_CLASS("game", "Music", wren_MusicAllocate, wren_FontFinalize,
	"Font resource handle");

WREN_CLASS_STATIC("game", "Music", "load(_)", wren_MusicLoad, "Load music from path");
WREN_CLASS_METHOD("game", "Music", "isValid", wren_MusicIsValid, "Check if music is valid");
WREN_CLASS_METHOD("game", "Music", "path", wren_MusicGetPath, "Get music path");
WREN_CLASS_METHOD("game", "Music", "toString", wren_MusicToString, "Convert to string");

// ============================================================================
// Sound Resource Handle
// ============================================================================

#include "Engine/Resource/SoundResource.h"

struct WrenSoundHandle
{
	Struktur::Resource::ResourcePtr<Struktur::Resource::SoundResource> resource;

	WrenSoundHandle() {}
	WrenSoundHandle(const Struktur::Resource::ResourcePtr<Struktur::Resource::SoundResource>& res) : resource(res) {}
};

void wren_SoundAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "Sound", 1);  // Get class into slot 1
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
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
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
WREN_FOREIGN_CLASS("game", "Sound", wren_SoundAllocate, wren_FontFinalize,
	"Font resource handle");

WREN_CLASS_STATIC("game", "Sound", "load(_)", wren_SoundLoad, "Load sound from path");
WREN_CLASS_METHOD("game", "Sound", "isValid", wren_SoundIsValid, "Check if sound is valid");
WREN_CLASS_METHOD("game", "Sound", "path", wren_SoundGetPath, "Get sound path");
WREN_CLASS_METHOD("game", "Sound", "toString", wren_SoundToString, "Convert to string");

// ============================================================================
// UI LABEL - Foreign class wrapping glm::vec2
// ============================================================================

#include "Engine/UI/UILabel.h"

struct WrenUILabel
{
	Struktur::UI::UILabel* label;
};

// Allocator - called when UILabel.new(pixelPosition, percentagePosition, labelText, fontSize) is invoked
void wren_UILabelAllocate(WrenVM* vm)
{
	// Allocate foreign object
	wrenGetVariable(vm, "game", "UILabel", 1);  // Get class into slot 1
	WrenUILabel* vec = (WrenUILabel*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenUILabel));
	new (vec) WrenUILabel{ nullptr };
}

// Finalizer - called when garbage collected
void wren_UILabelFinalize(void* data)
{
	WrenUILabel* vec = (WrenUILabel*)data;
	vec->~WrenUILabel();
}

// UILabel.setVisible(isVisible)
void wren_UILabelSetVisible(WrenVM* vm)
{
	WrenUILabel* handle = static_cast<WrenUILabel*>(wrenGetSlotForeign(vm, 0));
	if (!handle->label)
	{
		DEBUG_ERROR("UILabel.setVisible: label is Null");
		return;
	}
	bool isVisible = wrenGetSlotBool(vm, 1);
	handle->label->SetVisible(isVisible);
}

// UILabel.setFont(font)
void wren_UILabelSetFont(WrenVM* vm)
{
	WrenUILabel* handle = static_cast<WrenUILabel*>(wrenGetSlotForeign(vm, 0));
	if (!handle->label)
	{
		DEBUG_ERROR("UILabel.setFont: label is Null");
		return;
	}
	WrenFontHandle* font = static_cast<WrenFontHandle*>(wrenGetSlotForeign(vm, 1));
	handle->label->SetFont(font->resource);
}

// UILabel.setTextColor(color)
void wren_UILabelSetTextColor(WrenVM* vm)
{
	WrenUILabel* handle = static_cast<WrenUILabel*>(wrenGetSlotForeign(vm, 0));
	if (!handle->label)
	{
		DEBUG_ERROR("UILabel.setTextColor: label is Null");
		return;
	}
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 1));
	::Color rayColor{ (unsigned char)color->value.r, (unsigned char)color->value.g, (unsigned char)color->value.b, (unsigned char)color->value.a };
	handle->label->SetTextColor(rayColor);
}

// UILabel.setPosition(positionPixel, positionPercentage)
void wren_UILabelSetPosition(WrenVM* vm)
{
	WrenUILabel* handle = static_cast<WrenUILabel*>(wrenGetSlotForeign(vm, 0));
	if (!handle->label)
	{
		DEBUG_ERROR("UILabel.setPosition: label is Null");
		return;
	}
	WrenVec2* positionPixel = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* positionPercentage = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	handle->label->SetPosition(positionPixel->value, positionPercentage->value);
}

// UILabel.setAnchorPoint(anchorPoint)
void wren_UILabelSetAnchorPoint(WrenVM* vm)
{
	WrenUILabel* handle = static_cast<WrenUILabel*>(wrenGetSlotForeign(vm, 0));
	if (!handle->label)
	{
		DEBUG_ERROR("UILabel.setAnchorPoint: label is Null");
		return;
	}
	WrenVec2* anchorPoint = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	handle->label->SetAnchorPoint(anchorPoint->value);
}

// Register Sound foreign class
WREN_FOREIGN_CLASS("game", "UILabel", wren_UILabelAllocate, wren_UILabelFinalize, "UI Label component");

WREN_CLASS_METHOD("game", "UILabel", "setVisible(_)", wren_UILabelSetVisible, "Sets Label to be visible");
WREN_CLASS_METHOD("game", "UILabel", "setFont(_)", wren_UILabelSetFont, "Sets the labels font");
WREN_CLASS_METHOD("game", "UILabel", "setTextColor(_)", wren_UILabelSetTextColor, "Sets the labels text color");
WREN_CLASS_METHOD("game", "UILabel", "setPosition(_,_)", wren_UILabelSetPosition, "Sets the labels position");
WREN_CLASS_METHOD("game", "UILabel", "setAnchorPoint(_)", wren_UILabelSetAnchorPoint, "Sets the labels anchor point");

// ============================================================================
// GAME OBJECT MANAGER BINDINGS
// ============================================================================

// GameObject.create(name, parent) -> entityId
void wren_GameObjectCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameObjectManager = context->GetGameObjectManager();

	const char* name = wrenGetSlotString(vm, 1);
	double parentId = wrenGetSlotDouble(vm, 2);
	entt::entity parent = static_cast<entt::entity>(parentId);

	entt::entity entity = gameObjectManager.CreateGameObject(*context, name, parent);

	wrenSetSlotDouble(vm, 0, static_cast<double>(entity));
}

WREN_CLASS_STATIC("game", "GameObject", "create(_,_)", wren_GameObjectCreate, "Create a new Game Object with the given name and parent. Returns entity ID.");

// GameObject.destroy(entity)
void wren_GameObjectDestroy(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameObjectManager = context->GetGameObjectManager();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	gameObjectManager.DestroyGameObject(*context, entity);
}

WREN_CLASS_STATIC("game", "GameObject", "destroy(_)", wren_GameObjectDestroy, "Destroy an Game Object and all its children.");

// GameObject.isValid(entity) -> bool
void wren_GameObjectIsValid(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	bool valid = registry.valid(entity);
	wrenSetSlotBool(vm, 0, valid);
}

WREN_CLASS_STATIC("game", "GameObject", "isValid(_)", wren_GameObjectIsValid, "Check if an entity ID is valid.");

// TODO move this to a separate file.
#define COMPONENT_LIST\
	COMPONENT(Camera, "Camera")\
	COMPONENT(Level, "Level")\
	COMPONENT(PhysicsBody, "PhysicsBody")\
	COMPONENT(Shader, "Shader")\
	COMPONENT(Sprite, "Sprite")\
	COMPONENT(SpriteAnimation, "SpriteAnimation")\
	COMPONENT(TileMap, "TileMap")\
	COMPONENT(LocalTransform, "LocalTransform")\
	COMPONENT(WorldTransform, "WorldTransform")\
	COMPONENT(WrenScript, "Script")\
	// COMPONENT_LIST


// GameObject.hasComponent(entity, componentName) ->bool
void wren_GameObjecthasComponent(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	const char* componentName = wrenGetSlotString(vm, 2);
	bool hasComponent = false;
#define COMPONENT(component_name, component_name_string) 										\
	if (strcmp(componentName, component_name_string) == 0) 						                \
	{																		                    \
        auto* componentValue = registry.try_get<Struktur::Component::component_name>(entity);   \
        hasComponent = componentValue != nullptr;										        \
    } else
	COMPONENT_LIST
#undef COMPONENT
		// need to handle last else statement
	{
		DEBUG_ERROR("%s is not a valid component type", componentName);
	}
	wrenSetSlotBool(vm, 1, hasComponent);
}

WREN_CLASS_STATIC("game", "GameObject", "hasComponent(_)",
	wren_GameObjecthasComponent,
	"Checks if entity has a specific component.");

// GameObject.getAllWithComponent(componentName) -> List
void wren_GameObjectGetAllWithComponent(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	const char* componentName = wrenGetSlotString(vm, 1);

	wrenSetSlotNewList(vm, 0);

	int index = 0;

#define COMPONENT(component_name, component_name_string) 					\
	if (strcmp(componentName, #component_name_string) == 0) 				\
	{																		\
        auto view = registry.view<Struktur::Component::component_name>();	\
        for (auto entity : view)											\
		{																	\
            wrenSetSlotDouble(vm, 1, static_cast<double>(entity));			\
            wrenInsertInList(vm, 0, index, 1);								\
            index++;														\
        }																	\
    } else
	COMPONENT_LIST
#undef COMPONENT
		// need to handle last else statement
	{
		DEBUG_ERROR("%s is not a valid component type", componentName);
	}
}

WREN_CLASS_STATIC("game", "GameObject", "getAllWithComponent(_)",
	wren_GameObjectGetAllWithComponent,
	"Get all entities with a specific component. Returns list of entity IDs.");

// GameObject.getAllWithComponents(componentList) -> List
void wren_GameObjectGetAllWithComponents(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	// Slot 1 contains a Wren list of component names
	if (wrenGetSlotType(vm, 1) != WREN_TYPE_LIST)
	{
		DEBUG_ERROR("Slot 1 must contain a Wren list of component names");
		wrenSetSlotNull(vm, 0);
		return;
	}

	int componentCount = wrenGetListCount(vm, 1);

	wrenEnsureSlots(vm, 3);
	// Get component names from list
	std::vector<std::string> components;
	for (int i = 0; i < componentCount; i++) {
		wrenGetListElement(vm, 1, i, 2); // Get element into slot 2
		const char* componentName = wrenGetSlotString(vm, 2);
		components.emplace_back(componentName);
	}

	// Now filter entities based on components
	wrenSetSlotNewList(vm, 0);
	int resultIndex = 0;

	// Iterate all entities and check if they have ALL required components
	registry.each([&](auto entity)
		{
			bool hasAllComponents = true;

			for (const auto& compName : components)
			{
#define COMPONENT(component_name, component_name_string) 							\
            if (compName == component_name_string)                                  \
			{																		\
				if (!registry.any_of<Struktur::Component::component_name>(entity))	\
				{																	\
                    hasAllComponents = false;										\
                    break;															\
                }																	\
			} else
				COMPONENT_LIST
#undef COMPONENT
					// need to handle last else statement
				{
					DEBUG_ERROR("%s is not a valid component type", compName.c_str());
				}
			}

			if (hasAllComponents) {
				wrenSetSlotDouble(vm, 2, static_cast<double>(entity));
				wrenInsertInList(vm, 0, resultIndex, 2);
				resultIndex++;
			}
		});
}

WREN_CLASS_STATIC("game", "GameObject", "getAllWithComponents(_)",
	wren_GameObjectGetAllWithComponents,
	"Get all entities with ALL specified components. Pass list of component names.");

// GameObject.getAnyWithComponents(componentList) -> List
void wren_GameObjectGetAnyWithComponents(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	if (wrenGetSlotType(vm, 1) != WREN_TYPE_LIST)
	{
		DEBUG_ERROR("Slot 1 must contain a Wren list of component names");
		wrenSetSlotNull(vm, 0);
		return;
	}

	int componentCount = wrenGetListCount(vm, 1);

	std::vector<std::string> components(componentCount);
	for (int i = 0; i < componentCount; i++)
	{
		wrenGetListElement(vm, 1, i, 2);
		components.emplace_back(wrenGetSlotString(vm, 2));
	}

	wrenSetSlotNewList(vm, 0);
	int resultIndex = 0;

	registry.each([&](auto entity) {
		bool hasAnyComponent = false;

		for (const auto& compName : components)
		{
#define COMPONENT(component_name, component_name_string) 							\
	        if (compName == component_name_string)                                  \
			{																		\
				if (!registry.any_of<Struktur::Component::component_name>(entity))	\
				{																	\
                    hasAnyComponent = false;										\
                    break;															\
                }																	\
			} else
			COMPONENT_LIST
#undef COMPONENT
				// need to handle last else statement
			{
				DEBUG_ERROR("%s is not a valid component type", compName.c_str());
			}
		}

		if (hasAnyComponent) {
			wrenSetSlotDouble(vm, 2, static_cast<double>(entity));
			wrenInsertInList(vm, 0, resultIndex, 2);
			resultIndex++;
		}
		});
}

WREN_CLASS_STATIC("game", "GameObject", "getAllWithAnyComponents(_)",
	wren_GameObjectGetAnyWithComponents,
	"Get entities with ANY of the specified components.");

// GameObject.getAll() -> List of all entities
void wren_GameObjectGetAll(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	wrenSetSlotNewList(vm, 0);

	int index = 0;

	// Iterate ALL entities
	registry.each([&](const auto& entity)
		{
			wrenSetSlotDouble(vm, 1, static_cast<double>(entity));
			wrenInsertInList(vm, 0, index, 1);
			index++;
		});
}

WREN_CLASS_STATIC("game", "GameObject", "getAll()", wren_GameObjectGetAll,
	"Get all entities in the registry. Returns list of entity IDs.");

// GameObject.getAllWithIdentifier(Identifier) -> List of entity IDs
void wren_GameObjectGetAllWithIdentifier(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	const char* tag = wrenGetSlotString(vm, 1);

	// Create a Wren list for results
	wrenSetSlotNewList(vm, 0);

	// Find all entities with matching tag/component
	auto view = registry.view<Struktur::Component::Identifier>();

	int index = 0;
	for (const auto& [entity, identifier] : view.each())
	{
		if (identifier.type == tag)
		{
			// Add entity ID to list
			wrenSetSlotDouble(vm, 1, static_cast<double>(entity));
			wrenInsertInList(vm, 0, index, 1);
			index++;
		}
	}

	// Return list (already in slot 0)
}

WREN_CLASS_STATIC("game", "GameObject", "getAllWithIdentifier(_)", wren_GameObjectGetAllWithIdentifier,
	"Get all entities with a specific identifier. Returns list of entity IDs.");

// GameObject.forEach(callback) - calls callback for each entity
void wren_GameObjectForEach(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	// Callback is in slot 1
	WrenHandle* callbackHandle = wrenGetSlotHandle(vm, 1);

	// Iterate all entities
	registry.each([vm, callbackHandle](auto entity) {
		// Call Wren callback with entity ID
		wrenEnsureSlots(vm, 2);
		wrenSetSlotHandle(vm, 0, callbackHandle);
		wrenSetSlotDouble(vm, 1, static_cast<double>(entity));

		wrenCall(vm, callbackHandle);
		});

	wrenReleaseHandle(vm, callbackHandle);
}

WREN_CLASS_STATIC("game", "GameObject", "forEach(_)", wren_GameObjectForEach,
	"Iterate all entities, calling callback for each.");

// GameObject.forEachWithComponent(componentName, callback)
void wren_GameObjectForEachWithComponent(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	const char* componentName = wrenGetSlotString(vm, 1);
	WrenHandle* callbackHandle = wrenGetSlotHandle(vm, 2);

#define COMPONENT(component_name, component_name_string) 					\
	if (strcmp(componentName, component_name_string) == 0) 					\
	{																		\
        auto view = registry.view<Struktur::Component::component_name>();	\
        for (auto entity : view)											\
		{																	\
			wrenEnsureSlots(vm, 2);											\
            wrenSetSlotHandle(vm, 0, callbackHandle);						\
            wrenSetSlotDouble(vm, 1, static_cast<double>(entity));			\
            wrenCall(vm, callbackHandle);									\
        }																	\
    } else
	COMPONENT_LIST
#undef COMPONENT
		// need to handle last else statement
	{
		DEBUG_ERROR("%s is not a valid component type", componentName);
	}

	wrenReleaseHandle(vm, callbackHandle);
}

WREN_CLASS_STATIC("game", "GameObject", "forEachWithComponent(_,_)",
	wren_GameObjectForEachWithComponent,
	"Iterate entities with component, calling callback for each.");

// GameObject.forEachWithComponents(componentName, callback)
void wren_GameObjectForEachWithComponents(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	if (wrenGetSlotType(vm, 1) != WREN_TYPE_LIST)
	{
		DEBUG_ERROR("Slot 1 must contain a Wren list of component names");
		wrenSetSlotNull(vm, 0);
		return;
	}
	WrenHandle* callbackHandle = wrenGetSlotHandle(vm, 2);

	int componentCount = wrenGetListCount(vm, 1);

	// Get component names from list
	std::vector<std::string> components(componentCount);
	for (int i = 0; i < componentCount; i++) {
		wrenGetListElement(vm, 1, i, 2); // Get element into slot 2
		const char* componentName = wrenGetSlotString(vm, 2);
		components.emplace_back(componentName);
	}

	// Iterate all entities and check if they have ALL required components
	registry.each([&](auto entity)
		{
			bool hasAllComponents = true;

			for (const auto& compName : components)
			{
#define COMPONENT(component_name, component_name_string) 							\
			if (compName == component_name_string) 									\
			{																		\
				if (!registry.any_of<Struktur::Component::component_name>(entity))	\
				{																	\
                    hasAllComponents = false;										\
                    break;															\
                }																	\
			} else
				COMPONENT_LIST
#undef COMPONENT
					// need to handle last else statement
				{
					DEBUG_ERROR("%s is not a valid component type", compName.c_str());
				}
			}

			if (hasAllComponents) {
				wrenEnsureSlots(vm, 2);
				wrenSetSlotHandle(vm, 0, callbackHandle);
				wrenSetSlotDouble(vm, 1, static_cast<double>(entity));
				wrenCall(vm, callbackHandle);
			}
		});

	wrenReleaseHandle(vm, callbackHandle);
}

WREN_CLASS_STATIC("game", "GameObject", "forEachWithComponents(_,_)",
	wren_GameObjectForEachWithComponents,
	"Iterate entities with component, calling callback for each.");

// GameObject.forEachWithAnyComponents(componentName, callback)
void wren_GameObjectForEachWithAnyComponents(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	if (wrenGetSlotType(vm, 1) != WREN_TYPE_LIST)
	{
		DEBUG_ERROR("Slot 1 must contain a Wren list of component names");
		wrenSetSlotNull(vm, 0);
		return;
	}
	WrenHandle* callbackHandle = wrenGetSlotHandle(vm, 2);

	int componentCount = wrenGetListCount(vm, 1);

	// Get component names from list
	std::vector<std::string> components(componentCount);
	for (int i = 0; i < componentCount; i++) {
		wrenGetListElement(vm, 1, i, 2); // Get element into slot 2
		const char* componentName = wrenGetSlotString(vm, 2);
		components.emplace_back(componentName);
	}

	// Iterate all entities and check if they have ANY required components
	registry.each([&](auto entity)
		{
			bool hasAnyComponents = false;

			for (const auto& compName : components)
			{
#define COMPONENT(component_name, component_name_string) 							\
			if (compName == component_name_string) 									\
			{																		\
				if (registry.any_of<Struktur::Component::component_name>(entity))	\
				{																	\
                    hasAnyComponents = true;										\
                    break;															\
                }																	\
			} else
				COMPONENT_LIST
#undef COMPONENT
					// need to handle last else statement
				{
					DEBUG_ERROR("%s is not a valid component type", compName.c_str());
				}
			}

			if (hasAnyComponents) {
				wrenEnsureSlots(vm, 2);
				wrenSetSlotHandle(vm, 0, callbackHandle);
				wrenSetSlotDouble(vm, 1, static_cast<double>(entity));
				wrenCall(vm, callbackHandle);
			}
		});

	wrenReleaseHandle(vm, callbackHandle);
}

WREN_CLASS_STATIC("game", "GameObject", "forEachWithAnyComponents(_,_)",
	wren_GameObjectForEachWithAnyComponents,
	"Iterate entities with component, calling callback for each.");



// ============================================================================
// APPLICATION BINDINGS
// ============================================================================


// Application.setWindowSize(windowWidth, windowHeight)
void wren_ApplicationSetWindowSize(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();

	double windowWidthDouble = wrenGetSlotDouble(vm, 1);
	double windowHeightDouble = wrenGetSlotDouble(vm, 2);
	int windowWidth = static_cast<int>(windowWidthDouble);
	int windowHeight = static_cast<int>(windowHeightDouble);

	gameData.gameWidth = windowWidth;
	gameData.gameHeight = windowHeight;

	// TODO check if the window size needs to be changed
	// this can be called before there is a window so need to handle that case as well
	// need to handle debug and editor builds when the game and application size does not match
}

WREN_CLASS_STATIC("game", "Application", "setWindowSize(_,_)", wren_ApplicationSetWindowSize, "Change the size of the game window.");

// Application.setApplicationName(name)
void wren_ApplicationSetApplicationName(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData = context->GetGameData();

	const char* name = wrenGetSlotString(vm, 1);

	gameData.projectName = name;

	// TODO check if the window name needs to be changed here
	// this can be called before there is a window so need to handle that case as well
}

WREN_CLASS_STATIC("game", "Application", "setApplicationName(_)", wren_ApplicationSetApplicationName, "Changes the name of the game window.");

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/System/TransformSystem.h"

// ============================================================================
// TRANSFORM BINDINGS
// ============================================================================

// TODO also consider the physics body of the object when setting its position.

// Transform.getPosition(entity) -> vec3 or null
void wren_TransformGetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* transform = registry.try_get<Struktur::Component::WorldTransform>(entity);
	if (!transform) {
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "game", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->position);
}

WREN_CLASS_STATIC("game", "Transform", "getPosition(_)", wren_TransformGetPosition, "Get the world position of an entity. Returns vec3 or null if no transform.");

// Transform.setPosition(entity, vec3)
void wren_TransformSetPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	// Get Vec3 from slot 2
	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}

	transformSystem.SetWorldTransform(*context, entity, vec->value, glm::vec3(1.0f), glm::quat(1, 0, 0, 0));
}

WREN_CLASS_STATIC("game", "Transform", "setPosition(_,_)", wren_TransformSetPosition, "Set the world position of an entity.");

// Transform.setLocalPosition(entity, position)
void wren_TransformSetLocalPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	// Get Vec3 from slot 2
	WrenVec3* vec = static_cast<WrenVec3*>(wrenGetSlotForeign(vm, 2));

	if (!vec)
	{
		DEBUG_ERROR("Transform.setPosition: Invalid Vec3");
		return;
	}

	transformSystem.SetLocalTransform(*context, entity, vec->value, glm::vec3(1.0f), glm::quat(1, 0, 0, 0));
}

WREN_CLASS_STATIC("game", "Transform", "setLocalPosition(_,_)", wren_TransformSetLocalPosition, "Set the local position of an entity.");

// Transform.getLocalPosition(entity) -> Vec3 or null
void wren_TransformGetLocalPosition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* transform = registry.try_get<Struktur::Component::LocalTransform>(entity);
	if (!transform) {
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create Vec3 foreign object with position
	wrenGetVariable(vm, "game", "Vec3", 1);  // Get class into slot 1
	WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
	new (vec) WrenVec3(transform->position);
}

WREN_CLASS_STATIC("game", "Transform", "getLocalPosition(_)", wren_TransformGetLocalPosition, "Get the local position of an entity. Returns [x, y, z] or null if no transform.");

// Transform.getRotation(entity) -> Quat
void wren_TransformGetRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* transform = registry.try_get<Struktur::Component::WorldTransform>(entity);

	if (!transform)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	// Create Quat foreign object with rotation
	wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transform->rotation);
}

WREN_CLASS_STATIC("game", "Transform", "getRotation(_)", wren_TransformGetRotation, "Get rotation of entity as Quat.");

// Transform.setLocalRotation(entity, quat)
void wren_TransformSetLocalRotation(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 2);

	if (!quat)
	{
		DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}

	// Get current transform
	auto& registry = context->GetRegistry();
	auto* transform = registry.try_get<Struktur::Component::LocalTransform>(entity);

	if (transform)
	{
		transformSystem.SetLocalTransform(*context, entity, transform->position, transform->scale, quat->value);
	}
}

WREN_CLASS_STATIC("game", "Transform", "setLocalRotation(_,_)", wren_TransformSetLocalRotation, "Set local rotation of entity from Quat.");

// Transform.setRotation(entity, quat)
void wren_TransformSetRotation(WrenVM* vm) {
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& systemManager = context->GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 2);

	if (!quat)
	{
		DEBUG_ERROR("Transform.setRotation: Invalid Quat");
		return;
	}

	// Get current transform
	auto& registry = context->GetRegistry();
	auto* transform = registry.try_get<Struktur::Component::WorldTransform>(entity);

	if (transform)
	{
		transformSystem.SetWorldTransform(*context, entity, transform->position, transform->scale, quat->value);
	}
}

WREN_CLASS_STATIC("game", "Transform", "setRotation(_,_)", wren_TransformSetRotation, "Set rotation of entity from Quat.");

// Transform.getLocalRotation(entity) -> Quat
void wren_TransformGetLocalRotation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* transform = registry.try_get<Struktur::Component::LocalTransform>(entity);

	if (!transform)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenGetVariable(vm, "game", "Vec3", 1);  // Get class into slot 1
	WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
	new (quat) WrenQuat(transform->rotation);
}

WREN_CLASS_STATIC("game", "Transform", "getLocalRotation(_)", wren_TransformGetLocalRotation, "Get local rotation of entity as Quat.");

// ============================================================================
// INVENTORY BINDINGS
// ============================================================================

// Inventory.contains(item) -> bool
void wren_InventoryContains(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& inventory = context->GetInventory();

	const char* itemName = wrenGetSlotString(vm, 1);

	bool containsItem = std::find(inventory.begin(), inventory.end(), itemName) != inventory.end();

	wrenSetSlotBool(vm, 0, containsItem);
}

WREN_CLASS_STATIC("game", "Inventory", "contains(_)", wren_InventoryContains, "Check if an item is contained in the inventorty.");

// ============================================================================
// WORLD BINDINGS
// ============================================================================

// World.getLevelIndex(entity, levelName) -> number
void wren_WorldGetLevelIndex(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	const char* levelName = wrenGetSlotString(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* worldComponent = registry.try_get<Struktur::Component::World>(entity);

	if (!worldComponent)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	Struktur::FileLoading::LevelParser::World& worldMap = worldComponent->worldMap;

	int index = -1;
	for (int i = 0; i < worldMap.levels.size(); i++)
	{
		if (worldMap.levels[i].identifier == levelName)
		{
			index = i;
			break;
		}
	}

	if (index == -1)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	double doubleNumber = static_cast<double>(index);

	wrenSetSlotDouble(vm, 0, doubleNumber);
}

WREN_CLASS_STATIC("game", "World", "getLevelIndex(_,_)", wren_WorldGetLevelIndex, "Get the index of an Level in the world.");

// ============================================================================
// LEVEL BINDINGS
// ============================================================================
#include "Engine/Game/Level.h"

// Level.createWorldEntity(worldFilePath) -> number
void wren_LevelCreateWorldEntity(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	const char* worldFilePath = wrenGetSlotString(vm, 1);
	entt::entity worldEntity = Struktur::GameResource::Level::CreateWorldEntity(*context, worldFilePath);
	double entityId = static_cast<double>(worldEntity);
	wrenSetSlotDouble(vm, 0, entityId);
}

WREN_CLASS_STATIC("game", "Level", "createWorldEntity(_)", wren_LevelCreateWorldEntity, "Loads in a LDTK world file and creates the world game object and corresponding components.");

// Level.loadLevelEntities(worldEntity, levelIndex) -> number
// TODO handle null case for this
void wren_LevelLoadLevelEntities(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity worldEntity = static_cast<entt::entity>(entityId);

	double levelDouble = wrenGetSlotDouble(vm, 2);
	int levelIndex = static_cast<int>(levelDouble);

	entt::entity levelEntity = Struktur::GameResource::Level::LoadLevelEntities(*context, worldEntity, levelIndex);

	entityId = static_cast<double>(levelEntity);
	wrenSetSlotDouble(vm, 0, entityId);
}

WREN_CLASS_STATIC("game", "Level", "loadLevelEntities(_,_)", wren_LevelLoadLevelEntities, "Creates a level in the game and all its corresponding objects and entities.");

// ============================================================================
// RESOURCE MANAGER BINDINGS
// ============================================================================

// ResourceManager.getFontResource(fontPath) -> resourcePtr
void wren_ResourceManagerGetFontResource(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Resource::ResourceManager& resourceManager = context->GetResourceManager();

	const char* fontPath = wrenGetSlotString(vm, 1);

	wrenGetVariable(vm, "game", "Font", 1);  // Get class into slot 1
	WrenFontHandle* font = (WrenFontHandle*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenFontHandle));
	new (font) WrenFontHandle(resourceManager.GetFontResource(fontPath));
}

WREN_CLASS_STATIC("game", "ResourceManager", "getFontResource(_)", wren_ResourceManagerGetFontResource, "Creates a pointer to the font resource in the resource pool.");

// ResourceManager.getTextureResource(texturePath) -> resourcePtr
void wren_ResourceManagerGetTextureResource(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Resource::ResourceManager& resourceManager = context->GetResourceManager();

	const char* texturePath = wrenGetSlotString(vm, 1);

	wrenGetVariable(vm, "game", "Texture", 1);  // Get class into slot 1
	WrenTextureHandle* texture = (WrenTextureHandle*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenTextureHandle));
	new (texture) WrenTextureHandle(resourceManager.GetTexture(texturePath));
}

WREN_CLASS_STATIC("game", "ResourceManager", "getTextureResource(_)", wren_ResourceManagerGetTextureResource, "Creates a pointer to the texture resource in the resource pool.");

// ============================================================================
// SPRITE BINDINGS
// ============================================================================

// Sprite.create(spriteEntity, texture, color, offset, columns, rows, flipped, index, renderPriority) -> number
void wren_SpriteCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	entt::entity levelEntity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	WrenTextureHandle* texture = static_cast<WrenTextureHandle*>(wrenGetSlotForeign(vm, 2));
	WrenVec4* color = static_cast<WrenVec4*>(wrenGetSlotForeign(vm, 3));
	WrenVec2* offset = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 4));
	int columns = static_cast<int>(wrenGetSlotDouble(vm, 5));
	int rows = static_cast<int>(wrenGetSlotDouble(vm, 6));
	bool flipped = wrenGetSlotBool(vm, 7);
	int index = static_cast<int>(wrenGetSlotDouble(vm, 8));
	int renderPriority = static_cast<int>(wrenGetSlotDouble(vm, 9));

	::Color rayColor{ (unsigned char)color->value.r, (unsigned char)color->value.g, (unsigned char)color->value.b, (unsigned char)color->value.a };

	registry.emplace<Struktur::Component::Sprite>(levelEntity, texture->resource, rayColor, offset->value, columns, rows, flipped, index, renderPriority);
}

WREN_CLASS_STATIC("game", "Sprite", "create(_,_,_,_,_,_,_,_,_)", wren_SpriteCreate, "Creates the sprite Component.");

// Sprite.setRenderPriority(entity, renderPriority)
void wren_SpriteSetRenderPriority(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	double renderPriority = wrenGetSlotDouble(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* sprite = registry.try_get<Struktur::Component::Sprite>(entity);

	if (!sprite)
	{
		return;
	}

	sprite->renderPriority = static_cast<int>(renderPriority);
}

WREN_CLASS_STATIC("game", "Sprite", "setRenderPriority(_,_)", wren_SpriteSetRenderPriority, "Sets the render priority of a sprite component");

// Sprite.setFlipped(entity, flipped)
void wren_SpriteSetFlipped(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	bool flipped = wrenGetSlotBool(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* sprite = registry.try_get<Struktur::Component::Sprite>(entity);

	if (!sprite)
	{
		return;
	}

	sprite->flipped = flipped;
}

WREN_CLASS_STATIC("game", "Sprite", "setFlipped(_,_)", wren_SpriteSetFlipped, "Flips a sprite in a horizontal direction");

// ============================================================================
// SCRIPT BINDINGS
// ============================================================================

#include "Engine/ECS/System/WrenScriptSystem.h"

// Script.create(spriteEntity, scriptPath, className) -> number
void wren_ScriptCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	entt::entity levelEntity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	const char* scriptPath = wrenGetSlotString(vm, 2);
	const char* className = wrenGetSlotString(vm, 3);

	registry.emplace<Struktur::Component::WrenScript>(levelEntity, scriptPath, className);
}

WREN_CLASS_STATIC("game", "Script", "create(_,_,_)", wren_ScriptCreate, "Creates the script Component.");

// Script.createArg(spriteEntity, scriptPath, className, args) -> number
void wren_ScriptCreateArg(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	Struktur::System::WrenScriptSystem& scriptSystem = systemManager.GetSystem<Struktur::System::WrenScriptSystem>();

	entt::entity levelEntity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
	const char* scriptPath = wrenGetSlotString(vm, 2);
	const char* className = wrenGetSlotString(vm, 3);
	const char* args = wrenGetSlotString(vm, 4);

	auto& script = registry.emplace<Struktur::Component::WrenScript>(levelEntity, scriptPath, className, args);

	// Initialise the script

	//if (!scriptSystem.InitialiseScript(*context, levelEntity, script))
	//{
	//    DEBUG_ERROR("Failed to create script: %s", scriptPath);
	//    return;
	//}
	//scriptSystem.CallCreate(*context, levelEntity, script);
}

WREN_CLASS_STATIC("game", "Script", "createArg(_,_,_,_)", wren_ScriptCreateArg, "Creates the script Component with an arg.");

// Script.get(entity) -> classHandle or null
void wren_ScriptGet(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* script = registry.try_get<Struktur::Component::WrenScript>(entity);
	if (!script)
	{
		DEBUG_ERROR("Script.get: Entity does not have script");
		wrenSetSlotNull(vm, 0);
		return;
	}
	if (script->hasError)
	{
		DEBUG_WARNING("Script.get: Entity's script has an error. unable to call");
		wrenSetSlotNull(vm, 0);
		return;
	}
	if (!script->isInitialised)
	{
		DEBUG_WARNING("Script.get: Entity's script is not initialised");
		wrenSetSlotNull(vm, 0);
		return;
	}
	// Get the script's instance
	if (!script->instanceHandle)
	{
		DEBUG_ERROR("Script.get: No instance handle");
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenSetSlotHandle(vm, 0, script->instanceHandle);
}

WREN_CLASS_STATIC("game", "Script", "get(_)", wren_ScriptGet, "Gets a method on an entity's script");

// ============================================================================
// UI MANAGER BINDINGS
// ============================================================================

// UIManager.createUILabel(pixelPosition, percentagePosition, labelText, fontSize) -> Vec2
void wren_UIManagerCreateUILabel(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));

	WrenVec2* absolutePosition = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* relativePosition = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	const char* labelText = wrenGetSlotString(vm, 3);
	//TODO Make this optional
	float fontSz = static_cast<float>(wrenGetSlotDouble(vm, 4));

	Struktur::UI::UIManager& uiManager = context->GetUIManager();
	auto* label = uiManager.CreateElement<Struktur::UI::UILabel>(*context, absolutePosition->value, relativePosition->value, labelText, fontSz);

	// Allocate foreign object
	wrenGetVariable(vm, "game", "UILabel", 1);  // Get class into slot 1
	WrenUILabel* vec = (WrenUILabel*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenUILabel));
	new (vec) WrenUILabel{ label };
}

WREN_CLASS_STATIC("game", "UIManager", "createUILabel(_,_,_,_)", wren_UIManagerCreateUILabel, "Creates the UI component for UILabel.");

// UIManager.removeUILabel(label)
void wren_UIManagerRemoveUILabel(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::UI::UIManager& uiManager = context->GetUIManager();

	WrenUILabel* uiLabel = static_cast<WrenUILabel*>(wrenGetSlotForeign(vm, 1));

	uiManager.RemoveElement(uiLabel->label);

	uiLabel->label = nullptr;
}

WREN_CLASS_STATIC("game", "UIManager", "removeUILabel(_)", wren_UIManagerRemoveUILabel, "Creates the UI component for UILabel.");


// ============================================================================
// CAMERA BINDINGS
// ============================================================================

struct WrenCamera
{
	entt::entity entity = entt::null;
	Struktur::Component::Camera* camera = nullptr;

	WrenCamera() : camera(nullptr), entity(entt::null) {}
	WrenCamera(entt::entity entity, Struktur::Component::Camera* c) : camera(c), entity(entity) {}
};

// Allocator
void wren_CameraAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "Camera", 0);  // Get class into slot 1
	WrenCamera* camera = (WrenCamera*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenCamera));

	// Camera.new() - identity
	new (camera) WrenCamera();
}

// Finalizer
void wren_CameraFinalize(void* data)
{
	WrenCamera* camera = (WrenCamera*)data;
	camera->~WrenCamera();
}

void wren_CameraGetZoom(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, camera->camera->zoom);
}

void wren_CameraSetZoom(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	camera->camera->zoom = (float)wrenGetSlotDouble(vm, 1);
}

void wren_CameraGetForcePosition(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, camera->camera->forcePosition);
}

void wren_CameraSetForcePosition(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	camera->camera->forcePosition = wrenGetSlotBool(vm, 1);
}

void wren_CameraGetDamping(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);

	wrenGetVariable(vm, "game", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(camera->camera->damping);
}

void wren_CameraSetDamping(WrenVM* vm)
{
	WrenCamera* camera = (WrenCamera*)wrenGetSlotForeign(vm, 0);
	WrenVec2* damping = (WrenVec2*)wrenGetSlotForeign(vm, 1);
	camera->camera->damping = damping->value;
}

// Camera.create(entity) -> Camera
void wren_CameraCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto& cameraComponent = registry.emplace<Struktur::Component::Camera>(entity);

	wrenGetVariable(vm, "game", "Camera", 1);  // Get class into slot 1
	WrenCamera* camera = (WrenCamera*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenCamera));

	// Camera.new() - identity
	new (camera) WrenCamera(entity, &cameraComponent);
}

// Camera.worldPosToScreenPos(worldPos) -> Vec2
void wren_CameraWorldPosToScreenPos(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::GameResource::Camera& camera = context->GetCamera();

	WrenVec2* worldPos = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	glm::vec2 screenPos = camera.WorldPosToScreenPos(worldPos->value);

	wrenGetVariable(vm, "game", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(screenPos);
}

// Camera.screenPosToWorldPos(worldPos) -> Vec2
void wren_CameraScreenPosToWorldPos(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::GameResource::Camera& camera = context->GetCamera();

	WrenVec2* screenPos = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	glm::vec2 worldPos = camera.ScreenPosToWorldPos(screenPos->value);

	wrenGetVariable(vm, "game", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(worldPos);
}

// Register Vec3 foreign class
WREN_FOREIGN_CLASS("game", "Camera", wren_CameraAllocate, wren_CameraFinalize, "Camera component class");

// Register methods
WREN_CLASS_METHOD("game", "Camera", "zoom", wren_CameraGetZoom, "Get the zoom");
WREN_CLASS_METHOD("game", "Camera", "zoom=(_)", wren_CameraSetZoom, "Set the zoom");
WREN_CLASS_METHOD("game", "Camera", "forcePosition", wren_CameraGetForcePosition, "Get the forcePosition, will directly set the position of the next frame");
WREN_CLASS_METHOD("game", "Camera", "forcePosition=(_)", wren_CameraSetForcePosition, "Set the forcePosition, will directly set the position of the next frame");
WREN_CLASS_METHOD("game", "Camera", "damping", wren_CameraGetDamping, "Get the damping");
WREN_CLASS_METHOD("game", "Camera", "damping=(_)", wren_CameraSetDamping, "Set the damping");

// Register static methods
WREN_CLASS_STATIC("game", "Camera", "create(_)", wren_CameraCreate, "Creates a camera component.");
WREN_CLASS_STATIC("game", "Camera", "worldPosToScreenPos(_)", wren_CameraWorldPosToScreenPos, "Converts a world position to the screen position from the currently active camera.");
WREN_CLASS_STATIC("game", "Camera", "screenPosToWorldPos(_)", wren_CameraScreenPosToWorldPos, "Converts a screen position to the world position from the currently active camera.");

// ============================================================================
// LEVEL BINDINGS
// ============================================================================

// Input.getInputAxis2(inputKey) -> Vec2
void wren_InputGetInputAxis2(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Core::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	glm::vec2 inputDir = input.GetInputAxis2(inputKey);

	wrenGetVariable(vm, "game", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(inputDir);
}

WREN_CLASS_STATIC("game", "Input", "getInputAxis2(_)", wren_InputGetInputAxis2, "Gets input dir of a key code.");

// Input.isInputJustReleased(inputKey) -> bool
void wren_InputIsInputJustReleased(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Core::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustReleased = input.IsInputJustReleased(inputKey);

	wrenSetSlotBool(vm, 0, inputJustReleased);
}

WREN_CLASS_STATIC("game", "Input", "isInputJustReleased(_)", wren_InputIsInputJustReleased, "Gets input was just released.");

// ============================================================================
// BODY DEFINITION BINDINGS
// ============================================================================

struct WrenBodyDefinition
{
	b2BodyDef bodyDef;

	WrenBodyDefinition() : bodyDef() {}
	WrenBodyDefinition(const b2BodyDef& b) : bodyDef(b) {}
};

// Allocator
void wren_BodyDefinitionAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "BodyDefinition", 0);  // Get class into slot 1
	WrenBodyDefinition* bodyDef = (WrenBodyDefinition*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();
}

// Finalizer
void wren_BodyDefinitionFinalize(void* data)
{
	WrenBodyDefinition* bodyDef = (WrenBodyDefinition*)data;
	bodyDef->~WrenBodyDefinition();
}

void wren_BodyDefinitionCreateDynamicBody(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "BodyDefinition", 0);  // Get class into slot 1
	WrenBodyDefinition* bodyDef = (WrenBodyDefinition*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();

	bodyDef->bodyDef.type = b2_dynamicBody;
}

void wren_BodyDefinitionCreateStaticBody(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "BodyDefinition", 0);  // Get class into slot 1
	WrenBodyDefinition* bodyDef = (WrenBodyDefinition*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();

	bodyDef->bodyDef.type = b2_staticBody;
}

void wren_BodyDefinitionCreateKinematicBody(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "BodyDefinition", 0);  // Get class into slot 1
	WrenBodyDefinition* bodyDef = (WrenBodyDefinition*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenBodyDefinition));

	// BodyDefinition.new() - identity
	new (bodyDef) WrenBodyDefinition();

	bodyDef->bodyDef.type = b2_kinematicBody;
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("game", "BodyDefinition", wren_BodyDefinitionAllocate, wren_BodyDefinitionFinalize, "BodyDefinition class wraps b2BodyDef");

// Register static methods
WREN_CLASS_STATIC("game", "BodyDefinition", "createDynamicBody()", wren_BodyDefinitionCreateDynamicBody, "Get the BodyDefinition as a dynamic Body");
WREN_CLASS_STATIC("game", "BodyDefinition", "createStaticBody()", wren_BodyDefinitionCreateStaticBody, "Get the BodyDefinition as a static Body");
WREN_CLASS_STATIC("game", "BodyDefinition", "createKinematicBody()", wren_BodyDefinitionCreateKinematicBody, "Get the BodyDefinition as a kinematic Body");

// ============================================================================
// CIRCLE SHAPE BINDINGS
// ============================================================================

struct WrenPhysicsCircleShape
{
	b2CircleShape physicsShape;

	WrenPhysicsCircleShape() : physicsShape() {}
	WrenPhysicsCircleShape(const b2CircleShape& b) : physicsShape(b) {}
	WrenPhysicsCircleShape(float radius) : physicsShape()
	{
		physicsShape.m_radius = radius;
	}
};

// Allocator
void wren_PhysicsCircleShapeAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "PhysicsCircleShape", 0);  // Get class into slot 1
	WrenPhysicsCircleShape* bodyDef = (WrenPhysicsCircleShape*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenPhysicsCircleShape));

	// Initialise with constructor parameters if provided
	if (wrenGetSlotCount(vm) >= 2)
	{
		float radius = (float)wrenGetSlotDouble(vm, 1);
		new (bodyDef) WrenPhysicsCircleShape(radius);
	}
	else
	{
		new (bodyDef) WrenPhysicsCircleShape();
	}
}

// Finalizer
void wren_PhysicsCircleShapeFinalize(void* data)
{
	WrenPhysicsCircleShape* bodyDef = (WrenPhysicsCircleShape*)data;
	bodyDef->~WrenPhysicsCircleShape();
}


void wren_PhysicsCircleShapeGetRadius(WrenVM* vm)
{
	WrenPhysicsCircleShape* physicsShape = (WrenPhysicsCircleShape*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, physicsShape->physicsShape.m_radius);
}

void wren_PhysicsCircleShapeSetRadius(WrenVM* vm)
{
	WrenPhysicsCircleShape* physicsShape = (WrenPhysicsCircleShape*)wrenGetSlotForeign(vm, 0);
	physicsShape->physicsShape.m_radius = (float)wrenGetSlotDouble(vm, 1);
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("game", "PhysicsCircleShape", wren_PhysicsCircleShapeAllocate, wren_PhysicsCircleShapeFinalize, "BodyDefinition class wraps b2BodyDef");

// Register constructors
WREN_CONSTRUCTOR_DOC("game", "PhysicsCircleShape", wren_PhysicsCircleShapeAllocate, "Create physics circle shape", );
WREN_CONSTRUCTOR_DOC("game", "PhysicsCircleShape", wren_PhysicsCircleShapeAllocate, "Create physics circle shape with a radius components", radius);

// Register methods
WREN_CLASS_METHOD("game", "PhysicsCircleShape", "radius", wren_PhysicsCircleShapeGetRadius, "Get physics circle shape's radius");
WREN_CLASS_METHOD("game", "PhysicsCircleShape", "radius=(_)", wren_PhysicsCircleShapeSetRadius, "Set physics circle shape's radius");

// ============================================================================
// PHYSICS BODY BINDINGS
// ============================================================================
#include "System/PhysicsSystem.h"

struct WrenPhysicsBody
{
	entt::entity entity = entt::null;
	Struktur::Component::PhysicsBody* physicsBody = nullptr;

	WrenPhysicsBody() {}
	WrenPhysicsBody(entt::entity entity, Struktur::Component::PhysicsBody* physicsBody) : entity(entity), physicsBody(physicsBody) {}
};

// Allocator
void wren_PhysicsBodyAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "PhysicsBody", 0);  // Get class into slot 1
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenPhysicsBody));

	// Camera.new() - identity
	new (physicsBody) WrenPhysicsBody();
}

// Finalizer
void wren_PhysicsBodyFinalize(void* data)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)data;
	physicsBody->~WrenPhysicsBody();
}

void wren_PhysicsBodySetFixedRotation(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	bool fixedRotation = (float)wrenGetSlotBool(vm, 1);
	physicsBody->physicsBody->body->SetFixedRotation(fixedRotation);
}

void wren_PhysicsBodyGetSyncFromPhysics(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, physicsBody->physicsBody->syncFromPhysics);
}

void wren_PhysicsBodySetSyncFromPhysics(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	physicsBody->physicsBody->syncFromPhysics = (float)wrenGetSlotBool(vm, 1);
}

void wren_PhysicsBodyGetSyncToPhysics(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, physicsBody->physicsBody->syncToPhysics);
}

void wren_PhysicsBodySetSyncToPhysics(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);
	physicsBody->physicsBody->syncToPhysics = (float)wrenGetSlotBool(vm, 1);
}

// PhysicsBody.setLinearVelocity(entity, velocity)
void wren_PhysicsBodyStaticSetLinearVelocity(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& registry = context->GetRegistry();
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
	WrenVec2* velocity = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));

	auto* physicsBodyComponent = registry.try_get<Struktur::Component::PhysicsBody>(entity);

	if (!physicsBodyComponent)
	{
		return;
	}

	b2Vec2 b2Velecity = b2Vec2(velocity->value.x, velocity->value.y);
	physicsBodyComponent->body->SetLinearVelocity(b2Velecity);
}

// PhysicsBody.linearVelocity = velocity
void wren_PhysicsBodySetLinearVelocity(WrenVM* vm)
{
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenGetSlotForeign(vm, 0);

	WrenVec2* velocity = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));

	b2Vec2 b2Velecity = b2Vec2(velocity->value.x, velocity->value.y);
	physicsBody->physicsBody->body->SetLinearVelocity(b2Velecity);
}

// PhysicsBody.create(entity, bodyDef, shape) -> PhysicsBody
void wren_PhysicsBodyCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& physicsSystem = systemManager.GetSystem<Struktur::System::PhysicsSystem>();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
	WrenBodyDefinition* bodyDef = static_cast<WrenBodyDefinition*>(wrenGetSlotForeign(vm, 2));
	WrenPhysicsCircleShape* shape = static_cast<WrenPhysicsCircleShape*>(wrenGetSlotForeign(vm, 3));

	Struktur::Component::PhysicsBody& physicsBodyComponent = physicsSystem.CreatePhysicsBody(*context, entity, bodyDef->bodyDef, shape->physicsShape);

	wrenGetVariable(vm, "game", "PhysicsBody", 1);  // Get class into slot 1
	WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenPhysicsBody));

	// PhysicsBody.new() - identity
	new (physicsBody) WrenPhysicsBody(entity, &physicsBodyComponent);
}

// PhysicsBody.get(entity) -> PhysicsBody or null
void wren_PhysicsBodyGet(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);
	WrenBodyDefinition* bodyDef = static_cast<WrenBodyDefinition*>(wrenGetSlotForeign(vm, 2));
	WrenPhysicsCircleShape* shape = static_cast<WrenPhysicsCircleShape*>(wrenGetSlotForeign(vm, 3));

	auto* physicsBodyComponent = registry.try_get<Struktur::Component::PhysicsBody>(entity);

	if (physicsBodyComponent)
	{
		wrenGetVariable(vm, "game", "PhysicsBody", 1);  // Get class into slot 1
		WrenPhysicsBody* physicsBody = (WrenPhysicsBody*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenPhysicsBody));

		// PhysicsBody.new() - identity
		new (physicsBody) WrenPhysicsBody(entity, physicsBodyComponent);
	}
	else
	{
		wrenSetSlotNull(vm, 0);
	}
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("game", "PhysicsBody", wren_PhysicsBodyAllocate, wren_PhysicsBodyFinalize, "PhysicsBody class wraps PhysicsBody component");

// Register methods
WREN_CLASS_METHOD("game", "PhysicsBody", "fixedRotation=(_)", wren_PhysicsBodySetFixedRotation, "Sets the physics body fixed rotation");
WREN_CLASS_METHOD("game", "PhysicsBody", "syncFromPhysics", wren_PhysicsBodyGetSyncFromPhysics, "Get if physics bodys to transform sync with the physics position");
WREN_CLASS_METHOD("game", "PhysicsBody", "syncFromPhysics=(_)", wren_PhysicsBodySetSyncFromPhysics, "Set if physics bodys to transform sync with the physics position");
WREN_CLASS_METHOD("game", "PhysicsBody", "syncToPhysics", wren_PhysicsBodyGetSyncToPhysics, "Get if physics bodys to transform sync with the physics position");
WREN_CLASS_METHOD("game", "PhysicsBody", "syncToPhysics=(_)", wren_PhysicsBodySetSyncToPhysics, "Set if physics bodys to transform sync with the physics position");
WREN_CLASS_METHOD("game", "PhysicsBody", "linearVelocity", wren_PhysicsBodySetLinearVelocity, "Sets the linear velocity of a physics body.");

// Register static methods
WREN_CLASS_STATIC("game", "PhysicsBody", "create(_,_,_)", wren_PhysicsBodyCreate, "Create a physics body");
WREN_CLASS_STATIC("game", "PhysicsBody", "get(_)", wren_PhysicsBodyGet, "Gets a physics body");
WREN_CLASS_STATIC("game", "PhysicsBody", "setLinearVelocity(_,_)", wren_PhysicsBodyStaticSetLinearVelocity, "Sets the linear velocity of a physics body.");

// ============================================================================
// SPRITE ANIMATION DEFINITION BINDINGS
// ============================================================================
#include "Engine/ECS/System/AnimationSystem.h"

struct WrenSpriteAnimationDefinition
{
	Struktur::Animation::SpriteAnimation spriteAnimation;

	WrenSpriteAnimationDefinition() : spriteAnimation() {}
	WrenSpriteAnimationDefinition(const Struktur::Animation::SpriteAnimation& spriteAnimation) : spriteAnimation(spriteAnimation) {}
};

// Allocator
void wren_SpriteAnimationDefinitionAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "SpriteAnimationDefinition", 0);  // Get class into slot 1
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenSpriteAnimationDefinition));

	// Initialise with constructor parameters if provided
	if (wrenGetSlotCount(vm) >= 3)
	{
		unsigned int startFrame = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
		unsigned int endFrame = static_cast<unsigned int>(wrenGetSlotDouble(vm, 2));
		float animationTime = static_cast<float>(wrenGetSlotDouble(vm, 3));
		bool loop = wrenGetSlotBool(vm, 4);
		new (spriteAnimation) WrenSpriteAnimationDefinition({ startFrame, endFrame, animationTime, loop });
	}
	else
	{
		new (spriteAnimation) WrenSpriteAnimationDefinition();
	}
}

// Finalizer
void wren_SpriteAnimationDefinitionFinalize(void* data)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)data;
	spriteAnimation->~WrenSpriteAnimationDefinition();
}

void wren_SpriteAnimationGetStartFrame(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, spriteAnimation->spriteAnimation.startFrame);
}

void wren_SpriteAnimationSetStartFrame(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	spriteAnimation->spriteAnimation.startFrame = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
}

void wren_SpriteAnimationGetEndFrame(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, spriteAnimation->spriteAnimation.endFrame);
}

void wren_SpriteAnimationSetEndFrame(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	spriteAnimation->spriteAnimation.endFrame = static_cast<unsigned int>(wrenGetSlotDouble(vm, 1));
}

void wren_SpriteAnimationGetAnimationTime(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotDouble(vm, 0, spriteAnimation->spriteAnimation.animationTime);
}

void wren_SpriteAnimationSetAnimationTime(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	spriteAnimation->spriteAnimation.animationTime = static_cast<float>(wrenGetSlotDouble(vm, 1));
}

void wren_SpriteAnimationGetLoop(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	wrenSetSlotBool(vm, 0, spriteAnimation->spriteAnimation.loop);
}

void wren_SpriteAnimationSetLoop(WrenVM* vm)
{
	WrenSpriteAnimationDefinition* spriteAnimation = (WrenSpriteAnimationDefinition*)wrenGetSlotForeign(vm, 0);
	spriteAnimation->spriteAnimation.loop = wrenGetSlotBool(vm, 1);
}

// Register Quat foreign class
WREN_FOREIGN_CLASS("game", "SpriteAnimationDefinition", wren_SpriteAnimationDefinitionAllocate, wren_SpriteAnimationDefinitionFinalize, "SpriteAnimationDefinition class for defining the frames of a sprite sheet and speed of a sprite animation");

// Register constructors
WREN_CONSTRUCTOR_DOC("game", "SpriteAnimationDefinition", wren_SpriteAnimationDefinitionAllocate, "Create empty SpriteAnimationDefinition", );
WREN_CONSTRUCTOR_DOC("game", "SpriteAnimationDefinition", wren_SpriteAnimationDefinitionAllocate, "Create SpriteAnimationDefinition with startFrame, endFrame, animationTime, loop components", startFrame, endFrame, animationTime, loop);

// Register methods
WREN_CLASS_METHOD("game", "SpriteAnimationDefinition", "startFrame", wren_SpriteAnimationGetStartFrame, "Get startFrame");
WREN_CLASS_METHOD("game", "SpriteAnimationDefinition", "startFrame=(_)", wren_SpriteAnimationSetStartFrame, "Set startFrame");
WREN_CLASS_METHOD("game", "SpriteAnimationDefinition", "endFrame", wren_SpriteAnimationGetEndFrame, "Get endFrame");
WREN_CLASS_METHOD("game", "SpriteAnimationDefinition", "endFrame=(_)", wren_SpriteAnimationSetEndFrame, "Set endFrame");
WREN_CLASS_METHOD("game", "SpriteAnimationDefinition", "animationTime", wren_SpriteAnimationGetAnimationTime, "Get animationTime");
WREN_CLASS_METHOD("game", "SpriteAnimationDefinition", "animationTime=(_)", wren_SpriteAnimationSetAnimationTime, "Set animationTime");
WREN_CLASS_METHOD("game", "SpriteAnimationDefinition", "loop", wren_SpriteAnimationGetLoop, "Get loop");
WREN_CLASS_METHOD("game", "SpriteAnimationDefinition", "loop=(_)", wren_SpriteAnimationSetLoop, "Set loop");

// ============================================================================
// SPRITE ANIMATION BINDINGS
// ============================================================================

struct WrenSpriteAnimation
{
	entt::entity entity = entt::null;
	Struktur::Component::SpriteAnimation* spriteAnimation = nullptr;

	WrenSpriteAnimation() {}
	WrenSpriteAnimation(entt::entity entity, Struktur::Component::SpriteAnimation* spriteAnimation) : entity(entity), spriteAnimation(spriteAnimation) {}
};

// Allocator
void wren_SpriteAnimationAllocate(WrenVM* vm)
{
	wrenGetVariable(vm, "game", "SpriteAnimation", 0);  // Get class into slot 1
	WrenSpriteAnimation* spriteAnimation = (WrenSpriteAnimation*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenSpriteAnimation));

	// SpriteAnimation.new() - identity
	new (spriteAnimation) WrenSpriteAnimation();
}

// Finalizer
void wren_SpriteAnimationFinalize(void* data)
{
	WrenSpriteAnimation* spriteAnimation = (WrenSpriteAnimation*)data;
	spriteAnimation->~WrenSpriteAnimation();
}


// SpriteAnimation.create(entity) -> PhysicsBody
void wren_SpriteAnimationCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	Struktur::Component::SpriteAnimation& spriteAnimationComponent = registry.emplace<Struktur::Component::SpriteAnimation>(entity);

	wrenGetVariable(vm, "game", "SpriteAnimation", 1);  // Get class into slot 1
	WrenSpriteAnimation* spriteAnimation = (WrenSpriteAnimation*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenSpriteAnimation));

	// SpriteAnimation.new() - identity
	new (spriteAnimation) WrenSpriteAnimation(entity, &spriteAnimationComponent);
}

// SpriteAnimation.get(entity) -> PhysicsBody or null
void wren_SpriteAnimationGet(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	entt::entity entity = static_cast<entt::entity>(entityId);

	auto* spriteAnimationComponent = registry.try_get<Struktur::Component::SpriteAnimation>(entity);

	if (spriteAnimationComponent)
	{
		wrenGetVariable(vm, "game", "SpriteAnimation", 1);  // Get class into slot 1
		WrenSpriteAnimation* physicsBody = (WrenSpriteAnimation*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenSpriteAnimation));

		// SpriteAnimation.new() - identity
		new (physicsBody) WrenSpriteAnimation(entity, spriteAnimationComponent);
	}
	else
	{
		wrenSetSlotNull(vm, 0);
	}
}

// SpriteAnimation.addAnimation(animationKey, animationDefinition)
void wren_SpriteAnimationAddAnimation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	Struktur::System::AnimationSystem& animationSystem = systemManager.GetSystem<Struktur::System::AnimationSystem>();

	auto* spriteAnimation = static_cast<WrenSpriteAnimation*>(wrenGetSlotForeign(vm, 0));
	const char* animationKey = wrenGetSlotString(vm, 1);
	auto* animationDefinition = static_cast<WrenSpriteAnimationDefinition*>(wrenGetSlotForeign(vm, 2));

	animationSystem.AddAnimation(*context, spriteAnimation->entity, animationKey, animationDefinition->spriteAnimation);
}

// SpriteAnimation.setCurrentAnimation(entity, animationName)
void wren_SpriteAnimationStaticSetCurrentAnimation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& animationSystem = systemManager.GetSystem<Struktur::System::AnimationSystem>();
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	const char* animationName = wrenGetSlotString(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	if (!animationSystem.IsAnimationPlaying(*context, entity, animationName))
	{
		animationSystem.PlayAnimation(*context, entity, animationName);
	}
}

// SpriteAnimation.playAnimation(entity, animationName)
void wren_SpriteAnimationPlayAnimation(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& animationSystem = systemManager.GetSystem<Struktur::System::AnimationSystem>();
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	const char* animationName = wrenGetSlotString(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	animationSystem.PlayAnimation(*context, entity, animationName);
}

// SpriteAnimation.isAnimationPlaying(entity, animationName) -> bool
void wren_SpriteAnimationIsAnimationPlaying(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::System::SystemManager& systemManager = context->GetSystemManager();
	auto& animationSystem = systemManager.GetSystem<Struktur::System::AnimationSystem>();
	auto& registry = context->GetRegistry();

	double entityId = wrenGetSlotDouble(vm, 1);
	const char* animationName = wrenGetSlotString(vm, 2);
	entt::entity entity = static_cast<entt::entity>(entityId);

	bool isAnimationPlaying = animationSystem.IsAnimationPlaying(*context, entity, animationName);
	wrenSetSlotBool(vm, 0, isAnimationPlaying);
}

// Register BodyDefinition foreign class
WREN_FOREIGN_CLASS("game", "SpriteAnimation", wren_SpriteAnimationAllocate, wren_SpriteAnimationFinalize, "Sprite animation class wraps SpriteAnimation component");

// Register methods
WREN_CLASS_METHOD("game", "SpriteAnimation", "addAnimation(_,_)", wren_SpriteAnimationAddAnimation, "Adds an animation to the sprite animation component");

// Register static methods
WREN_CLASS_STATIC("game", "SpriteAnimation", "create(_)", wren_SpriteAnimationCreate, "Create a Sprite animation");
WREN_CLASS_STATIC("game", "SpriteAnimation", "get(_)", wren_SpriteAnimationGet, "Gets a Sprite animation");
WREN_CLASS_STATIC("game", "SpriteAnimation", "setCurrentAnimation(_,_)", wren_SpriteAnimationStaticSetCurrentAnimation, "Will set and play a current sprite animation, is already playing the animation continue it.");
WREN_CLASS_STATIC("game", "SpriteAnimation", "forcePlayAnimation(_,_)", wren_SpriteAnimationPlayAnimation, "Will play a sprite animation, and if playering animation will forcibly restart it.");
WREN_CLASS_STATIC("game", "SpriteAnimation", "isAnimationPlaying(_,_)", wren_SpriteAnimationIsAnimationPlaying, "Checks if a cirtain animation is playing.");
