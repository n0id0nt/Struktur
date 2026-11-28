#pragma once

// Helpers shared by the job system tests.
#include <chrono>
#include <thread>
#include <vector>

#include "Engine/Jobs/JobSystem.h"

// Zero workers (inline mode), one, four and all - or only zero when threads are compiled out.
inline std::vector<int> WorkerCounts()
{
	std::vector<int> counts = {0};
	if (Struktur::Jobs::JobSystem::ThreadsAvailable())
	{
		counts.push_back(1);
		counts.push_back(4);
		const int all = Struktur::Jobs::JobSystem::ResolveWorkerCount(-1);
		if (all > 4)
		{
			counts.push_back(all);
		}
	}
	return counts;
}

inline void SleepMilliseconds(int milliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// Busy for a precise time (sleep_for rounds up to the OS timer - ~15 ms on Windows).
inline void BusyMilliseconds(double milliseconds)
{
	const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	while (std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() < milliseconds)
	{
	}
}

inline double MillisecondsSince(std::chrono::steady_clock::time_point start)
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}
