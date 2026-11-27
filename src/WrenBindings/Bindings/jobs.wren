// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: jobs

// Work running on the job system - poll it from update() (isComplete, then succeeded and result). Made by engine calls such as Texture.loadAsync and FileSystem.readStringAsync
foreign class Task {
    // True once the task has finished, failed, been cancelled or been dropped
    foreign isComplete
    // True once the task has finished and its result is ready
    foreign succeeded
    // True if the work ran but failed - see error
    foreign failed
    // True if cancel() stopped it
    foreign cancelled
    // Why the task failed, or null
    foreign error
    // 0 to 1 as reported by the work; 1 once it succeeded
    foreign progress
    // "pending", "running", "succeeded", "failed", "cancelled" or "dropped"
    foreign status
    // The result (String, Texture, ...) once succeeded, otherwise null
    foreign result
    // Asks the task to stop; one that hasn't started never runs
    foreign cancel()
}

