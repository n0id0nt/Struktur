# Job system

The engine's job system and scheduler. **Phase 1 is done:** worker threads, lanes, dependencies, waiting that helps,
parallel-for and inline mode, with unit tests and a benchmark. Phases 2–6 (tasks, frame integration, ECS systems
as jobs, Wren handles, web threads) are planned at the end.

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

### Without threads

- **Build switch:** `STRUKTUR_THREADS` (a CMake option) is on for desktop and off for Emscripten by default. With it
  off, no `std::thread` code is compiled and `JobSystem::ThreadsAvailable()` returns false.
- **Run-time switch:** `JobSystemConfig{0}` gives zero workers on any build. Use it for debugging, or for web builds
  whose page isn't cross-origin isolated.
- **Same API in both cases:** jobs queue up and run on the calling thread inside `Wait()` or `RunPending()`.
  Nothing runs until something waits or pumps. Phase 3 will make the engine pump background work each frame
  within a budget, so `IsComplete()` keeps advancing on web too.
- **Tested:** the whole test suite runs with 0, 1, 4 and all workers, and also in a `STRUKTUR_THREADS=OFF` build.

## Building, testing, benchmarking

```powershell
cmake -S src/Engine/Jobs -B build-jobs
cmake --build build-jobs --config Release
build-jobs/Release/StrukturJobsTests.exe
build-jobs/Release/job_bench.exe --csv job_bench.csv
```

Add `--quick` for a shorter benchmark run, and `-DSTRUKTUR_THREADS=OFF` for the no-threads build. The root build
links `StrukturJobs` into the game, with tests and the benchmark off. Nothing in the engine uses it yet; that's
phase 3.

### Tests (13 cases, ~84k checks)

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

**Stability:** 40 Release runs and 10 Debug runs in a row all passed.

## Measured (phase 1)

Release build on a 20-thread desktop CPU (Windows, MSVC). Full numbers: `job_bench.exe --csv`.

| Benchmark | 0 workers (inline) | 1 | 4 | 8 | 19 |
|---|---|---|---|---|---|
| Schedule + wait, one at a time (ns/job) | 121 | 352 | 950 | 1393 | 1402 |
| Schedule + complete, batched (ns/job) | 312 | 585 | 1125 | 1458 | 1468 |
| Tiny-job throughput, 1 M jobs (M jobs/s) | 3.15 | 1.96 | 0.83 | 0.63 | 0.58 |
| Parallel-for, 200k items × ~0.8 µs: speed-up | 1.0× | 2.0× | 4.9× | 8.7× | 17.8× |
| Parallel-for efficiency | 100 % | 99 % | 98 % | 97 % | 89 % |
| 10k dependency chain (ns/link) | 271 | 495 | 727 | 1406 | 1521 |
| 8 producers × 20k jobs (M jobs/s) | – | 1.54 | 1.25 | 0.97 | 0.66 |
| Simulated frame, mean (ms) | 48.5 | 24.8 | 10.1 | 6.6 | 3.6 |
| Simulated frame + background streaming, mean / p99 (ms) | 50.4 / 52.1 | 25.7 / 27.6 | 11.1 / 12.0 | 7.1 / 7.9 | 5.1 / 5.9 |

The baseline is a plain `std::function` call: 0.8 ns.

**What this says:**
- **Parallel work scales well.** Parallel-for keeps 89–99 % efficiency up to all 19 workers, and the simulated
  frame (a parallel-for phase, dependent jobs, 1 ms of main-thread work) goes from 48 ms to 3.6 ms. This passes
  the plan's target of at least 70 % efficiency at 8 threads, with 97 % measured.
- **Per-job cost misses the target beyond about 2 workers.** It's 0.35 µs with one worker but about 1.4 µs at
  8 or more, and throughput for very small jobs drops as workers are added. Every schedule and pop goes through
  one locked queue per lane, and each job is a `shared_ptr` plus a `std::function`. That's the "simple first"
  design from the plan, and the benchmark shows where it stops scaling. For jobs of 50 µs or more (what systems
  and streaming will be) the cost is negligible. For very fine-grained jobs it isn't yet: use parallel-for with a
  bigger grain instead.
- **Background work can delay frame work.** In the latency test every worker is inside a long background job, so
  frame jobs wait until a worker frees up or the main thread helps (p50 about 2.5 ms, p99 about 4.7 ms). Lanes
  pick the next job but can't pre-empt one that's running. The "+ background" frame rows show the same effect at
  high worker counts (+1.5 ms at 16–19 workers, with that many 2 ms streaming jobs in flight).

## Next

- **Phase 1b** (driven by the numbers above):
  1. Per-worker work-stealing queues and pooled job objects (no `shared_ptr` / `std::function` allocation per
     job) to cut per-job cost and contention.
  2. Cap how many workers may run background jobs at once (e.g. all but two), so frame work always finds a free
     worker.
- **Phase 2:** tasks with results, progress and cancellation; follow-up steps that run on the main thread;
  step-based background jobs that run within a per-frame budget (long jobs give way between steps, which also
  fixes the latency issue).
- **Phase 3:** frame integration (sync points in `UpdateLoop`, the post-frame lane, a per-frame inline budget),
  plus moving the ride world's `ChunkStreamer`, texture decoding and the ride autosave onto it.
- **Phase 4:** ECS systems as jobs (access declarations, a dependency graph per phase, command lists for entity
  changes). Every existing system starts out marked main-thread-only.
- **Phase 5:** Wren task handles (`if (load.isComplete)`).
- **Phase 6:** optional threaded web build (pthread pool and cross-origin isolation headers).

**Not covered yet:**
- ThreadSanitizer (not available with MSVC).
- Web, Linux and macOS builds of the library.
- The real-world benchmarks (chunk building, game frame time) wait for phase 3, when the engine uses the job
  system.
