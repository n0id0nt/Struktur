#pragma once

#include <string>

#include "Engine/Jobs/Task.h"
#include "Engine/Resource/TextureResource.h"
#include "wren.hpp"

// Scripts' view of the job system (phase 5 of docs/JobSystem.md): a `Task` foreign class in the "jobs" module that a
// script polls from update(), e.g.
//
//   var load = Texture.loadAsync("Backgrounds/big.png")
//   ...
//   if (load.isComplete && load.succeeded) sprite.texture = load.result
//
// Wren itself never runs on a worker: the work behind a task is C++, and its result is turned into a Wren value on the
// main thread, when the script reads `result`.

// What the C++ work behind a script task produces.
struct ScriptTaskValue
{
	enum class Kind
	{
		NONE,
		ERROR,    // the work failed: `message` says why
		STRING,   // `text`
		TEXTURE,  // `image`, decoded from `path`; uploaded when the script first reads the result
	};
	Kind kind = Kind::NONE;
	std::string message;
	std::string text;
	std::string path;
	Struktur::Resource::TextureResource::DecodedImage image;
};

struct WrenScriptTask
{
	Struktur::Jobs::Task<ScriptTaskValue> task;
	// TEXTURE results, once handed to the resource manager (the decoded pixels move there).
	Struktur::Resource::ResourcePtr<Struktur::Resource::TextureResource> texture;
	bool textureCreated = false;
};

// Puts a new jobs.Task wrapping `task` into `slot` (uses slot + 1 for the class). The calling module must import
// Task from "jobs" (WREN_IMPORT) so the class is loaded.
void wrenSetSlotScriptTask(WrenVM* vm, int slot, Struktur::Jobs::Task<ScriptTaskValue> task);
