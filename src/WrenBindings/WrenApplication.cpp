#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "raylib.h"
#include "wren.hpp"

// ============================================================================
// APPLICATION BINDINGS
// ============================================================================

// Application.setWindowSize(windowWidth, windowHeight)
void wren_ApplicationSetWindowSize(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData                 = context->GetGameData();

	double windowWidthDouble  = wrenGetSlotDouble(vm, 1);
	double windowHeightDouble = wrenGetSlotDouble(vm, 2);
	int windowWidth           = static_cast<int>(windowWidthDouble);
	int windowHeight          = static_cast<int>(windowHeightDouble);

	gameData.gameWidth  = windowWidth;
	gameData.gameHeight = windowHeight;

	// TODO check if the window size needs to be changed
	// this can be called before there is a window so need to handle that case as well
	// need to handle debug and editor builds when the game and application size does not match
}

// Application.setApplicationName(name)
void wren_ApplicationSetApplicationName(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData                 = context->GetGameData();

	const char* name = wrenGetSlotString(vm, 1);

	gameData.projectName = name;

	// TODO check if the window name needs to be changed here
	// this can be called before there is a window so need to handle that case as well
}

// Application.registerComponentScript(module, className)
void wren_ApplicationRegisterComponentScript(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& scriptComponentRegistry  = context->GetWrenScriptComponentRegistry();
	const char* module             = wrenGetSlotString(vm, 1);
	const char* className          = wrenGetSlotString(vm, 2);

	scriptComponentRegistry.RegisterScriptComponent(module, className);
}

// Application.gameWidth -> number
void wren_ApplicationGetGameWidth(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData                 = context->GetGameData();
	double gameWidth               = static_cast<double>(gameData.gameWidth);
	wrenSetSlotDouble(vm, 0, gameWidth);
}

// Application.gameHeight -> number
void wren_ApplicationGetGameHeight(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData                 = context->GetGameData();
	double gameHeight              = static_cast<double>(gameData.gameHeight);
	wrenSetSlotDouble(vm, 0, gameHeight);
}

// Application.pixelsPerMeter -> number
void wren_ApplicationGetPixelsPerMeter(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData                 = context->GetGameData();
	double pixelsPerMeter          = static_cast<double>(gameData.pixelsPerMeter);
	wrenSetSlotDouble(vm, 0, pixelsPerMeter);
}

// Application.setPixelsPerMeter(number)
void wren_ApplicationSetPixelsPerMeter(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData                 = context->GetGameData();
	double pixelsPerMeter          = wrenGetSlotDouble(vm, 1);
	gameData.pixelsPerMeter        = static_cast<float>(pixelsPerMeter);
}

// Application.velocityIterations -> number
void wren_ApplicationGetVelocityIterations(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData                 = context->GetGameData();
	double velocityIterations      = static_cast<double>(gameData.velocityIterations);
	wrenSetSlotDouble(vm, 0, velocityIterations);
}

// Application.setVelocityIterations(number)
void wren_ApplicationSetVelocityIterations(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData                 = context->GetGameData();
	double velocityIterations      = wrenGetSlotDouble(vm, 1);
	gameData.velocityIterations    = static_cast<int>(velocityIterations);
}

// Application.isFullScreen -> bool
void wren_ApplicationGetIsFullScreen(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData                 = context->GetGameData();
	wrenSetSlotBool(vm, 0, gameData.isFullScreen);
}

// Application.setIsFullScreen(bool)
void wren_ApplicationSetIsFullScreen(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& gameData                 = context->GetGameData();
	bool isFullScreen              = wrenGetSlotBool(vm, 1);
	gameData.isFullScreen          = isFullScreen;
#ifndef EDITOR
	if (::IsWindowReady() && gameData.isFullScreen != ::IsWindowFullscreen())
	{
		::ToggleFullscreen();
	}
#endif
}

// ============================================================================
// TIME BINDINGS
// ============================================================================

// Time.unscaledDelta -> number
void wren_TimeGetUnscaledDelta(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& timeSystem               = context->GetTimeSystem();
	wrenSetSlotDouble(vm, 0, timeSystem.unscaledDelta);
}

// Time.scaledDelta -> number
void wren_TimeGetScaledDelta(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& timeSystem               = context->GetTimeSystem();
	wrenSetSlotDouble(vm, 0, timeSystem.scaledDelta);
}

// Time.unscaledTime -> number
void wren_TimeGetUnscaledTime(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& timeSystem               = context->GetTimeSystem();
	wrenSetSlotDouble(vm, 0, timeSystem.unscaledTime);
}

// Time.scaledTime -> number
void wren_TimeGetScaledTime(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& timeSystem               = context->GetTimeSystem();
	wrenSetSlotDouble(vm, 0, timeSystem.scaledTime);
}

// Time.timeScale -> number
void wren_TimeGetTimeScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& timeSystem               = context->GetTimeSystem();
	wrenSetSlotDouble(vm, 0, timeSystem.timeScale);
}

// Time.setTimeScale(number)
void wren_TimeSetTimeScale(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	auto& timeSystem               = context->GetTimeSystem();
	double timeScale               = wrenGetSlotDouble(vm, 1);
	timeSystem.SetTimeScale(timeScale);
}

WREN_BINDING_MODULE(Application)
{
	WREN_CLASS_STATIC(registry, "app", "Application", "setWindowSize(_,_)", wren_ApplicationSetWindowSize,
	                  "Change the size of the game window.");
	WREN_CLASS_STATIC(registry, "app", "Application", "setApplicationName(_)", wren_ApplicationSetApplicationName,
	                  "Changes the name of the game window.");
	WREN_CLASS_STATIC(registry, "app", "Application", "registerComponentScript(_,_)",
	                  wren_ApplicationRegisterComponentScript, "Changes the name of the game window.");
	WREN_CLASS_STATIC(registry, "app", "Application", "gameWidth", wren_ApplicationGetGameWidth,
	                  "Get the games width.");
	WREN_CLASS_STATIC(registry, "app", "Application", "gameHeight", wren_ApplicationGetGameHeight,
	                  "Get the games height.");
	WREN_CLASS_STATIC(registry, "app", "Application", "pixelsPerMeter", wren_ApplicationGetPixelsPerMeter,
	                  "Get the pixels per meter for the physics system.");
	WREN_CLASS_STATIC(registry, "app", "Application", "setPixelsPerMeter(_)", wren_ApplicationSetPixelsPerMeter,
	                  "Set the pixels per meter for the physics system.");
	WREN_CLASS_STATIC(registry, "app", "Application", "velocityIterations", wren_ApplicationGetVelocityIterations,
	                  "Get the velocity iterations for the physics system.");
	WREN_CLASS_STATIC(registry, "app", "Application", "setVelocityIterations(_)", wren_ApplicationSetVelocityIterations,
	                  "Set the velocity iterations for the physics system.");
	WREN_CLASS_STATIC(registry, "app", "Application", "isFullScreen", wren_ApplicationGetIsFullScreen,
	                  "Get the if the application is funning in full screen.");
	WREN_CLASS_STATIC(registry, "app", "Application", "setIsFullScreen(_)", wren_ApplicationSetIsFullScreen,
	                  "Set the if the application is funning in full screen.");

	WREN_CLASS_STATIC(registry, "app", "Time", "unscaledDelta", wren_TimeGetUnscaledDelta,
	                  "Gets the unsclaled delta time.");
	WREN_CLASS_STATIC(registry, "app", "Time", "scaledDelta", wren_TimeGetScaledDelta, "Gets the sclaled delta time.");
	WREN_CLASS_STATIC(registry, "app", "Time", "unscaledTime", wren_TimeGetUnscaledTime, "Get the unsclaled time.");
	WREN_CLASS_STATIC(registry, "app", "Time", "scaledTime", wren_TimeGetScaledTime, "Get the sclaled time.");
	WREN_CLASS_STATIC(registry, "app", "Time", "timeScale", wren_TimeGetTimeScale, "Get the sclaled time.");
	WREN_CLASS_STATIC(registry, "app", "Time", "setTimeScale(_)", wren_TimeSetTimeScale, "Set the time scale.");
}
