#pragma once

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
class GameContext;

namespace System
{
class UISystem : public ISystem
{
   public:
	void Update(GameContext& context) override;
	std::string Name() const override
	{
		return "UI System";
	}
};
}  // namespace System
}  // namespace Struktur
