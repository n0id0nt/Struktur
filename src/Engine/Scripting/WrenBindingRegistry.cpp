#include "WrenBindingRegistry.h"

namespace Struktur::Wren
{
void RegisterAllBindings(BindingRegistry& registry)
{
	RegisterAnimationBindings(registry);
	RegisterGameObjectBindings(registry);
	RegisterGameObjectComponentBindings(registry);
	RegisterRendererBindings(registry);
	RegisterMathBindings(registry);
	RegisterPhysicsBindings(registry);
	RegisterCollisionLayersBindings(registry);
	RegisterResourceManagerBindings(registry);
	RegisterUIBindings(registry);
	RegisterDialogueBindings(registry);
	RegisterFunctionCallbackBindings(registry);
	RegisterFileSystemBindings(registry);
	RegisterFlagsBindings(registry);
	RegisterApplicationBindings(registry);
	RegisterInputBindings(registry);
	RegisterDebugBindings(registry);
	RegisterSerialisationBindings(registry);
	RegisterVariableSubstitutionBindings(registry);
	RegisterEventsBindings(registry);
	RegisterAudioBindings(registry);
	RegisterLocalizationBindings(registry);
	RegisterFitnessBindings(registry);
	RegisterRideBindings(registry);
	RegisterJobsBindings(registry);
	RegisterWorldBindings(registry);
}
}  // namespace Struktur::Wren
