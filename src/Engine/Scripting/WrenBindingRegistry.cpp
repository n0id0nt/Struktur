#include "WrenBindingRegistry.h"

namespace Struktur::Wren
{
void RegisterAllBindings(BindingRegistry& registry)
{
	// Core's
	RegisterMathBindings(registry);
	RegisterResourceManagerBindings(registry);
	RegisterFunctionCallbackBindings(registry);
	RegisterFileSystemBindings(registry);
	RegisterApplicationBindings(registry);
	RegisterInputBindings(registry);
	RegisterDebugBindings(registry);
	RegisterSerialisationBindings(registry);
	RegisterEventsBindings(registry);
	RegisterAudioBindings(registry);
	RegisterLocalizationBindings(registry);
	RegisterJobsBindings(registry);
#ifdef STRUKTUR_ECS
	RegisterGameObjectBindings(registry);
	#ifdef STRUKTUR_PHYSICS
	RegisterPhysicsBodyBindings(registry);
	#endif
	RegisterGameObjectComponentBindings(registry);
#endif
#ifdef STRUKTUR_PHYSICS
	RegisterPhysicsBindings(registry);
	RegisterCollisionLayersBindings(registry);
#endif
#ifdef STRUKTUR_UI
	RegisterUIBindings(registry);
#endif
#ifdef STRUKTUR_DIALOGUE
	RegisterDialogueBindings(registry);
	RegisterVariableSubstitutionBindings(registry);
#endif
#ifdef STRUKTUR_FLAG
	RegisterFlagsBindings(registry);
#endif
#ifdef STRUKTUR_FITNESS
	RegisterFitnessBindings(registry);
#endif
	// The game's own
	RegisterRideBindings(registry);
	RegisterWorldBindings(registry);
}
}  // namespace Struktur::Wren
