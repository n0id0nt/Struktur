#include "FlagServices.h"

namespace Struktur::Flag
{
Services::Services()
{
	flagManager = std::make_unique<FlagManager>();
}

void Services::Shutdown()
{
	flagManager.reset();
}
}  // namespace Struktur::Flag
