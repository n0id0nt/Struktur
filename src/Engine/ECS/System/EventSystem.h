#pragma once

#include <queue>
#include <string>

#include "Engine/Callback/Variant.h"
#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
class GameContext;

namespace System
{
class EventSystem : public ISystem
{
   public:
	EventSystem() {}

	void Update(GameContext& context) override;

	std::string Name() const override
	{
		return "Event System";
	}
};
}  // namespace System
}  // namespace Struktur
