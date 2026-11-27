# Job system

The engine's job system and scheduler. **Phases 1, 1b, 2 and 3 are done:**
- **Phase 1:** worker threads, lanes, dependencies, waiting that helps, parallel-for and inline mode.
- **Phase 1b:** pooled jobs, per-worker work-stealing queues and a cap on background workers.
- **Phase 2:** tasks with results, progress and cancellation; follow-ups that run on the main thread; stepped jobs.
- **Phase 3:** the engine owns a job system and syncs it with the frame loop; level tilesets decode in parallel;
  on branch `RideSoftware`, ride world chunks build as background tasks and ride autosaves write in the background.

All of it has unit tests and a benchmark. Phases 4–6 (ECS systems as jobs, Wren handles, web threads) are planned at
the end.

Code: `src/Engine/Jobs/` (library `StrukturJobs`, no SDL, bgfx or Wren). Tests: `src/Engine/Jobs/Tests/`. Benchmark:
`tools/job_bench/`.

## Using it

```cpp
#include "Engine/Jobs/JobSystem.h"
using namespace Struktur::Jobs;

JobSystem jobs;                                         // one worker per core, minus the main thread
JobHandle load   = jobs.Schedule(LoadLevel, Lane::BACKGROUND);
JobHandle build  = jobs.Schedule(BuildNavMesh, Lane::BACKGROUND, load);   // starts after `load`
JobHandle update = jobs.ParallelFor(0, count, 1024, UpdateRange);         // UpdateRange(first, last)
jobs.Wait(update);                                      // runs queued frame jobs on this thread until done

if (build.IsComplete()) { ... }                         // poll from the update loop
```

- **Lanes** set priority: `FRAME` first, then `POST_FRAME`, then `BACKGROUND`. Workers always take the
  highest-priority queued job.
- **Dependencies:** `Schedule(work, lane, dependencies)` starts the job only after every dependency has completed.
  `WhenAll(handles)` is a handle for a group of jobs.
- **`Wait()` helps instead of blocking:** the waiting thread runs queued jobs until the one it needs is done.
  - **The main thread** only helps with jobs of the waited job's lane or higher. Waiting on frame work never makes
    it pick up a long background job mid-frame.
  - **Worker threads, and inline mode,** may also take lower lanes, which is where the waited job's dependencies
    can be.
- **`RunPending(budgetSeconds, lowestLane)`** runs queued jobs on the calling thread for up to a time budget,
  checked between jobs.
- **`Shutdown()`** (also run by the destructor) lets running jobs finish, then drops every queued job. A dropped
  job completes without running and reports `WasDropped()`, so nothing waiting on it hangs. Scheduling after
  shutdown returns an already-dropped handle.
- **Dependency rule:** jobs must not hold locks that a job they depend on also takes. Dependencies, not locks,
  order the work.
- **Background limit:** `JobSystemConfig{workers, backgroundWorkerLimit}` caps how many workers run background jobs
  at once. The default (-1) keeps workers free for frame work: all workers when there's 1, all but one for 2–3, all
  but two otherwise. Threads helping inside `Wait()` don't count towards it.
- **Stats:** `GetStats()` reports jobs scheduled, run, run by helpers, stolen and dropped, steps requeued,
  main-thread follow-ups run, how many jobs needed heap storage for their work, and the pool size.

### Tasks (phase 2)

```cpp
#include "Engine/Jobs/Task.h"

Task<Image> load = jobs.Run([path](TaskContext& context) {
    Image image = Decode(path, [&](float done) { context.SetProgress(done); });
    return image;                                       // ignored if the task was cancelled meanwhile
});

// In the update loop:
if (load.IsComplete() && load.Succeeded()) { Upload(load.GetResult()); }
loadingBar.SetValue(load.GetProgress());

// Or hand the result to the main thread once it's ready:
load.ThenOnMainThread([](Task<Image>& done) {
    if (done.Succeeded()) { Upload(done.TakeResult()); }  // bgfx, Wren and entt are safe here
});

// Once a frame, on the main thread (the engine's FrameScheduler does this - see below):
jobs.RunMainThreadWork(0.002);
```

- **`jobs.Run(work, lane, dependencies)`** schedules a task (background lane by default). `work` is `T()` or
  `T(TaskContext&)`. The task is a `Task<T>`; copies share the same state.
- **Status:** `PENDING`, `RUNNING`, then one of `SUCCEEDED`, `CANCELLED` or `DROPPED` (shut down before it
  finished). `IsComplete()` is true for all three final states. `GetResult()`, `TryGetResult()` and
  `TakeResult()` work only after `SUCCEEDED`.
- **Progress:** the work calls `context.SetProgress(0..1)`; it reads as 1 once the task succeeds.
- **Cancel:** `task.Cancel()` asks the task to stop. If it hasn't started, its work never runs. If it's running, it
  stops when it next checks `context.IsCancelled()`, and any result it returns is discarded.
- **Follow-ups:** `task.ThenOnMainThread(fn)` runs `fn(task)` inside the next `RunMainThreadWork()` after the task
  completes, cancelled ones included. One added after completion is queued straight away. A task dropped at
  shutdown never runs its follow-ups. `jobs.RunOnMainThread(fn)` queues any function the same way.
- **`RunMainThreadWork(budget)`** runs queued follow-ups, oldest first, checking the time budget between them. With
  zero workers it also runs queued jobs in what's left of the budget, so tasks keep progressing without threads.
- **Stepped work:** work that returns `StepResult` is called again after each `CONTINUE` until it returns `DONE`.
  Between steps it goes back in the queue, so higher-lane jobs and other queued work get a turn. Through `Run()`
  it's a `Task<void>` that can be cancelled between steps; through `Schedule()` it's a plain job. With zero workers
  each step is one job for `RunPending()` / `RunMainThreadWork()`, so streaming spans frames within the budget.
- **Wait:** `jobs.Wait(task)` works like waiting on its handle. `task.GetHandle()` gives the job handle for
  dependencies: a task's status turns final a moment before its handle completes, so poll the task, not the handle.
- **Cost:** each task allocates a small shared state, unlike plain `Schedule()`. Tasks are for coarse work (loads,
  builds, streaming); use plain jobs or parallel-for for fine-grained work.

### In the engine (phase 3)

```cpp
Jobs::JobSystem& jobs = context.GetJobSystem();
Task<Mesh> build = jobs.Run([=] { return BuildChunk(seed); });
build.ThenOnMainThread([&context](Task<Mesh>& done) { Upload(context, done.TakeResult()); });

context.GetFrameScheduler().PostFrame([stats] { WriteStats(stats); });   // after this frame, done by the next
```

- **Ownership:** `GameContext` owns the `JobSystem` and a `FrameScheduler`. The worker count is one per core minus
  the main thread. `--threads=N` overrides it, and `--threads=0` runs everything on the main thread (handy for
  debugging). Builds with `STRUKTUR_THREADS` off (web by default) always get zero workers.
- **The frame** (`UpdateLoop` in `Game.cpp`):

  ```
  poll input, time ─ BeginFrame ─ game loop (update, render, submit) ─ EndFrame
                     │                                                 │
                     ├ waits for last frame's post-frame jobs          └ starts this frame's post-frame jobs
                     └ runs main-thread follow-ups (2 ms budget);        (POST_FRAME lane, overlapping the next
                       with zero workers, also queued jobs               frame's input and update)
  ```

  `BeginFrame` shows up as "Jobs Sync" in the profiler. Background tasks aren't tied to frames: they run on
  workers whenever there's room, and hand results back through follow-ups.
- **Shutdown:** `GameContext::Shutdown()` shuts the job system down before anything else, so no job can outlive
  the systems it uses. Queued jobs and their follow-ups are dropped.
- **Parallel texture decode:** `ResourceManager::PrefetchTextures(context, paths)` decodes textures that aren't
  loaded yet on all workers and waits; the following `GetTexture()` calls skip the file read and decode. Loading a
  level uses it for all of the level's tilesets. The GPU upload stays on the main thread as before.
- **Thread rules:** work on workers must not touch Wren, the entt registry, bgfx or the UI. Hand results to a
  follow-up instead. File reads (PhysFS) and `TextureResource::Decode` are safe on workers.

### How it works (phase 1b)

- **Pooled jobs:** jobs live in a pool of fixed slots (chunks of 4,096, growing on demand) and are reused, so
  scheduling doesn't allocate. A `JobHandle` stores the slot index and a generation number; a handle to a reused
  slot still reports complete. Work is stored in the slot inline when it fits in 64 bytes (most lambdas), otherwise
  on the heap.
- **Work-stealing queues:** each worker has its own queue per lane. A worker runs its newest job first (warm
  caches, short chains) and an idle worker steals the oldest job from another. Threads that aren't workers spread
  their jobs across the queues in turn.
- **Free-slot caches:** each worker keeps a small list of free slots, refilled from and returned to a shared list in
  batches of 256.
- **Idle workers** spin briefly, then sleep until woken by new work.

### Without threads

- **Build switch:** `STRUKTUR_THREADS` (a CMake option) is on for desktop and off for Emscripten by default. With it
  off, no `std::thread` code is compiled and `JobSystem::ThreadsAvailable()` returns false.
- **Run-time switch:** `JobSystemConfig{0}` gives zero workers on any build. Use it for debugging, or for web builds
  whose page isn't cross-origin isolated.
- **Same API in both cases:** jobs queue up and run on the calling thread inside `Wait()` or `RunPending()`.
  Nothing runs until something waits or pumps. The engine pumps queued work every frame within the main-thread
  budget (`FrameScheduler::BeginFrame`), so `IsComplete()` keeps advancing on web too.
- **Tested:** the whole test suite runs with 0, 1, 4 and all workers, and also in a `STRUKTUR_THREADS=OFF` build.

## Building, testing, benchmarking

```powershell
cmake -S src/Engine/Jobs -B build-jobs
cmake --build build-jobs --config Release
build-jobs/Release/StrukturJobsTests.exe
build-jobs/Release/job_bench.exe --csv job_bench.csv
```

Add `--quick` for a shorter benchmark run, and `-DSTRUKTUR_THREADS=OFF` for the no-threads build. The root build
links `StrukturJobs` into the game, with tests and the benchmark off.

### Tests (36 cases, ~149k checks)

Every scenario runs with 0, 1, 4 and all workers:
- Exactly-once execution of 20k jobs, and the job counters.
- A 2,000-job dependency chain runs in order.
- A random 20k-job dependency graph across all lanes: no job starts before its dependencies finish.
- Waiting from inside jobs (64 × 16 nested) doesn't deadlock.
- Parallel-for matches a plain loop over 1 M items, including empty and single-item ranges.
- Inline mode: nothing runs until waited or pumped, and the time budget is respected.
- A 200k-long chain completes without overflowing the stack, both inline and on shutdown.
- Lane priority order.
- The main thread waiting on frame work never runs background jobs.
- `WhenAll` and invalid handles.
- Shutdown finishes the running job, drops 1,000 dependents and completes late schedules as dropped.
- Eight threads scheduling 10k jobs each at once.
- Phase 1b: an old handle to a reused slot still reads as complete; the pool grows past 30k live jobs; work of
  any size (inline or heap) runs once and is destroyed once, including dropped work; background jobs never use
  more workers than the limit while a frame job still starts promptly; idle workers steal spawned jobs.
- Phase 2 (`Tests/TaskTests.cpp`):
  - Results, `Task<void>`, invalid tasks, and polling from a simulated update loop.
  - Tasks depending on other tasks, including a 100-way fan-in.
  - Cancelling before start (the work never runs) and while running (the result is discarded).
  - Progress from a worker, and step by step without workers.
  - Follow-ups:
    - run only inside `RunMainThreadWork()` and on its thread;
    - one added late still runs;
    - cancelled tasks still get theirs.
  - Main-thread work runs in order and respects its budget.
  - Stepped jobs run every step once and can be cancelled between steps.
  - With one worker busy streaming, a frame job starts within about one step.
  - Without workers, streaming spans frames and each frame stays within budget plus one step.
  - Shutdown drops unfinished tasks: no follow-ups, work destroyed exactly once.
  - Four threads creating 2,000 tasks each.

**Stability:** 40 Release runs and 10 Debug runs in a row all passed, plus the `STRUKTUR_THREADS=OFF` build.

## Measured

Release build on a 20-thread desktop CPU (Windows, MSVC). Each cell is phase 1 → phase 1b. Full numbers:
`job_bench.exe --csv`.

| Benchmark | 0 workers (inline) | 1 | 4 | 8 | 19 |
|---|---|---|---|---|---|
| Schedule + wait, one at a time (ns/job) | 121 → 90 | 352 → 175 | 950 → 601 | 1393 → 1140 | 1402 → 1980 |
| Schedule + complete, batched (ns/job) | 312 → 158 | 585 → 334 | 1125 → 405 | 1458 → 534 | 1468 → 647 |
| Tiny-job throughput, 1 M jobs (M jobs/s) | 3.15 → 6.25 | 1.96 → 3.23 | 0.83 → 2.36 | 0.63 → 1.97 | 0.58 → 1.61 |
| Parallel-for, 200k items × ~0.8 µs: speed-up | 1.0× | 2.0× | 4.9× | 8.7× | 17.8× → 17.7× |
| Frame job start under background load, p50 (µs) | – | 2720 → 2330 | 2380 → 2.6 | 2730 → 0.6 | 1100 → 2.7 |
| Frame job start under background load, p99 (µs) | – | 4800 → 4030 | 4380 → 34 | 4790 → 39 | 2550 → 454 |
| 10k dependency chain (ns/link) | 271 → 125 | 495 → 80 | 727 → 162 | 1406 → 519 | 1521 → 451 |
| 8 producers × 20k jobs (M jobs/s) | – | 1.54 → 1.71 | 1.25 → 2.60 | 0.97 → 2.36 | 0.66 → 2.34 |
| Simulated frame, mean (ms) | 48.5 | 24.8 → 25.1 | 10.1 → 10.4 | 6.6 | 3.6 |
| Simulated frame + background streaming, mean / p99 (ms) | 50.4 / 52.5 | 25.9 / 27.4 | 11.1 / 11.8 | 7.1 / 7.5 | 5.1 / 5.9 → 5.0 / 5.7 |

The baseline is a plain `std::function` call: 0.8 ns.

**What this says:**
- **Per-job cost roughly halved, and contention mostly went away.** Batched scheduling costs 0.3–0.65 µs at
  any worker count (was up to 1.5 µs), tiny-job throughput is 2–3× higher, and eight producers scheduling at once
  now get 2.3–2.6 M jobs/s instead of dropping to 0.66 M. Throughput still falls a little as workers are added:
  more threads stealing from each other for jobs that take nanoseconds. Jobs of a few µs or more don't notice.
- **One-at-a-time schedule + wait got slower at 19 workers** (1.4 → 2.0 µs). Each job wakes a sleeping worker,
  and with that many workers the wake-up and the steal attempts cost more than the job. It's the worst case (one
  tiny job, then wait); batching, parallel-for or running it inline avoids it.
- **Frame work no longer waits behind background work.** With 4 or more workers, the background limit keeps two
  workers free, so a frame job starts within a few µs (p50) instead of 2.4–2.7 ms. With a single worker there is
  nothing to reserve, so it still waits until the main thread helps; phase 2's step-based background jobs address
  that case.
- **Parallel work is unchanged.** Parallel-for keeps 89–99 % efficiency, and the simulated frame still goes from
  48 ms to 3.6 ms. Those jobs are big enough that per-job cost never mattered.

### Phase 2: tasks, steps and follow-ups

Same machine and build. Plain-job numbers didn't change: the step check costs nothing measurable. Batched
schedule + complete was 0.16 / 0.31 / 0.39 / 0.49 / 0.58 µs at 0 / 1 / 4 / 8 / 19 workers.

| Benchmark | 0 workers (inline) | 1 | 4 | 8 | 19 |
|---|---|---|---|---|---|
| Task run + wait, one at a time (ns/task) | 150 | 235 | 668 | 1210 | 1973 |
| Task run + complete, batched (ns/task) | 275 | 309 | 396 | 515 | 753 |
| … plus a main-thread follow-up per task (ns/task) | 482 | 550 | 777 | 857 | 973 |
| Running one follow-up on the main thread (ns) | 129 | 137 | 135 | 144 | 230 |
| One stepped job, cost per step (ns) | 33 | 24 | 285 | 249 | 206 |
| Frame job start, background load as one long job: p50 / p99 (µs) | – | 2549 / 4236 | 0.7 / 17 | 2.8 / 40 | 1.0 / 825 |
| Frame job start, same load as 0.5 ms steps: p50 / p99 (µs) | – | 259 / 647 | 0.7 / 22 | 2.5 / 31 | 41 / 396 |

**What this says:**
- **A task costs about the same as a plain job,** plus its shared state: 0.3–0.75 µs batched against 0.16–0.58 µs.
  A main-thread follow-up adds about 0.2–0.4 µs in all, 0.13 µs of it on the main thread. That's negligible for the
  loads and builds tasks are meant for.
- **Stepped background work fixes the single-worker case.** With one worker there's no spare worker to reserve, so
  a frame job used to wait out the whole background job (p50 2.5 ms). With the same work split into 0.5 ms steps,
  it waits for the current step to end (p50 0.26 ms, p99 0.65 ms). Finer steps wait less.
- **With 4+ workers the background limit already does the job.** Steps change little there. At 19 workers the
  median rose to 41 µs, because requeued steps keep waking idle workers, while the worst 1 % halved.
- **A step costs 25–300 ns** of queueing. Steps of 0.1 ms or more lose well under 1 % to it.

### Phase 3 in the game

Debug game build (`build-nogame`), muted 45-second runs into the overworld:
- **19 workers:** the first level's 5 tilesets decoded in one parallel prefetch, and all 5 textures came from it.
- **`--threads=0`:** same result.
- **Both runs:** no errors, and the save files were unchanged.
- **Shutdown:** closing the window shut the game down within 30 seconds, with no errors. The log stops partway
  through cleanup because stdout is buffered, and the exit code wasn't captured.

**Not measured yet:** a level's load time or a frame's time, before against after. Five small tilesets is too little
work to show a difference; the ride world's chunk building is the meaningful benchmark.

### Phase 3, ride part (branch `RideSoftware`)

- **Chunk streaming:** `ChunkStreamer` no longer owns a thread. Each chunk build is a background `Task`, so builds
  run on as many workers as the background limit allows instead of one. A null job system builds inline (tests).
  - A settings change cancels queued builds and takes their meshes back at once (an atomic claim flag the build
    checks first); running builds finish and come back stale.
  - Destroying the streamer reclaims unstarted builds and waits for running ones, since they write into its meshes.
  - `GameContext` now stops the job system first in `Shutdown()` but destroys it last, so the ride world and
    session can still wait on their (by then complete) tasks while being torn down.
- **Ride autosave:** a background task, not `PostFrame`. A post-frame job must finish by the next frame, and a slow
  disk would stall that frame; a background task can't. The main thread copies the ride and builds the JSON text,
  since those read live session state; the worker encodes the FIT file and writes both files atomically. The result
  comes back as the usual SAVED / SAVE_FAILED session event. `Finish()` waits for an autosave still in flight, so
  it can't overwrite the final files.
- **Tests** (build-fitness, 106 cases, 10 runs in a row): job-system against inline chunk builds with 4 and 0
  workers, settings flipped every 7 frames mid-stream, worlds destroyed right after requesting chunks, and
  background autosaves with 2 and 0 workers (files written, SAVED reported, `Finish()` still correct).
- **In the game:** a `--demo` free ride (Debug, muted) for 82 seconds, past the 60-second autosave. The world
  streamed and rendered normally, there were no errors, and the final FIT and JSON were written. Closing the window
  then exited normally.
- **Not measured:** chunk build throughput against the old single worker thread.

## Next

- **Phase 4:** ECS systems as jobs (access declarations, a dependency graph per phase, command lists for entity
  changes). Every existing system starts out marked main-thread-only.
- **Phase 5:** Wren task handles (`if (load.isComplete)`).
- **Phase 6:** optional threaded web build (pthread pool and cross-origin isolation headers).

**Not covered yet:**
- ThreadSanitizer (not available with MSVC).
- Web, Linux and macOS builds of the library.
- Real-world benchmarks: chunk build throughput against the old worker thread, and game frame time.
- The editor build (`build-windows-debug`) and the web build haven't been rebuilt with the engine changes.
