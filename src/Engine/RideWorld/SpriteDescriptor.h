#pragma once

#include <string>
#include <vector>

namespace Struktur::RideWorld
{
enum class SpriteLayerMotion
{
	STATIC,  // one image (frame 0 of the role's sheet)
	CRANK,   // a frame per crank position (rows: pedalling / out of saddle / still poses)
	WHEEL_ROTATE,  // side-on wheel: the layer rotates about its centre with the wheel angle
	WHEEL_SCROLL,  // rear view: a tileable tread strip scrolls vertically with the wheel angle
};

struct SpriteLayer
{
	std::string name;
	std::string role;  // manifest role
	SpriteLayerMotion motion = SpriteLayerMotion::STATIC;
	// Where the layer sits in frame pixels (x, y, width, height); width 0 = the whole frame. May reach outside the
	// frame (e.g. a character drawn over the bike).
	int rectX = 0;
	int rectY = 0;
	int rectWidth  = 0;
	int rectHeight = 0;
	int spokes     = 0;  // WHEEL_ROTATE: for the wagon-wheel check
	int framesPerRevolution = 0;  // CRANK: this layer's own cycle length (0 = the descriptor's)
	// CRANK rows and the columns of the still poses in the "still" row.
	int pedalRow       = 0;
	int outOfSaddleRow = -1;  // -1 = no out-of-saddle art (the pedal row is used)
	int stillRow       = -1;
	int coastColumn    = 0;
	int idleColumn     = 0;
	int outOfSaddleOffsetY = 0;  // shift (pixels) for static layers while standing
};

// How the rider is drawn - a JSON file (assets/World/Rider/rider.json). Either layered (frame, wheels, legs, torso -
// each its own manifest role) or one flat sheet (kind "sheet": one role, rows = poses).
struct SpriteDescriptor
{
	std::string kind = "layered";  // layered | sheet
	int frameWidth   = 64;
	int frameHeight  = 96;
	int framesPerRevolution = 12;
	int anchorX       = 32;  // pixel on the ground between the wheels
	int anchorY       = 94;
	double screenHeight = 0.32;  // the player sprite's height as a fraction of the screen
	double heightMeters = 1.85;  // world height when drawn as an actor (pacer / ghost)
	bool lean           = true;  // rotate the sprite into bends
	int leanLeftRow     = -1;    // optional lean art (sheet kind); -1 = rotate instead
	int leanRightRow    = -1;
	std::vector<SpriteLayer> layers;
};

// Returns false (and `error`) when the text isn't a usable descriptor.
bool ParseSpriteDescriptor(const std::string& json, SpriteDescriptor& out, std::string& error);
// The built-in descriptor matching the code-drawn placeholder rider.
SpriteDescriptor DefaultSpriteDescriptor();
}  // namespace Struktur::RideWorld
