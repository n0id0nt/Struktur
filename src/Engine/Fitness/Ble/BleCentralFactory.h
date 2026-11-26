#pragma once

#include <memory>

#include "Engine/Fitness/Ble/IBleCentral.h"

namespace Struktur::Fitness
{
// The platform's real backend: WinRtBleCentral on Windows, NullBleCentral everywhere else.
std::unique_ptr<IBleCentral> CreatePlatformBleCentral();
}  // namespace Struktur::Fitness
