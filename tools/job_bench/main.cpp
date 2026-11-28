// job_bench - measures the job system (docs/JobSystem.md). Build Release:
//   cmake -S src/Engine/Jobs -B build-jobs && cmake --build build-jobs --config Release --target job_bench
//   build-jobs/Release/job_bench.exe [--quick] [--csv results.csv]
//
// Every benchmark runs for a sweep of worker counts (0 = inline mode, up to cores - 1) and prints one table row per
// result; --csv also writes them as benchmark,workers,metric,value,unit.
//   overhead     schedule + wait per job: one at a time, and batched; plus a plain std::function call as a baseline
//   throughput   tiny jobs per second for 10k / 100k / 1M jobs
//   parallelFor  speed-up and efficiency on a CPU-bound loop over 200k "entities"
//   latency      schedule-to-start time (p50 / p99) while background work keeps the workers busy
//   chain        a 10k-long dependency chain: cost per link
//   contention   8 threads scheduling 20k jobs each at the same time
//   frame        a simulated frame (parallel-for phase, dependent jobs, main-thread work) with and without
//                background streaming load: frame time mean / p50 / p99 / stddev and main-thread wait
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "Engine/Jobs/JobSystem.h"
#include "Engine/Jobs/Task.h"

using namespace Struktur::Jobs;
using Clock = std::chrono::steady_clock;

struct Result
{
	std::string benchmark;
	int workers;
	std::string metric;
	double value;
	std::string unit;
};

static std::vector<Result> s_results;
static bool s_quick = false;

static void Report(const std::string& benchmark, int workers, const std::string& metric, double value,
                   const std::string& unit)
{
	s_results.push_back(Result{benchmark, workers, metric, value, unit});
	std::printf("  %-12s workers=%-3d %-28s %12.3f %s\n", benchmark.c_str(), workers, metric.c_str(), value, unit.c_str());
	std::fflush(stdout);
}

static double SecondsSince(Clock::time_point start)
{
	return std::chrono::duration<double>(Clock::now() - start).count();
}

// Burns CPU for `microseconds` of wall time (models a job of known length).
static void BusyFor(double microseconds)
{
	const Clock::time_point start = Clock::now();
	volatile double sink          = 0.0;
	while (std::chrono::duration<double, std::micro>(Clock::now() - start).count() < microseconds)
	{
		sink = sink + std::sqrt(sink + 1.0);
	}
}

// A fixed amount of arithmetic per item (deterministic work, for speed-up measurements).
static float EntityWork(float seed)
{
	float value = seed;
	for (int step = 0; step < 48; ++step)
	{
		value = std::sin(value) * 0.5f + std::cos(value * 1.3f) + 0.001f * step;
	}
	return value;
}

static double Percentile(std::vector<double> values, double fraction)
{
	if (values.empty())
	{
		return 0.0;
	}
	std::sort(values.begin(), values.end());
	const size_t index = std::min(values.size() - 1, static_cast<size_t>(fraction * (values.size() - 1) + 0.5));
	return values[index];
}

static double Mean(const std::vector<double>& values)
{
	double sum = 0.0;
	for (double value : values)
	{
		sum += value;
	}
	return values.empty() ? 0.0 : sum / static_cast<double>(values.size());
}

static double StandardDeviation(const std::vector<double>& values)
{
	const double mean = Mean(values);
	double sum        = 0.0;
	for (double value : values)
	{
		sum += (value - mean) * (value - mean);
	}
	return values.size() < 2 ? 0.0 : std::sqrt(sum / static_cast<double>(values.size() - 1));
}

static void EmptyJob()
{
}

static std::atomic<int64_t> s_counter{0};

static void CountJob()
{
	s_counter.fetch_add(1, std::memory_order_relaxed);
}

// --- benchmarks --------------------------------------------------------------------------------------------------

static void BenchOverhead(int workers)
{
	JobSystem jobs(JobSystemConfig{workers});
	const int count = s_quick ? 20000 : 100000;
	{
		const Clock::time_point start = Clock::now();
		for (int index = 0; index < count; ++index)
		{
			jobs.Wait(jobs.Schedule(EmptyJob));
		}
		Report("overhead", workers, "schedule+wait each", SecondsSince(start) / count * 1e9, "ns/job");
	}
	{
		std::vector<JobHandle> handles;
		handles.reserve(count);
		const Clock::time_point start = Clock::now();
		for (int index = 0; index < count; ++index)
		{
			handles.push_back(jobs.Schedule(EmptyJob));
		}
		const double scheduled = SecondsSince(start);
		jobs.Wait(jobs.WhenAll(handles));
		const double total = SecondsSince(start);
		Report("overhead", workers, "schedule only (batch)", scheduled / count * 1e9, "ns/job");
		Report("overhead", workers, "schedule+complete (batch)", total / count * 1e9, "ns/job");
	}
	if (workers == 0)
	{
		std::function<void()> call = EmptyJob;
		const Clock::time_point start = Clock::now();
		for (int index = 0; index < count * 10; ++index)
		{
			call();
		}
		Report("overhead", workers, "baseline std::function call", SecondsSince(start) / (count * 10) * 1e9, "ns/call");
	}
}

static void BenchThroughput(int workers)
{
	JobSystem jobs(JobSystemConfig{workers});
	const int sizes[] = {10000, 100000, 1000000};
	for (int size : sizes)
	{
		if (s_quick && size > 100000)
		{
			continue;
		}
		s_counter = 0;
		std::vector<JobHandle> handles;
		handles.reserve(static_cast<size_t>(size));
		const Clock::time_point start = Clock::now();
		for (int index = 0; index < size; ++index)
		{
			handles.push_back(jobs.Schedule(CountJob));
		}
		jobs.Wait(jobs.WhenAll(handles));
		const double seconds = SecondsSince(start);
		if (s_counter.load() != size)
		{
			std::printf("  !! throughput: %lld of %d jobs ran\n", static_cast<long long>(s_counter.load()), size);
		}
		Report("throughput", workers, std::to_string(size) + " tiny jobs", size / seconds / 1e6, "M jobs/s");
	}
}

static void BenchParallelFor(int workers, double sequentialSeconds, std::vector<float>& data)
{
	JobSystem jobs(JobSystemConfig{workers});
	const size_t grain = 2048;
	double best        = 1e9;
	for (int repeat = 0; repeat < 5; ++repeat)
	{
		const Clock::time_point start = Clock::now();
		jobs.Wait(jobs.ParallelFor(0, data.size(), grain,
		                           [&data](size_t first, size_t last)
		                           {
			                           for (size_t index = first; index < last; ++index)
			                           {
				                           data[index] = EntityWork(data[index]);
			                           }
		                           }));
		best = std::min(best, SecondsSince(start));
	}
	const double speedup = sequentialSeconds / best;
	Report("parallelFor", workers, "time (best of 5)", best * 1000.0, "ms");
	Report("parallelFor", workers, "speed-up vs plain loop", speedup, "x");
	Report("parallelFor", workers, "efficiency (speed-up / threads)", speedup / (workers + 1) * 100.0, "%");
}

// Background load: one long job per worker in flight - 200 slices of 0.5 ms, either as one job (`stepped` false) or as
// a stepped job that gives way after each slice.
static void BenchLatency(int workers, bool stepped)
{
	if (workers == 0)
	{
		return;  // inline jobs start when someone pumps - there is no latency to measure
	}
	JobSystem jobs(JobSystemConfig{workers});
	std::atomic<bool> stopLoad{false};
	std::vector<JobHandle> load;
	for (int index = 0; index < workers; ++index)
	{
		if (stepped)
		{
			load.push_back(jobs.Schedule(
			    [&stopLoad, slice = 0]() mutable
			    {
				    BusyFor(500);
				    return ++slice < 200 && !stopLoad.load() ? StepResult::CONTINUE : StepResult::DONE;
			    },
			    Lane::BACKGROUND));
			continue;
		}
		load.push_back(jobs.Schedule(
		    [&stopLoad]
		    {
			    for (int slice = 0; slice < 200 && !stopLoad.load(); ++slice)
			    {
				    BusyFor(500);
			    }
		    },
		    Lane::BACKGROUND));
	}
	const int count = s_quick ? 2000 : 10000;
	std::vector<double> latency(static_cast<size_t>(count), 0.0);
	std::vector<JobHandle> handles;
	handles.reserve(static_cast<size_t>(count));
	for (int index = 0; index < count; ++index)
	{
		const Clock::time_point scheduled = Clock::now();
		handles.push_back(jobs.Schedule(
		    [&latency, index, scheduled]
		    { latency[static_cast<size_t>(index)] = std::chrono::duration<double, std::micro>(Clock::now() - scheduled).count(); },
		    Lane::FRAME));
		if (index % 64 == 63)
		{
			BusyFor(20);  // bursts, not one flood
		}
	}
	jobs.WaitAll(handles);
	stopLoad = true;
	jobs.WaitAll(load);
	const std::string loadName = stepped ? " (stepped load)" : " (busy)";
	Report("latency", workers, "schedule-to-start p50" + loadName, Percentile(latency, 0.50), "us");
	Report("latency", workers, "schedule-to-start p99" + loadName, Percentile(latency, 0.99), "us");
}

// Tasks against plain jobs: the cost of a Task (shared state, status, result), of a step and of a main-thread follow-up.
static void BenchTasks(int workers)
{
	JobSystem jobs(JobSystemConfig{workers});
	const int count = s_quick ? 20000 : 100000;
	{
		const Clock::time_point start = Clock::now();
		for (int index = 0; index < count; ++index)
		{
			jobs.Wait(jobs.Run([index] { return index; }, Lane::FRAME));
		}
		Report("tasks", workers, "run+wait each", SecondsSince(start) / count * 1e9, "ns/task");
	}
	{
		std::vector<Task<int>> tasks;
		std::vector<JobHandle> handles;
		tasks.reserve(static_cast<size_t>(count));
		handles.reserve(static_cast<size_t>(count));
		const Clock::time_point start = Clock::now();
		for (int index = 0; index < count; ++index)
		{
			tasks.push_back(jobs.Run([index] { return index; }, Lane::FRAME));
			handles.push_back(tasks.back().GetHandle());
		}
		jobs.Wait(jobs.WhenAll(handles));
		Report("tasks", workers, "run+complete (batch)", SecondsSince(start) / count * 1e9, "ns/task");
	}
	{
		// Follow-ups: every task hands its result to the main thread, which runs them all in one RunMainThreadWork().
		int64_t sum = 0;
		std::vector<JobHandle> handles;
		handles.reserve(static_cast<size_t>(count));
		const Clock::time_point start = Clock::now();
		for (int index = 0; index < count; ++index)
		{
			Task<int> task = jobs.Run([index] { return index; }, Lane::FRAME);
			task.ThenOnMainThread([&sum](Task<int>& done) { sum += done.GetResult(); });
			handles.push_back(task.GetHandle());
		}
		jobs.Wait(jobs.WhenAll(handles));
		const Clock::time_point followUps = Clock::now();
		jobs.RunMainThreadWork(1e9);
		Report("tasks", workers, "run+complete+follow-up (batch)", SecondsSince(start) / count * 1e9, "ns/task");
		Report("tasks", workers, "follow-up on main thread", SecondsSince(followUps) / count * 1e9, "ns/follow-up");
		if (sum != static_cast<int64_t>(count) * (count - 1) / 2)
		{
			std::printf("  !! tasks: follow-ups summed %lld\n", static_cast<long long>(sum));
		}
	}
	{
		// One stepped job: the cost of going round the queue between steps.
		const int steps = count;
		int done        = 0;
		const Clock::time_point start = Clock::now();
		jobs.Wait(jobs.Schedule([&done, steps] { return ++done < steps ? StepResult::CONTINUE : StepResult::DONE; }, Lane::FRAME));
		Report("tasks", workers, "stepped job, per step", SecondsSince(start) / steps * 1e9, "ns/step");
	}
}

static void BenchChain(int workers)
{
	JobSystem jobs(JobSystemConfig{workers});
	const int length = s_quick ? 2000 : 10000;
	const Clock::time_point start = Clock::now();
	JobHandle previous;
	for (int index = 0; index < length; ++index)
	{
		previous = jobs.Schedule(EmptyJob, Lane::FRAME, previous);
	}
	jobs.Wait(previous);
	Report("chain", workers, std::to_string(length) + "-link chain", SecondsSince(start) / length * 1e9, "ns/link");
}

static void BenchContention(int workers)
{
	if (!JobSystem::ThreadsAvailable() || workers == 0)
	{
		return;
	}
	JobSystem jobs(JobSystemConfig{workers});
	const int producers = 8;
	const int perThread = s_quick ? 5000 : 20000;
	s_counter           = 0;
	const Clock::time_point start = Clock::now();
	std::vector<std::thread> threads;
	for (int producer = 0; producer < producers; ++producer)
	{
		threads.emplace_back(
		    [&jobs, perThread]
		    {
			    std::vector<JobHandle> handles;
			    handles.reserve(static_cast<size_t>(perThread));
			    for (int index = 0; index < perThread; ++index)
			    {
				    handles.push_back(jobs.Schedule(CountJob));
			    }
			    jobs.WaitAll(handles);
		    });
	}
	for (std::thread& thread : threads)
	{
		thread.join();
	}
	const double seconds = SecondsSince(start);
	Report("contention", workers, "8 producers x " + std::to_string(perThread), producers * perThread / seconds / 1e6, "M jobs/s");
}

// One frame: a parallel-for over the "entities" (phase A), four jobs that depend on it (phase B), and 1 ms of
// main-thread work in parallel with phase B. Background load: a pool of 2 ms streaming jobs kept topped up; with zero
// workers they get a 2 ms per-frame budget on the main thread, as the engine would give them.
static void BenchFrame(int workers, bool withLoad)
{
	JobSystem jobs(JobSystemConfig{workers});
	const int frames = s_quick ? 120 : 300;
	std::vector<float> entities(60000, 0.5f);
	std::vector<double> frameMs;
	std::vector<double> waitMs;
	std::vector<JobHandle> streaming;
	const size_t streamingInFlight = static_cast<size_t>(std::max(2, workers));
	for (int frame = 0; frame < frames; ++frame)
	{
		if (withLoad)
		{
			streaming.erase(std::remove_if(streaming.begin(), streaming.end(), [](const JobHandle& handle) { return handle.IsComplete(); }),
			                streaming.end());
			while (streaming.size() < streamingInFlight)
			{
				streaming.push_back(jobs.Schedule([] { BusyFor(2000); }, Lane::BACKGROUND));
			}
		}
		const Clock::time_point start = Clock::now();
		JobHandle phaseA = jobs.ParallelFor(0, entities.size(), 2048,
		                                    [&entities](size_t first, size_t last)
		                                    {
			                                    for (size_t index = first; index < last; ++index)
			                                    {
				                                    entities[index] = EntityWork(entities[index]);
			                                    }
		                                    });
		std::vector<JobHandle> phaseB;
		for (int index = 0; index < 4; ++index)
		{
			phaseB.push_back(jobs.Schedule([] { BusyFor(250); }, Lane::FRAME, phaseA));
		}
		BusyFor(1000);  // main-thread-only work (scripts, UI)
		const Clock::time_point waitStart = Clock::now();
		jobs.Wait(jobs.WhenAll(phaseB));
		waitMs.push_back(SecondsSince(waitStart) * 1000.0);
		if (withLoad && workers == 0)
		{
			jobs.RunPending(0.002, Lane::BACKGROUND);
		}
		frameMs.push_back(SecondsSince(start) * 1000.0);
	}
	jobs.WaitAll(streaming);
	const std::string name = withLoad ? "frame+load" : "frame";
	Report(name, workers, "frame mean", Mean(frameMs), "ms");
	Report(name, workers, "frame p50", Percentile(frameMs, 0.50), "ms");
	Report(name, workers, "frame p99", Percentile(frameMs, 0.99), "ms");
	Report(name, workers, "frame stddev", StandardDeviation(frameMs), "ms");
	Report(name, workers, "main thread in Wait (mean)", Mean(waitMs), "ms");
}

int main(int argc, char** argv)
{
	std::string csvPath;
	for (int index = 1; index < argc; ++index)
	{
		if (std::strcmp(argv[index], "--quick") == 0)
		{
			s_quick = true;
		}
		else if (std::strcmp(argv[index], "--csv") == 0 && index + 1 < argc)
		{
			csvPath = argv[++index];
		}
	}
	const int maxWorkers = JobSystem::ResolveWorkerCount(-1);
	std::vector<int> sweep = {0};
	for (int workers : {1, 2, 4, 8, 12, 16})
	{
		if (workers < maxWorkers)
		{
			sweep.push_back(workers);
		}
	}
	if (maxWorkers > 0)
	{
		sweep.push_back(maxWorkers);
	}
	std::printf("job_bench: %u logical cores, threads %s, sweep up to %d workers%s\n",
	            std::thread::hardware_concurrency(), JobSystem::ThreadsAvailable() ? "on" : "off", maxWorkers,
	            s_quick ? " (quick)" : "");

	std::printf("\n[overhead]\n");
	for (int workers : sweep)
	{
		BenchOverhead(workers);
	}
	std::printf("\n[throughput]\n");
	for (int workers : sweep)
	{
		BenchThroughput(workers);
	}
	std::printf("\n[parallelFor]\n");
	std::vector<float> data(200000, 0.25f);
	double sequential = 1e9;
	for (int repeat = 0; repeat < 5; ++repeat)
	{
		const Clock::time_point start = Clock::now();
		for (float& value : data)
		{
			value = EntityWork(value);
		}
		sequential = std::min(sequential, SecondsSince(start));
	}
	Report("parallelFor", 0, "plain loop (best of 5)", sequential * 1000.0, "ms");
	for (int workers : sweep)
	{
		BenchParallelFor(workers, sequential, data);
	}
	std::printf("\n[latency]\n");
	for (int workers : sweep)
	{
		BenchLatency(workers, false);
		BenchLatency(workers, true);
	}
	std::printf("\n[chain]\n");
	for (int workers : sweep)
	{
		BenchChain(workers);
	}
	std::printf("\n[contention]\n");
	for (int workers : sweep)
	{
		BenchContention(workers);
	}
	std::printf("\n[tasks]\n");
	for (int workers : sweep)
	{
		BenchTasks(workers);
	}
	std::printf("\n[frame]\n");
	for (int workers : sweep)
	{
		BenchFrame(workers, false);
		BenchFrame(workers, true);
	}

	if (!csvPath.empty())
	{
		std::ofstream csv(csvPath);
		csv << "benchmark,workers,metric,value,unit\n";
		for (const Result& result : s_results)
		{
			csv << result.benchmark << "," << result.workers << "," << result.metric << "," << result.value << ","
			    << result.unit << "\n";
		}
		std::printf("\nwrote %s\n", csvPath.c_str());
	}
	return 0;
}
