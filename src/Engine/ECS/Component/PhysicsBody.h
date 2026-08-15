#pragma once

#include "box2d/box2d.h"

namespace Struktur
{
namespace Component
{
struct PhysicsBody
{
	b2Body* body     = nullptr;
	bool isKinematic = false;
};
}  // namespace Component
}  // namespace Struktur
