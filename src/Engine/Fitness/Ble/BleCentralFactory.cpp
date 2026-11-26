#include "BleCentralFactory.h"

#include "Engine/Fitness/Ble/NullBleCentral.h"
#if defined(STRUKTUR_FITNESS_WINRT)
	#include "Engine/Fitness/Ble/WinRtBleCentral.h"
#endif

namespace Struktur::Fitness
{
std::unique_ptr<IBleCentral> CreatePlatformBleCentral()
{
#if defined(STRUKTUR_FITNESS_WINRT)
	return std::make_unique<WinRtBleCentral>();
#else
	return std::make_unique<NullBleCentral>();
#endif
}
}  // namespace Struktur::Fitness
