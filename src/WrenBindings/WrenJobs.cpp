#include "WrenJobs.h"

#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "WrenResourceManager.h"

static Struktur::GameContext& ContextOf(WrenVM* vm)
{
	return *static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
}

static WrenScriptTask& TaskOf(WrenVM* vm)
{
	return *static_cast<WrenScriptTask*>(wrenGetSlotForeign(vm, 0));
}

// Completed but the work itself reported a failure.
static bool HasError(const WrenScriptTask& wrapper)
{
	return wrapper.task.Succeeded() && wrapper.task.GetResult().kind == ScriptTaskValue::Kind::ERROR;
}

void wrenSetSlotScriptTask(WrenVM* vm, int slot, Struktur::Jobs::Task<ScriptTaskValue> task)
{
	wrenEnsureSlots(vm, slot + 2);
	wrenGetVariable(vm, "jobs", "Task", slot + 1);
	WrenScriptTask* wrapper =
	    static_cast<WrenScriptTask*>(wrenSetSlotNewForeign(vm, slot, slot + 1, sizeof(WrenScriptTask)));
	new (wrapper) WrenScriptTask();
	wrapper->task = std::move(task);
}

// ============================================================================
// jobs.Task
// ============================================================================

void wren_JobsTaskAllocate(WrenVM* vm)
{
	// Only made by engine calls (Texture.loadAsync, FileSystem.readStringAsync); a script `Task.new()` gets an
	// invalid task that reports itself as dropped.
	WrenScriptTask* wrapper = static_cast<WrenScriptTask*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenScriptTask)));
	new (wrapper) WrenScriptTask();
}

void wren_JobsTaskFinalize(void* data)
{
	// Dropping the last reference to a running task is fine: the work finishes (or is cancelled) and its result is
	// simply discarded. No Wren calls are allowed here.
	static_cast<WrenScriptTask*>(data)->~WrenScriptTask();
}

void wren_JobsTaskIsComplete(WrenVM* vm)
{
	wrenSetSlotBool(vm, 0, TaskOf(vm).task.IsComplete());
}

void wren_JobsTaskSucceeded(WrenVM* vm)
{
	const WrenScriptTask& wrapper = TaskOf(vm);
	wrenSetSlotBool(vm, 0, wrapper.task.Succeeded() && !HasError(wrapper));
}

void wren_JobsTaskFailed(WrenVM* vm)
{
	wrenSetSlotBool(vm, 0, HasError(TaskOf(vm)));
}

void wren_JobsTaskCancelled(WrenVM* vm)
{
	wrenSetSlotBool(vm, 0, TaskOf(vm).task.WasCancelled());
}

void wren_JobsTaskError(WrenVM* vm)
{
	const WrenScriptTask& wrapper = TaskOf(vm);
	if (HasError(wrapper))
	{
		wrenSetSlotString(vm, 0, wrapper.task.GetResult().message.c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 0);
	}
}

void wren_JobsTaskProgress(WrenVM* vm)
{
	wrenSetSlotDouble(vm, 0, TaskOf(vm).task.GetProgress());
}

void wren_JobsTaskStatus(WrenVM* vm)
{
	const WrenScriptTask& wrapper = TaskOf(vm);
	wrenSetSlotString(vm, 0, HasError(wrapper) ? "failed" : Struktur::Jobs::ToString(wrapper.task.GetStatus()));
}

void wren_JobsTaskCancel(WrenVM* vm)
{
	TaskOf(vm).task.Cancel();
}

// task.result -> the value (String, Texture), or null until the task has succeeded.
void wren_JobsTaskResult(WrenVM* vm)
{
	WrenScriptTask& wrapper = TaskOf(vm);
	if (!wrapper.task.Succeeded() || HasError(wrapper))
	{
		wrenSetSlotNull(vm, 0);
		return;
	}
	const ScriptTaskValue& value = wrapper.task.GetResult();
	switch (value.kind)
	{
		case ScriptTaskValue::Kind::STRING:
			wrenSetSlotBytes(vm, 0, value.text.data(), value.text.size());
			return;
		case ScriptTaskValue::Kind::TEXTURE:
		{
			if (!wrapper.textureCreated)
			{
				// Main thread: hand the decoded pixels to the texture pool (a no-op if the texture is loaded already)
				// and take a reference, which uploads on first draw like any other texture.
				Struktur::GameContext& context = ContextOf(vm);
				ScriptTaskValue owned          = wrapper.task.TakeResult();
				context.GetResourceManager().AddDecodedTexture(owned.path, std::move(owned.image));
				wrapper.texture        = context.GetResourceManager().GetTexture(context, owned.path);
				wrapper.textureCreated = true;
			}
			if (!wrapper.texture)
			{
				wrenSetSlotNull(vm, 0);
				return;
			}
			wrenEnsureSlots(vm, 2);
			wrenGetVariable(vm, "resourceManager", "Texture", 1);
			WrenTextureHandle* handle =
			    static_cast<WrenTextureHandle*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenTextureHandle)));
			new (handle) WrenTextureHandle(wrapper.texture);
			return;
		}
		default:
			wrenSetSlotNull(vm, 0);
			return;
	}
}

// ============================================================================
// Engine calls that start tasks
// ============================================================================

// FileSystem.readStringAsync(path) -> Task whose result is the file's text
void wren_FileSystemReadStringAsync(WrenVM* vm)
{
	const std::string path = wrenGetSlotString(vm, 1);
	Struktur::Jobs::Task<ScriptTaskValue> task = ContextOf(vm).GetJobSystem().Run(
	    [path]
	    {
		    ScriptTaskValue value;
		    Struktur::FileResult<std::string> result = Struktur::FileSystem::ReadString(path);
		    if (result.success)
		    {
			    value.kind = ScriptTaskValue::Kind::STRING;
			    value.text = std::move(result.value);
		    }
		    else
		    {
			    value.kind    = ScriptTaskValue::Kind::ERROR;
			    value.message = result.errorMessage.empty() ? "could not read " + path : result.errorMessage;
		    }
		    return value;
	    });
	wrenSetSlotScriptTask(vm, 0, std::move(task));
}

// Texture.loadAsync(path) -> Task whose result is a Texture
void wren_ResourceTextureLoadAsync(WrenVM* vm)
{
	const std::string path = wrenGetSlotString(vm, 1);
	Struktur::Jobs::Task<ScriptTaskValue> task = ContextOf(vm).GetJobSystem().Run(
	    [path]
	    {
		    ScriptTaskValue value;
		    value.path = path;
		    std::string error;
		    if (Struktur::Resource::TextureResource::Decode(path, value.image, error))
		    {
			    value.kind = ScriptTaskValue::Kind::TEXTURE;
		    }
		    else
		    {
			    value.kind    = ScriptTaskValue::Kind::ERROR;
			    value.message = "could not load " + path + ": " + error;
		    }
		    return value;
	    });
	wrenSetSlotScriptTask(vm, 0, std::move(task));
}

void Struktur::Wren::RegisterJobsBindings(BindingRegistry& registry)
{
	WREN_FOREIGN_CLASS(registry, "jobs", "Task", wren_JobsTaskAllocate, wren_JobsTaskFinalize,
	                   "Work running on the job system - poll it from update() (isComplete, then succeeded and result). "
	                   "Made by engine calls such as Texture.loadAsync and FileSystem.readStringAsync");
	WREN_CLASS_METHOD(registry, "jobs", "Task", "isComplete", wren_JobsTaskIsComplete,
	                  "True once the task has finished, failed, been cancelled or been dropped");
	WREN_CLASS_METHOD(registry, "jobs", "Task", "succeeded", wren_JobsTaskSucceeded,
	                  "True once the task has finished and its result is ready");
	WREN_CLASS_METHOD(registry, "jobs", "Task", "failed", wren_JobsTaskFailed,
	                  "True if the work ran but failed - see error");
	WREN_CLASS_METHOD(registry, "jobs", "Task", "cancelled", wren_JobsTaskCancelled, "True if cancel() stopped it");
	WREN_CLASS_METHOD(registry, "jobs", "Task", "error", wren_JobsTaskError, "Why the task failed, or null");
	WREN_CLASS_METHOD(registry, "jobs", "Task", "progress", wren_JobsTaskProgress,
	                  "0 to 1 as reported by the work; 1 once it succeeded");
	WREN_CLASS_METHOD(registry, "jobs", "Task", "status", wren_JobsTaskStatus,
	                  "\"pending\", \"running\", \"succeeded\", \"failed\", \"cancelled\" or \"dropped\"");
	WREN_CLASS_METHOD(registry, "jobs", "Task", "result", wren_JobsTaskResult,
	                  "The result (String, Texture, ...) once succeeded, otherwise null");
	WREN_CLASS_METHOD(registry, "jobs", "Task", "cancel()", wren_JobsTaskCancel,
	                  "Asks the task to stop; one that hasn't started never runs");

	WREN_IMPORT(registry, "fileSystem", "jobs", "Task");
	WREN_CLASS_STATIC(registry, "fileSystem", "FileSystem", "readStringAsync(_)", wren_FileSystemReadStringAsync,
	                  "Reads a file as a String on the job system; returns a jobs.Task whose result is the text");
	WREN_IMPORT(registry, "resourceManager", "jobs", "Task");
	WREN_CLASS_STATIC(registry, "resourceManager", "Texture", "loadAsync(_)", wren_ResourceTextureLoadAsync,
	                  "Loads a texture on the job system (decode on a worker, GPU upload on the main thread); returns a "
	                  "jobs.Task whose result is the Texture");
}
