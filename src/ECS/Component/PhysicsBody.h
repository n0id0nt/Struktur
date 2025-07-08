#pragma once

#include "box2d/box2d.h"

namespace Struktur
{
	namespace Component
	{
        struct PhysicsBody {
            b2Body* body = nullptr;
            bool isKinematic = false;
            bool syncFromPhysics = true;
            bool syncToPhysics = false;
            bool isDirty = true;
        };
    }
}
