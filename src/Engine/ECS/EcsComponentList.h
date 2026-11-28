#pragma once

// The ECS library's script components (see ScriptComponents.h).
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/WrenScript.h"

#define ECS_COMPONENT_LIST                                 \
	COMPONENT(LocalTransform, Transform, "LocalTransform") \
	COMPONENT(WorldTransform, Transform, "WorldTransform") \
	COMPONENT(WrenScript, WrenScript, "Script")
