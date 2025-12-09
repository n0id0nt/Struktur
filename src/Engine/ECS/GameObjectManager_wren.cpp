#include "GameObjectManager.h"

#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"

#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Shader.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/WrenScript.h"

// ============================================================================
// Vec2 - Foreign class wrapping glm::vec2
// ============================================================================

struct WrenVec2
{
    glm::vec2 value;
    
    WrenVec2() : value(0.0f, 0.0f) {}
    WrenVec2(float x, float y) : value(x, y) {}
    WrenVec2(const glm::vec2& v) : value(v) {}
};

// Allocator - called when Vec2.new() is invoked
void wren_Vec2Allocate(WrenVM* vm)
{
    // Allocate foreign object
    wrenGetVariable(vm, "game", "Vec2", 0);  // Get class into slot 1
    WrenVec2* vec = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenVec2));
    
    // Initialize with constructor parameters if provided
    if (wrenGetSlotCount(vm) >= 3)
	{
        float x = (float)wrenGetSlotDouble(vm, 1);
        float y = (float)wrenGetSlotDouble(vm, 2);
        new (vec) WrenVec2(x, y);
    }
	else
	{
        new (vec) WrenVec2();
    }
}

// Finalizer - called when garbage collected
void wren_Vec2Finalize(void* data)
{
    WrenVec2* vec = (WrenVec2*)data;
    vec->~WrenVec2();
}

// Getters
void wren_Vec2GetX(WrenVM* vm)
{
    WrenVec2* vec = (WrenVec2*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, vec->value.x);
}

void wren_Vec2GetY(WrenVM* vm)
{
    WrenVec2* vec = (WrenVec2*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, vec->value.y);
}

// Setters
void wren_Vec2SetX(WrenVM* vm)
{
    WrenVec2* vec = (WrenVec2*)wrenGetSlotForeign(vm, 0);
    vec->value.x = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec2SetY(WrenVM* vm)
{
    WrenVec2* vec = (WrenVec2*)wrenGetSlotForeign(vm, 0);
    vec->value.y = (float)wrenGetSlotDouble(vm, 1);
}

// toString()
void wren_Vec2ToString(WrenVM* vm)
{
    WrenVec2* vec = (WrenVec2*)wrenGetSlotForeign(vm, 0);
    
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Vec2(%.2f, %.2f)", vec->value.x, vec->value.y);
    
    wrenSetSlotString(vm, 0, buffer);
}

// Math operations
void wren_Vec2Add(WrenVM* vm)
{
    WrenVec2* a = (WrenVec2*)wrenGetSlotForeign(vm, 0);
    WrenVec2* b = (WrenVec2*)wrenGetSlotForeign(vm, 1);
    
    // Create new Vec2 with result
    wrenGetVariable(vm, "game", "Vec2", 1);  // Get class into slot 1
    WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
    new (result) WrenVec2(a->value + b->value);
}

void wren_Vec2Subtract(WrenVM* vm)
{
    WrenVec2* a = (WrenVec2*)wrenGetSlotForeign(vm, 0);
    WrenVec2* b = (WrenVec2*)wrenGetSlotForeign(vm, 1);
    
    wrenGetVariable(vm, "game", "Vec2", 1);  // Get class into slot 1
    WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
    new (result) WrenVec2(a->value - b->value);
}

void wren_Vec2Multiply(WrenVM* vm)
{
    WrenVec2* vec = (WrenVec2*)wrenGetSlotForeign(vm, 0);
    float scalar = (float)wrenGetSlotDouble(vm, 1);
    
    wrenGetVariable(vm, "game", "Vec2", 1);  // Get class into slot 1
    WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
    new (result) WrenVec2(vec->value * scalar);
}

void wren_Vec2Length(WrenVM* vm)
{
    WrenVec2* vec = (WrenVec2*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, glm::length(vec->value));
}

void wren_Vec2Normalize(WrenVM* vm)
{
    WrenVec2* vec = (WrenVec2*)wrenGetSlotForeign(vm, 0);
    
    wrenGetVariable(vm, "game", "Vec2", 1);  // Get class into slot 1
    WrenVec2* result = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
    new (result) WrenVec2(glm::normalize(vec->value));
}

void wren_Vec2Dot(WrenVM* vm)
{
    WrenVec2* a = (WrenVec2*)wrenGetSlotForeign(vm, 0);
    WrenVec2* b = (WrenVec2*)wrenGetSlotForeign(vm, 1);
    
    wrenSetSlotDouble(vm, 0, glm::dot(a->value, b->value));
}

// Register Vec2 foreign class
WREN_FOREIGN_CLASS("game", "Vec2", wren_Vec2Allocate, wren_Vec2Finalize, "2D vector class wrapping glm::vec2");

// Register constructors
WREN_CONSTRUCTOR_DOC("game", "Vec2", wren_Vec2Allocate, "Create zero vector", );
WREN_CONSTRUCTOR_DOC("game", "Vec2", wren_Vec2Allocate, "Create vector with x, y components", x, y);

// Register methods
WREN_CLASS_METHOD("game", "Vec2", "x", wren_Vec2GetX, "Get X component");
WREN_CLASS_METHOD("game", "Vec2", "y", wren_Vec2GetY, "Get Y component");
WREN_CLASS_METHOD("game", "Vec2", "x=(_)", wren_Vec2SetX, "Set X component");
WREN_CLASS_METHOD("game", "Vec2", "y=(_)", wren_Vec2SetY, "Set Y component");
WREN_CLASS_METHOD("game", "Vec2", "toString", wren_Vec2ToString, "Convert to string");
WREN_CLASS_METHOD("game", "Vec2", "+(_)", wren_Vec2Add, "Add two vectors");
WREN_CLASS_METHOD("game", "Vec2", "-(_)", wren_Vec2Subtract, "Subtract vectors");
WREN_CLASS_METHOD("game", "Vec2", "*(_)", wren_Vec2Multiply, "Multiply by scalar");
WREN_CLASS_METHOD("game", "Vec2", "length", wren_Vec2Length, "Get vector length");
WREN_CLASS_METHOD("game", "Vec2", "normalize()", wren_Vec2Normalize, "Get normalized vector");
WREN_CLASS_METHOD("game", "Vec2", "dot(_)", wren_Vec2Dot, "Dot product");

// ============================================================================
// Vec3 - Foreign class wrapping glm::vec3
// ============================================================================

struct WrenVec3
{
    glm::vec3 value;
    
    WrenVec3() : value(0.0f, 0.0f, 0.0f) {}
    WrenVec3(float x, float y, float z) : value(x, y, z) {}
    WrenVec3(const glm::vec3& v) : value(v) {}
};

void wren_Vec3Allocate(WrenVM* vm)
{
    wrenGetVariable(vm, "game", "Vec3", 0);  // Get class into slot 4
    WrenVec3* vec = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenVec3));
    
    if (wrenGetSlotCount(vm) >= 4)
	{
        float x = (float)wrenGetSlotDouble(vm, 1);
        float y = (float)wrenGetSlotDouble(vm, 2);
        float z = (float)wrenGetSlotDouble(vm, 3);
        new (vec) WrenVec3(x, y, z);
    }
	else
	{
        new (vec) WrenVec3();
    }
}

void wren_Vec3Finalize(void* data)
{
    WrenVec3* vec = (WrenVec3*)data;
    vec->~WrenVec3();
}

void wren_Vec3GetX(WrenVM* vm)
{
    WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, vec->value.x);
}

void wren_Vec3GetY(WrenVM* vm)
{
    WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, vec->value.y);
}

void wren_Vec3GetZ(WrenVM* vm)
{
    WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, vec->value.z);
}

void wren_Vec3SetX(WrenVM* vm)
{
    WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    vec->value.x = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec3SetY(WrenVM* vm)
{
    WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    vec->value.y = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec3SetZ(WrenVM* vm)
{
    WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    vec->value.z = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec3ToString(WrenVM* vm)
{
    WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    
    char buffer[80];
    snprintf(buffer, sizeof(buffer), "Vec3(%.2f, %.2f, %.2f)", 
             vec->value.x, vec->value.y, vec->value.z);
    
    wrenSetSlotString(vm, 0, buffer);
}

void wren_Vec3Add(WrenVM* vm)
{
    WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 1);
    
    wrenGetVariable(vm, "game", "Vec3", 1);  // Get class into slot 1
    WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
    new (result) WrenVec3(a->value + b->value);
}

void wren_Vec3Subtract(WrenVM* vm)
{
    WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 1);
    
    wrenGetVariable(vm, "game", "Vec3", 1);  // Get class into slot 1
    WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
    new (result) WrenVec3(a->value - b->value);
}

void wren_Vec3Multiply(WrenVM* vm)
{
    WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    float scalar = (float)wrenGetSlotDouble(vm, 1);
    
    wrenGetVariable(vm, "game", "Vec3", 1);  // Get class into slot 1
    WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
    new (result) WrenVec3(vec->value * scalar);
}

void wren_Vec3Length(WrenVM* vm)
{
    WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, glm::length(vec->value));
}

void wren_Vec3Normalize(WrenVM* vm)
{
    WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    
    wrenGetVariable(vm, "game", "Vec3", 1);  // Get class into slot 1
    WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
    new (result) WrenVec3(glm::normalize(vec->value));
}

void wren_Vec3Dot(WrenVM* vm)
{
    WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 1);
    
    wrenSetSlotDouble(vm, 0, glm::dot(a->value, b->value));
}

void wren_Vec3Cross(WrenVM* vm)
{
    WrenVec3* a = (WrenVec3*)wrenGetSlotForeign(vm, 0);
    WrenVec3* b = (WrenVec3*)wrenGetSlotForeign(vm, 1);
    
    wrenGetVariable(vm, "game", "Vec3", 1);  // Get class into slot 1
    WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
    new (result) WrenVec3(glm::cross(a->value, b->value));
}

// Register Vec3 foreign class
WREN_FOREIGN_CLASS("game", "Vec3", wren_Vec3Allocate, wren_Vec3Finalize, "3D vector class wrapping glm::vec3");

// Register constructors
WREN_CONSTRUCTOR_DOC("game", "Vec3", wren_Vec3Allocate, "Create zero vector", );
WREN_CONSTRUCTOR_DOC("game", "Vec3", wren_Vec3Allocate, "Create vector with x, y, z components", x, y, z);

// Register methods
WREN_CLASS_METHOD("game", "Vec3", "x", wren_Vec3GetX, "Get X component");
WREN_CLASS_METHOD("game", "Vec3", "y", wren_Vec3GetY, "Get Y component");
WREN_CLASS_METHOD("game", "Vec3", "z", wren_Vec3GetZ, "Get Z component");
WREN_CLASS_METHOD("game", "Vec3", "x=(_)", wren_Vec3SetX, "Set X component");
WREN_CLASS_METHOD("game", "Vec3", "y=(_)", wren_Vec3SetY, "Set Y component");
WREN_CLASS_METHOD("game", "Vec3", "z=(_)", wren_Vec3SetZ, "Set Z component");
WREN_CLASS_METHOD("game", "Vec3", "toString", wren_Vec3ToString, "Convert to string");
WREN_CLASS_METHOD("game", "Vec3", "+(_)", wren_Vec3Add, "Add two vectors");
WREN_CLASS_METHOD("game", "Vec3", "-(_)", wren_Vec3Subtract, "Subtract vectors");
WREN_CLASS_METHOD("game", "Vec3", "*(_)", wren_Vec3Multiply, "Multiply by scalar");
WREN_CLASS_METHOD("game", "Vec3", "length", wren_Vec3Length, "Get vector length");
WREN_CLASS_METHOD("game", "Vec3", "normalize()", wren_Vec3Normalize, "Get normalized vector");
WREN_CLASS_METHOD("game", "Vec3", "dot(_)", wren_Vec3Dot, "Dot product");
WREN_CLASS_METHOD("game", "Vec3", "cross(_)", wren_Vec3Cross, "Cross product");

// ============================================================================
// Vec4 - Foreign class wrapping glm::vec4
// ============================================================================

struct WrenVec4
{
    glm::vec4 value;
    
    WrenVec4() : value(0.0f, 0.0f, 0.0f, 0.0f) {}
    WrenVec4(float x, float y, float z, float w) : value(x, y, z, w) {}
    WrenVec4(const glm::vec4& v) : value(v) {}
};

void wren_Vec4Allocate(WrenVM* vm)
{
    wrenGetVariable(vm, "game", "Vec4", 0);  // Get class into slot 1
    WrenVec4* vec = static_cast<WrenVec4*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenVec4)));
    
    if (wrenGetSlotCount(vm) >= 5)
	{
        float x = static_cast<int>(wrenGetSlotDouble(vm, 1));
        float y = static_cast<int>(wrenGetSlotDouble(vm, 2));
        float z = static_cast<int>(wrenGetSlotDouble(vm, 3));
        float w = static_cast<int>(wrenGetSlotDouble(vm, 4));
        new (vec) WrenVec4(x, y, z, w);
    }
	else
	{
        new (vec) WrenVec4();
    }
}

void wren_Vec4Finalize(void* data)
{
    WrenVec4* vec = (WrenVec4*)data;
    vec->~WrenVec4();
}

void wren_Vec4GetX(WrenVM* vm)
{
    WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, vec->value.x);
}

void wren_Vec4GetY(WrenVM* vm)
{
    WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, vec->value.y);
}

void wren_Vec4GetZ(WrenVM* vm)
{
    WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, vec->value.z);
}

void wren_Vec4GetW(WrenVM* vm)
{
    WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, vec->value.w);
}

void wren_Vec4SetX(WrenVM* vm)
{
    WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    vec->value.x = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec4SetY(WrenVM* vm)
{
    WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    vec->value.y = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec4SetZ(WrenVM* vm)
{
    WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    vec->value.z = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec4SetW(WrenVM* vm)
{
    WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    vec->value.w = (float)wrenGetSlotDouble(vm, 1);
}

void wren_Vec4ToString(WrenVM* vm)
{
    WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    
    char buffer[80];
    snprintf(buffer, sizeof(buffer), "Vec4(%.2f, %.2f, %.2f)", 
             vec->value.x, vec->value.y, vec->value.z);
    
    wrenSetSlotString(vm, 0, buffer);
}

void wren_Vec4Add(WrenVM* vm)
{
    WrenVec4* a = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    WrenVec4* b = (WrenVec4*)wrenGetSlotForeign(vm, 1);
    
    wrenGetVariable(vm, "game", "Vec4", 1);  // Get class into slot 1
    WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
    new (result) WrenVec4(a->value + b->value);
}

void wren_Vec4Subtract(WrenVM* vm)
{
    WrenVec4* a = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    WrenVec4* b = (WrenVec4*)wrenGetSlotForeign(vm, 1);
    
    wrenGetVariable(vm, "game", "Vec4", 1);  // Get class into slot 1
    WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
    new (result) WrenVec4(a->value - b->value);
}

void wren_Vec4Multiply(WrenVM* vm)
{
    WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    float scalar = (float)wrenGetSlotDouble(vm, 1);
    
    wrenGetVariable(vm, "game", "Vec4", 1);  // Get class into slot 1
    WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
    new (result) WrenVec4(vec->value * scalar);
}

void wren_Vec4Length(WrenVM* vm)
{
    WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, glm::length(vec->value));
}

void wren_Vec4Normalize(WrenVM* vm)
{
    WrenVec4* vec = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    
    wrenGetVariable(vm, "game", "Vec4", 1);  // Get class into slot 1
    WrenVec4* result = (WrenVec4*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec4));
    new (result) WrenVec4(glm::normalize(vec->value));
}

void wren_Vec4Dot(WrenVM* vm)
{
    WrenVec4* a = (WrenVec4*)wrenGetSlotForeign(vm, 0);
    WrenVec4* b = (WrenVec4*)wrenGetSlotForeign(vm, 1);
    
    wrenSetSlotDouble(vm, 0, glm::dot(a->value, b->value));
}

// Register Vec3 foreign class
WREN_FOREIGN_CLASS("game", "Vec4", wren_Vec4Allocate, wren_Vec4Finalize, "4D vector class wrapping glm::vec4");

// Register constructors
WREN_CONSTRUCTOR_DOC("game", "Vec4", wren_Vec4Allocate, "Create zero vector", );
WREN_CONSTRUCTOR_DOC("game", "Vec4", wren_Vec4Allocate, "Create vector with x, y, z, w components", x, y, z, w);

// Register methods
WREN_CLASS_METHOD("game", "Vec4", "x", wren_Vec4GetX, "Get X component");
WREN_CLASS_METHOD("game", "Vec4", "y", wren_Vec4GetY, "Get Y component");
WREN_CLASS_METHOD("game", "Vec4", "z", wren_Vec4GetZ, "Get Z component");
WREN_CLASS_METHOD("game", "Vec4", "w", wren_Vec4GetW, "Get W component");
WREN_CLASS_METHOD("game", "Vec4", "x=(_)", wren_Vec4SetX, "Set X component");
WREN_CLASS_METHOD("game", "Vec4", "y=(_)", wren_Vec4SetY, "Set Y component");
WREN_CLASS_METHOD("game", "Vec4", "z=(_)", wren_Vec4SetZ, "Set Z component");
WREN_CLASS_METHOD("game", "Vec4", "w=(_)", wren_Vec4SetW, "Set W component");
WREN_CLASS_METHOD("game", "Vec4", "toString", wren_Vec4ToString, "Convert to string");
WREN_CLASS_METHOD("game", "Vec4", "+(_)", wren_Vec4Add, "Add two vectors");
WREN_CLASS_METHOD("game", "Vec4", "-(_)", wren_Vec4Subtract, "Subtract vectors");
WREN_CLASS_METHOD("game", "Vec4", "*(_)", wren_Vec4Multiply, "Multiply by scalar");
WREN_CLASS_METHOD("game", "Vec4", "length", wren_Vec4Length, "Get vector length");
WREN_CLASS_METHOD("game", "Vec4", "normalize()", wren_Vec4Normalize, "Get normalized vector");
WREN_CLASS_METHOD("game", "Vec4", "dot(_)", wren_Vec4Dot, "Dot product");
// ============================================================================
// Quat - Foreign class wrapping glm::quat
// ============================================================================

struct WrenQuat {
    glm::quat value;
    
    WrenQuat() : value(1.0f, 0.0f, 0.0f, 0.0f) {}  // Identity quaternion
    WrenQuat(float w, float x, float y, float z) : value(w, x, y, z) {}
    WrenQuat(const glm::quat& q) : value(q) {}
};

// Allocator
void wren_QuatAllocate(WrenVM* vm)
{
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
    
    if (wrenGetSlotCount(vm) >= 5)
	{
        // Quat.new(w, x, y, z)
        float w = (float)wrenGetSlotDouble(vm, 1);
        float x = (float)wrenGetSlotDouble(vm, 2);
        float y = (float)wrenGetSlotDouble(vm, 3);
        float z = (float)wrenGetSlotDouble(vm, 4);
        new (quat) WrenQuat(w, x, y, z);
    } else {
        // Quat.new() - identity
        new (quat) WrenQuat();
    }
}

// Finalizer
void wren_QuatFinalize(void* data)
{
    WrenQuat* quat = (WrenQuat*)data;
    quat->~WrenQuat();
}

// Component getters
void wren_QuatGetW(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, quat->value.w);
}

void wren_QuatGetX(WrenVM* vm) 
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, quat->value.x);
}

void wren_QuatGetY(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, quat->value.y);
}

void wren_QuatGetZ(WrenVM* vm) 
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, quat->value.z);
}

// Component setters
void wren_QuatSetW(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    quat->value.w = (float)wrenGetSlotDouble(vm, 1);
}

void wren_QuatSetX(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    quat->value.x = (float)wrenGetSlotDouble(vm, 1);
}

void wren_QuatSetY(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    quat->value.y = (float)wrenGetSlotDouble(vm, 1);
}

void wren_QuatSetZ(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    quat->value.z = (float)wrenGetSlotDouble(vm, 1);
}

// toString
void wren_QuatToString(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    
    char buffer[96];
    snprintf(buffer, sizeof(buffer), "Quat(%.2f, %.2f, %.2f, %.2f)", 
             quat->value.w, quat->value.x, quat->value.y, quat->value.z);
    
    wrenSetSlotString(vm, 0, buffer);
}

// Static constructors for common rotations

// Quat.identity() -> Quat
void wren_QuatIdentity(WrenVM* vm)
{
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
    new (quat) WrenQuat(1.0f, 0.0f, 0.0f, 0.0f);
}

// Quat.fromAxisAngle(axis, angle) -> Quat
void wren_QuatFromAxisAngle(WrenVM* vm)
{
    WrenVec3* axis = (WrenVec3*)wrenGetSlotForeign(vm, 1);
    float angle = (float)wrenGetSlotDouble(vm, 2);
    
    if (!axis)
	{
        DEBUG_ERROR("Quat.fromAxisAngle: Invalid axis");
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
    new (quat) WrenQuat(glm::angleAxis(angle, glm::normalize(axis->value)));
}

// Quat.fromEuler(x, y, z) -> Quat (angles in radians)
void wren_QuatFromEuler(WrenVM* vm)
{
    float x = (float)wrenGetSlotDouble(vm, 1);
    float y = (float)wrenGetSlotDouble(vm, 2);
    float z = (float)wrenGetSlotDouble(vm, 3);
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
    new (quat) WrenQuat(glm::quat(glm::vec3(x, y, z)));
}

// Quat.fromEulerDegrees(x, y, z) -> Quat (angles in degrees)
void wren_QuatFromEulerDegrees(WrenVM* vm)
{
    float x = glm::radians((float)wrenGetSlotDouble(vm, 1));
    float y = glm::radians((float)wrenGetSlotDouble(vm, 2));
    float z = glm::radians((float)wrenGetSlotDouble(vm, 3));
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
    new (quat) WrenQuat(glm::quat(glm::vec3(x, y, z)));
}

// Quat.lookAt(forward, up) -> Quat
void wren_QuatLookAt(WrenVM* vm)
{
    WrenVec3* forward = (WrenVec3*)wrenGetSlotForeign(vm, 1);
    WrenVec3* up = (WrenVec3*)wrenGetSlotForeign(vm, 2);
    
    if (!forward || !up)
	{
        DEBUG_ERROR("Quat.lookAt: Invalid vectors");
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenQuat* quat = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
    new (quat) WrenQuat(glm::quatLookAt(glm::normalize(forward->value), up->value));
}

// Instance methods

// quat.normalize() -> Quat
void wren_QuatNormalize(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenQuat* result = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
    new (result) WrenQuat(glm::normalize(quat->value));
}

// quat.inverse() -> Quat
void wren_QuatInverse(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenQuat* result = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
    new (result) WrenQuat(glm::inverse(quat->value));
}

// quat.conjugate() -> Quat
void wren_QuatConjugate(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenQuat* result = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
    new (result) WrenQuat(glm::conjugate(quat->value));
}

// quat.length -> Num
void wren_QuatLength(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    wrenSetSlotDouble(vm, 0, glm::length(quat->value));
}

// quat.toEuler() -> Vec3 (radians)
void wren_QuatToEuler(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    
    glm::vec3 euler = glm::eulerAngles(quat->value);
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
    new (result) WrenVec3(euler);
}

// quat.toEulerDegrees() -> Vec3 (degrees)
void wren_QuatToEulerDegrees(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    
    glm::vec3 euler = glm::eulerAngles(quat->value);
    euler = glm::degrees(euler);
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
    new (result) WrenVec3(euler);
}

// quat.toAxisAngle() -> Map {"axis": Vec3, "angle": Num}
void wren_QuatToAxisAngle(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    
    float angle = glm::angle(quat->value);
    glm::vec3 axis = glm::axis(quat->value);
    
    // Create map
    wrenSetSlotNewMap(vm, 0);
    
    // Add axis
    wrenSetSlotString(vm, 1, "axis");
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenVec3* axisVec = (WrenVec3*)wrenSetSlotNewForeign(vm, 2, 1, sizeof(WrenVec3));
    new (axisVec) WrenVec3(axis);
    wrenSetMapValue(vm, 0, 1, 2);
    
    // Add angle
    wrenSetSlotString(vm, 1, "angle");
    wrenSetSlotDouble(vm, 2, angle);
    wrenSetMapValue(vm, 0, 1, 2);
}

// Operators

// quat * quat -> Quat (quaternion multiplication)
void wren_QuatMultiply(WrenVM* vm)
{
    WrenQuat* a = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    WrenQuat* b = (WrenQuat*)wrenGetSlotForeign(vm, 1);
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenQuat* result = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
    new (result) WrenQuat(a->value * b->value);
}

// quat.rotate(vec3) -> Vec3 (rotate vector by quaternion)
void wren_QuatRotateVec3(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    WrenVec3* vec = (WrenVec3*)wrenGetSlotForeign(vm, 1);
    
    if (!vec)
	{
        DEBUG_ERROR("Quat.rotate: Invalid Vec3");
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    glm::vec3 rotated = quat->value * vec->value;
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
    new (result) WrenVec3(rotated);
}

// Quat.slerp(a, b, t) -> Quat (spherical linear interpolation)
void wren_QuatSlerp(WrenVM* vm)
{
    WrenQuat* a = (WrenQuat*)wrenGetSlotForeign(vm, 1);
    WrenQuat* b = (WrenQuat*)wrenGetSlotForeign(vm, 2);
    float t = (float)wrenGetSlotDouble(vm, 3);
    
    if (!a || !b)
	{
        DEBUG_ERROR("Quat.slerp: Invalid quaternions");
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenQuat* result = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
    new (result) WrenQuat(glm::slerp(a->value, b->value, t));
}

// Quat.lerp(a, b, t) -> Quat (linear interpolation)
void wren_QuatLerp(WrenVM* vm)
{
    WrenQuat* a = (WrenQuat*)wrenGetSlotForeign(vm, 1);
    WrenQuat* b = (WrenQuat*)wrenGetSlotForeign(vm, 2);
    float t = (float)wrenGetSlotDouble(vm, 3);
    
    if (!a || !b)
	{
        DEBUG_ERROR("Quat.lerp: Invalid quaternions");
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenQuat* result = (WrenQuat*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenQuat));
    new (result) WrenQuat(glm::lerp(a->value, b->value, t));
}

// quat.dot(other) -> Num
void wren_QuatDot(WrenVM* vm)
{
    WrenQuat* a = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    WrenQuat* b = (WrenQuat*)wrenGetSlotForeign(vm, 1);
    
    if (!b)
	{
        DEBUG_ERROR("Quat.dot: Invalid quaternion");
        wrenSetSlotDouble(vm, 0, 0.0);
        return;
    }
    
    wrenSetSlotDouble(vm, 0, glm::dot(a->value, b->value));
}

// Helper: Get forward/up/right vectors from quaternion

// quat.forward() -> Vec3
void wren_QuatForward(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    
    glm::vec3 forward = quat->value * glm::vec3(0, 0, -1);
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
    new (result) WrenVec3(forward);
}

// quat.up() -> Vec3
void wren_QuatUp(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    
    glm::vec3 up = quat->value * glm::vec3(0, 1, 0);
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
    new (result) WrenVec3(up);
}

// Quat.right() -> Vec3
void wren_QuatRight(WrenVM* vm)
{
    WrenQuat* quat = (WrenQuat*)wrenGetSlotForeign(vm, 0);
    
    glm::vec3 right = quat->value * glm::vec3(1, 0, 0);
    
    wrenGetVariable(vm, "game", "Quat", 1);  // Get class into slot 1
    WrenVec3* result = (WrenVec3*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec3));
    new (result) WrenVec3(right);
}

// Register Quat foreign class
WREN_FOREIGN_CLASS("game", "Quat", wren_QuatAllocate, wren_QuatFinalize,
                   "Quaternion class for 3D rotations wrapping glm::quat");

// Register constructors
WREN_CONSTRUCTOR_DOC("game", "Quat", wren_QuatAllocate, "Create identity quaternion", );
WREN_CONSTRUCTOR_DOC("game", "Quat", wren_QuatAllocate, "Create quaternion with w, x, y, z components", w, x, y, z);

// Register instance methods
WREN_CLASS_METHOD("game", "Quat", "w", wren_QuatGetW, "Get W component");
WREN_CLASS_METHOD("game", "Quat", "x", wren_QuatGetX, "Get X component");
WREN_CLASS_METHOD("game", "Quat", "y", wren_QuatGetY, "Get Y component");
WREN_CLASS_METHOD("game", "Quat", "z", wren_QuatGetZ, "Get Z component");
WREN_CLASS_METHOD("game", "Quat", "w=(_)", wren_QuatSetW, "Set W component");
WREN_CLASS_METHOD("game", "Quat", "x=(_)", wren_QuatSetX, "Set X component");
WREN_CLASS_METHOD("game", "Quat", "y=(_)", wren_QuatSetY, "Set Y component");
WREN_CLASS_METHOD("game", "Quat", "z=(_)", wren_QuatSetZ, "Set Z component");
WREN_CLASS_METHOD("game", "Quat", "toString", wren_QuatToString, "Convert to string");
WREN_CLASS_METHOD("game", "Quat", "normalize()", wren_QuatNormalize, "Get normalized quaternion");
WREN_CLASS_METHOD("game", "Quat", "inverse()", wren_QuatInverse, "Get inverse quaternion");
WREN_CLASS_METHOD("game", "Quat", "conjugate()", wren_QuatConjugate, "Get conjugate quaternion");
WREN_CLASS_METHOD("game", "Quat", "length", wren_QuatLength, "Get quaternion length");
WREN_CLASS_METHOD("game", "Quat", "toEuler()", wren_QuatToEuler, "Convert to Euler angles (radians)");
WREN_CLASS_METHOD("game", "Quat", "toEulerDegrees()", wren_QuatToEulerDegrees, "Convert to Euler angles (degrees)");
WREN_CLASS_METHOD("game", "Quat", "toAxisAngle()", wren_QuatToAxisAngle, "Convert to axis-angle representation");
WREN_CLASS_METHOD("game", "Quat", "*(_)", wren_QuatMultiply, "Multiply quaternions");
WREN_CLASS_METHOD("game", "Quat", "rotate(_)", wren_QuatRotateVec3, "Rotate vector by quaternion");
WREN_CLASS_METHOD("game", "Quat", "dot(_)", wren_QuatDot, "Dot product");
WREN_CLASS_METHOD("game", "Quat", "forward()", wren_QuatForward, "Get forward direction vector");
WREN_CLASS_METHOD("game", "Quat", "up()", wren_QuatUp, "Get up direction vector");
WREN_CLASS_METHOD("game", "Quat", "right()", wren_QuatRight, "Get right direction vector");

// Register static methods
WREN_CLASS_STATIC("game", "Quat", "identity()", wren_QuatIdentity, "Create identity quaternion");
WREN_CLASS_STATIC("game", "Quat", "fromAxisAngle(_,_)", wren_QuatFromAxisAngle, "Create quaternion from axis and angle");
WREN_CLASS_STATIC("game", "Quat", "fromEuler(_,_,_)", wren_QuatFromEuler, "Create quaternion from Euler angles (radians)");
WREN_CLASS_STATIC("game", "Quat", "fromEulerDegrees(_,_,_)", wren_QuatFromEulerDegrees, "Create quaternion from Euler angles (degrees)");
WREN_CLASS_STATIC("game", "Quat", "lookAt(_,_)", wren_QuatLookAt, "Create quaternion from forward and up vectors");
WREN_CLASS_STATIC("game", "Quat", "slerp(_,_,_)", wren_QuatSlerp, "Spherical linear interpolation");
WREN_CLASS_STATIC("game", "Quat", "lerp(_,_,_)", wren_QuatLerp, "Linear interpolation");

// ============================================================================
// Texture Resource Handle
// ============================================================================

#include "Engine/Resource/TextureResource.h"

struct WrenTextureHandle
{
    Struktur::Resource::ResourcePtr<Struktur::Resource::TextureResource> resource;
    
    WrenTextureHandle() {}
    WrenTextureHandle(const Struktur::Resource::ResourcePtr<Struktur::Resource::TextureResource>& res) : resource(res) {}
};

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
    new (vec) WrenUILabel{nullptr};
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
    ::Color rayColor {(unsigned char)color->value.r, (unsigned char)color->value.g, (unsigned char)color->value.b, (unsigned char)color->value.a};
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
WREN_CLASS_METHOD("game", "UILabel", "setPosition(_)", wren_UILabelSetPosition, "Sets the labels position");
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

// TODO move this to a seperate file.
#define COMPONENT_LIST\
	COMPONENT(Camera)\
	COMPONENT(Level)\
	COMPONENT(PhysicsBody)\
	COMPONENT(Shader)\
	COMPONENT(Sprite)\
	COMPONENT(SpriteAnimation)\
	COMPONENT(TileMap)\
	COMPONENT(LocalTransform)\
	COMPONENT(WorldTransform)\
	COMPONENT(WrenScript)\
	// COMPONENT_LIST


// GameObject.getAllWithComponent(componentName) -> List
void wren_GameObjectGetAllWithComponent(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
    
    const char* componentName = wrenGetSlotString(vm, 1);
    
    wrenSetSlotNewList(vm, 0);
    
    int index = 0;
    
#define COMPONENT(component_name) 											\
	if (strcmp(componentName, #component_name) == 0) 						\
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
    
    // Get component names from list
    std::vector<std::string> components(componentCount);
    for (int i = 0; i < componentCount; i++) {
        wrenGetListElement(vm, 1, i, 2); // Get element into slot 2
        const char* componentName = wrenGetSlotString(vm, 2);
        components.push_back(componentName);
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
#define COMPONENT(component_name) 													\
			if (compName == #component_name) 										\
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
        components.push_back(wrenGetSlotString(vm, 2));
    }
    
    wrenSetSlotNewList(vm, 0);
    int resultIndex = 0;
    
    registry.each([&](auto entity) {
        bool hasAnyComponent = false;
        
        for (const auto& compName : components)
		{
#define COMPONENT(component_name) 													\
			if (compName == #component_name) 										\
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
    registry.each([&](auto entity)
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
    for (auto [entity, identifier] : view.each())
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

#define COMPONENT(component_name) 											\
	if (strcmp(componentName, #component_name) == 0) 						\
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
        components.push_back(componentName);
    }
    
    // Iterate all entities and check if they have ALL required components
    registry.each([&](auto entity)
	{
        bool hasAllComponents = true;
        
        for (const auto& compName : components)
		{
#define COMPONENT(component_name) 													\
			if (compName == #component_name) 										\
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
        components.push_back(componentName);
    }
    
    // Iterate all entities and check if they have ANY required components
    registry.each([&](auto entity)
	{
        bool hasAnyComponents = false;
        
        for (const auto& compName : components)
		{
#define COMPONENT(component_name) 													\
			if (compName == #component_name) 										\
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
// SPRITE COMPONENT BINDINGS
// ============================================================================

// SpriteComponent.create(spriteEntity, texture, color, offset, columns, rows, flipped, index, renderPriority) -> number
void wren_SpriteComponentCreate(WrenVM* vm)
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

WREN_CLASS_STATIC("game", "SpriteComponent", "create(_,_,_,_,_,_,_,_,_)", wren_SpriteComponentCreate, "Creates the sprite Component.");

// ScriptComponent.setRenderPriority(entity, renderPriority)
void wren_ScriptComponentSetRenderPriority(WrenVM* vm)
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

WREN_CLASS_STATIC("game", "ScriptComponent", "setRenderPriority(_,_)", wren_ScriptComponentSetRenderPriority, "Sets the render priority of a sprite component");

// ============================================================================
// SCRIPT COMPONENT BINDINGS
// ============================================================================

// ScriptComponent.create(spriteEntity, scriptPath, className) -> number
void wren_ScriptComponentCreate(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	entt::entity levelEntity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
    const char* scriptPath = wrenGetSlotString(vm, 2);
    const char* className = wrenGetSlotString(vm, 3);

	registry.emplace<Struktur::Component::WrenScript>(levelEntity, scriptPath, className);
}

WREN_CLASS_STATIC("game", "ScriptComponent", "create(_,_,_)", wren_ScriptComponentCreate, "Creates the script Component.");

// ScriptComponent.createArg(spriteEntity, scriptPath, className, args) -> number
void wren_ScriptComponentCreateArg(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	entt::registry& registry = context->GetRegistry();

	entt::entity levelEntity = static_cast<entt::entity>(wrenGetSlotDouble(vm, 1));
    const char* scriptPath = wrenGetSlotString(vm, 2);
    const char* className = wrenGetSlotString(vm, 3);
    const char* args = wrenGetSlotString(vm, 4);

	registry.emplace<Struktur::Component::WrenScript>(levelEntity, scriptPath, className, args);

    //TODO possibly initialise the wren script here??

}

WREN_CLASS_STATIC("game", "ScriptComponent", "createArg(_,_,_,_)", wren_ScriptComponentCreateArg, "Creates the script Component with an arg.");

// ScriptComponent.hasMethod(entity, methodName) -> Bool
void wren_ScriptComponentHasMethod(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
    
    double entityId = wrenGetSlotDouble(vm, 1);
    entt::entity entity = static_cast<entt::entity>(entityId);
    
    const char* methodName = wrenGetSlotString(vm, 2);
    
    auto* script = registry.try_get<Struktur::Component::WrenScript>(entity);
    
    if (!script || !script->isInitialized || script->hasError) {
        wrenSetSlotBool(vm, 0, false);
        return;
    }
    
    // Check if the script's instance has this method
    // We need to get the Wren instance and check if it responds to the method
    
    // For now, we'll just check if the script is valid
    // You could implement a method registry in WrenScript component
    wrenSetSlotBool(vm, 0, true);
}

WREN_CLASS_STATIC("game", "ScriptComponent", "hasMethod(_,_)", wren_ScriptComponentHasMethod, "Check if entity's script has a method");

// ScriptComponent.callArg(entity, methodName, ...args) -> result
void wren_ScriptComponentCallArg(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
    auto& scriptEngine = context->GetWrenScriptEngine();
    
    double entityId = wrenGetSlotDouble(vm, 1);
    entt::entity entity = static_cast<entt::entity>(entityId);
    
    const char* methodName = wrenGetSlotString(vm, 2);
    
    auto* script = registry.try_get<Struktur::Component::WrenScript>(entity);
    
    if (!script || !script->isInitialized || script->hasError)
    {
        DEBUG_ERROR("ScriptComponent.call: Entity has no valid script");
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    WrenVM* wrenVM = scriptEngine.GetVM();
    
    // Get the script's instance
    if (!script->instanceHandle)
    {
        DEBUG_ERROR("ScriptComponent.call: No instance handle");
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    // Build method signature based on argument count
    int argCount = wrenGetSlotCount(vm) - 3;  // Total - (receiver, entity, methodName)
    
    std::string signature = methodName;
    if (argCount > 0)
    {
        signature += "(";
        for (int i = 0; i < argCount; i++) {
            if (i > 0) signature += ",";
            signature += "_";
        }
        signature += ")";
    }
    else
    {
        signature += "()";
    }
    
    // Create call handle
    WrenHandle* methodHandle = wrenMakeCallHandle(wrenVM, signature.c_str());
    
    if (!methodHandle)
    {
        DEBUG_ERROR("ScriptComponent.call: Method '%s' not found", signature.c_str());
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    // Set up call
    wrenEnsureSlots(wrenVM, argCount + 1);
    wrenSetSlotHandle(wrenVM, 0, script->instanceHandle);
    
    // Copy arguments from calling VM to target VM
    for (int i = 0; i < argCount; i++)
    {
        int sourceSlot = i + 3;  // Skip receiver, entity, methodName
        int targetSlot = i + 1;
        
        WrenType type = wrenGetSlotType(vm, sourceSlot);
        
        switch (type) {
            case WREN_TYPE_BOOL:
                wrenSetSlotBool(wrenVM, targetSlot, wrenGetSlotBool(vm, sourceSlot));
                break;
            case WREN_TYPE_NUM:
                wrenSetSlotDouble(wrenVM, targetSlot, wrenGetSlotDouble(vm, sourceSlot));
                break;
            case WREN_TYPE_STRING:
                wrenSetSlotString(wrenVM, targetSlot, wrenGetSlotString(vm, sourceSlot));
                break;
            case WREN_TYPE_NULL:
                wrenSetSlotNull(wrenVM, targetSlot);
                break;
            default:
                DEBUG_ERROR("ScriptComponent.call: Type of '%s' not implemented", signature.c_str());
                // For complex types, we'd need to handle them specially
                wrenSetSlotNull(wrenVM, targetSlot);
                break;
        }
    }
    
    // Call the method
    WrenInterpretResult result = wrenCall(wrenVM, methodHandle);
    wrenReleaseHandle(wrenVM, methodHandle);
    
    if (result != WREN_RESULT_SUCCESS)
    {
        DEBUG_ERROR("ScriptComponent.call: Method call failed");
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    // Copy return value back to calling VM
    WrenType returnType = wrenGetSlotType(wrenVM, 0);
    
    switch (returnType)
    {
        case WREN_TYPE_BOOL:
            wrenSetSlotBool(vm, 0, wrenGetSlotBool(wrenVM, 0));
            break;
        case WREN_TYPE_NUM:
            wrenSetSlotDouble(vm, 0, wrenGetSlotDouble(wrenVM, 0));
            break;
        case WREN_TYPE_STRING:
            wrenSetSlotString(vm, 0, wrenGetSlotString(wrenVM, 0));
            break;
        case WREN_TYPE_NULL:
            wrenSetSlotNull(vm, 0);
            break;
        default:
            // For complex return types, we'd need special handling
            wrenSetSlotNull(vm, 0);
            break;
    }
}

WREN_CLASS_STATIC("game", "ScriptComponent", "callArg(_,_,_)", wren_ScriptComponentCallArg,"Call a method on another entity's script with arguments");

// ScriptComponent.call(entity, methodName) -> result
void wren_ScriptComponentCall(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
    auto& scriptEngine = context->GetWrenScriptEngine();
    
    double entityId = wrenGetSlotDouble(vm, 1);
    entt::entity entity = static_cast<entt::entity>(entityId);
    
    const char* methodName = wrenGetSlotString(vm, 2);
    
    auto* script = registry.try_get<Struktur::Component::WrenScript>(entity);
    
    if (!script || !script->isInitialized || script->hasError)
    {
        DEBUG_ERROR("ScriptComponent.call: Entity has no valid script");
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    WrenVM* wrenVM = scriptEngine.GetVM();
    
    // Get the script's instance
    if (!script->instanceHandle)
    {
        DEBUG_ERROR("ScriptComponent.call: No instance handle");
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    // Create call handle
    std::string signature = std::string(methodName) + "()";
    WrenHandle* methodHandle = wrenMakeCallHandle(wrenVM, signature.c_str());
    
    if (!methodHandle)
    {
        DEBUG_ERROR("ScriptComponent.call: Method '%s' not found", signature.c_str());
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    // Set up call
    wrenEnsureSlots(wrenVM, 1);
    wrenSetSlotHandle(wrenVM, 0, script->instanceHandle);
    
    WrenInterpretResult result = wrenCall(wrenVM, methodHandle);
    wrenReleaseHandle(wrenVM, methodHandle);
    
    if (result != WREN_RESULT_SUCCESS)
    {
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    // Return value
    WrenType returnType = wrenGetSlotType(wrenVM, 0);
    
    switch (returnType)
    {
        case WREN_TYPE_BOOL:
            wrenSetSlotBool(vm, 0, wrenGetSlotBool(wrenVM, 0));
            break;
        case WREN_TYPE_NUM:
            wrenSetSlotDouble(vm, 0, wrenGetSlotDouble(wrenVM, 0));
            break;
        case WREN_TYPE_STRING:
            wrenSetSlotString(vm, 0, wrenGetSlotString(wrenVM, 0));
            break;
        default:
            wrenSetSlotNull(vm, 0);
            break;
    }
}

WREN_CLASS_STATIC("game", "ScriptComponent", "call(_,_)", wren_ScriptComponentCall, "Call a method on another entity's script");

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
    new (vec) WrenUILabel{label};
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

WREN_CLASS_STATIC("game", "Camera", "worldPosToScreenPos(_)", wren_CameraWorldPosToScreenPos, "Converts a world position to the screen position.");

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

WREN_CLASS_STATIC("game", "Camera", "screenPosToWorldPos(_)", wren_CameraScreenPosToWorldPos, "Converts a screen position to the world position.");

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
