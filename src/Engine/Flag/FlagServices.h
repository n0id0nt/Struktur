#pragma once

#include <memory>

#include "Engine/Flag/FlagManager.h"

namespace Struktur::Flag
{
// The flag library's services. A game's context holds one and returns its members.
struct Services
{
	Services();
	// Destroys the services, in order.
	void Shutdown();

	std::unique_ptr<FlagManager> flagManager;
};
}  // namespace Struktur::Flag
