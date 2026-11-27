# Job system

The engine's job system and scheduler. **Phases 1 and 1b are done:** worker threads, lanes, dependencies, waiting
that helps, parallel-for and inline mode (phase 1), then pooled jobs, per-worker work-stealing queues and a cap on
background workers (phase 1b), with unit tests and a benchmark. Phases 2–6 (tasks, frame integration, ECS systems
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
- **Background limit:** `JobSystemConfig{workers, backgroundWorkerLimit}` caps how many workers run background jobs
  at once. The default (-1) keeps workers free for frame work: all workers when there's 1, all but one for 2–3, all
  but two otherwise. Threads helping inside `Wait()` don't count towards it.
- **Stats:** `GetStats()` reports jobs scheduled, run, run by helpers, stolen and dropped, how many jobs needed heap
  storage for their work, and the pool size.

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

### Tests (18 cases, ~84k checks)

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

## Next

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
