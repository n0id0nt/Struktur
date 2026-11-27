#pragma once

#include <cstdint>

#include "Engine/World/Camera.h"
#include "Engine/World/RiderAnimation.h"

// The world's entities (World::LoadWorld creates them): the player's rider, pacers, finish banners and the camera.
// Plain data - the systems are in ECS/System.
namespace Struktur::Component
{
// Destroyed when the world unloads.
struct WorldMember
{
};

// A place on the road.
struct RoadPosition
{
	double distance = 0.0;  // metres along the route
	double lateral  = 0.0;  // metres left of the centre line
};

// Pedalling: crank phase, wheel angle, pose and lean (System::RiderAnimationSystem).
struct CrankAnimation
{
	World::RiderAnimation animation;
};

// The player's rider, drawn close up over the scene and animated from the trainer input.
struct RiderSprite
{
};

// A rider drawn in the scene, with a colour multiplier on the jersey.
struct Tint
{
	uint32_t abgr = 0xFFFFFFFFu;
};

// Rides at its own steady pace near the player (System::PacerSystem). For --demo; ghosts would be another mover.
struct Pacer
{
	double cadenceRpm = 88.0;
	double startAhead = 25.0;  // metres in front of the player, where it appears and rejoins
	double time       = 0.0;
	double speedMetersPerSecond = 0.0;
	bool placed       = false;
};

// A finish banner across the road at the entity's RoadPosition.
struct Banner
{
};

// The camera entity: System::CameraSystem moves it (with a World::RoadFollow) and copies it to the context's camera.
struct Camera3D
{
	World::Camera camera;
};
}  // namespace Struktur::Component
