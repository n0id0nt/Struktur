#include "ChunkStreamer.h"

#include <algorithm>
#include <cmath>

namespace Struktur::RideWorld
{
static constexpr double SAMPLE_MARGIN_METERS = 20.0;  // route samples copied either side of a chunk (bend radius)
static constexpr int MAX_REQUESTS_PER_UPDATE = 4;

static bool ByChunkIndex(const ChunkMesh* first, const ChunkMesh* second)
{
	return first->chunkIndex < second->chunkIndex;
}

ChunkStreamer::ChunkStreamer(bool threaded)
    : m_threaded(threaded),
      m_generation(1),
      m_busy(false),
      m_stop(false)
{
	if (m_threaded)
	{
		m_worker = std::thread(&ChunkStreamer::WorkerLoop, this);
	}
}

ChunkStreamer::~ChunkStreamer()
{
	if (m_threaded)
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_stop = true;
			m_jobs.clear();
		}
		m_wake.notify_all();
		m_worker.join();
	}
}

void ChunkStreamer::Configure(const ChunkSettings& settings, BiomeList biomes,
                              std::shared_ptr<const PropTypeTable> propTypes)
{
	m_settings  = settings;
	m_biomes    = std::move(biomes);
	m_propTypes = std::move(propTypes);
	++m_generation;
	// Drop queued work; anything mid-build comes back with the old generation and is recycled.
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (Job& job : m_jobs)
		{
			m_freeMeshes.push_back(job.mesh);
		}
		m_jobs.clear();
	}
	m_pendingIndices.clear();
	for (ChunkMesh* mesh : m_ready)
	{
		m_released.push_back(mesh->chunkIndex);
		ReleaseMesh(mesh);
	}
	m_ready.clear();
	m_newlyReady.clear();
}

const ChunkSettings& ChunkStreamer::GetSettings() const
{
	return m_settings;
}

ChunkMesh* ChunkStreamer::AcquireMesh()
{
	if (!m_freeMeshes.empty())
	{
		ChunkMesh* mesh = m_freeMeshes.back();
		m_freeMeshes.pop_back();
		return mesh;
	}
	m_allMeshes.push_back(std::make_unique<ChunkMesh>());
	return m_allMeshes.back().get();
}

void ChunkStreamer::ReleaseMesh(ChunkMesh* mesh)
{
	mesh->chunkIndex = -1;
	m_freeMeshes.push_back(mesh);
}

bool ChunkStreamer::IsPendingOrReady(int64_t chunkIndex) const
{
	if (std::find(m_pendingIndices.begin(), m_pendingIndices.end(), chunkIndex) != m_pendingIndices.end())
	{
		return true;
	}
	for (const ChunkMesh* mesh : m_ready)
	{
		if (mesh->chunkIndex == chunkIndex)
		{
			return true;
		}
	}
	return false;
}

void ChunkStreamer::RequestChunk(Route& route, BiomeSequence& sequence, int64_t chunkIndex)
{
	const double start = static_cast<double>(chunkIndex) * m_settings.chunkLength;
	const double end   = start + m_settings.chunkLength;
	route.EnsureGenerated(end + SAMPLE_MARGIN_METERS);

	Job job;
	job.generation          = m_generation;
	job.mesh                = AcquireMesh();
	job.job.chunkIndex      = chunkIndex;
	job.job.start           = start;
	job.job.settings        = m_settings;
	job.job.biomes          = m_biomes;
	job.job.propTypes       = m_propTypes;
	const double spacing    = route.GetSettings().sampleSpacingMeters;
	job.job.sampleSpacing   = spacing;
	const int64_t first     = std::max(route.GetFirstIndex(),
                                   static_cast<int64_t>(std::floor((start - SAMPLE_MARGIN_METERS) / spacing)));
	const int64_t last      = static_cast<int64_t>(std::ceil((end + SAMPLE_MARGIN_METERS) / spacing));
	job.job.firstSample     = first;
	route.CopySamples(first, last - first + 1, job.job.samples);
	const int rows = static_cast<int>(std::lround(m_settings.chunkLength / m_settings.rowSpacing)) + 1;
	job.job.rowBlends.resize(static_cast<size_t>(rows));
	for (int row = 0; row < rows; ++row)
	{
		job.job.rowBlends[static_cast<size_t>(row)] = sequence.BlendAt(start + row * m_settings.rowSpacing);
	}
	m_pendingIndices.push_back(chunkIndex);

	if (!m_threaded)
	{
		BuildChunk(job.job, *job.mesh);
		m_finished.push_back(std::move(job));
		return;
	}
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_jobs.push_back(std::move(job));
	}
	m_wake.notify_one();
}

void ChunkStreamer::CollectFinished()
{
	std::vector<Job> finished;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		finished.swap(m_finished);
	}
	for (Job& job : finished)
	{
		const int64_t index = job.job.chunkIndex;
		m_pendingIndices.erase(std::remove(m_pendingIndices.begin(), m_pendingIndices.end(), index),
		                       m_pendingIndices.end());
		if (job.generation != m_generation)
		{
			ReleaseMesh(job.mesh);
			continue;
		}
		m_ready.push_back(job.mesh);
		m_newlyReady.push_back(job.mesh);
		const double milliseconds = job.mesh->buildMilliseconds;
		m_stats.lastBuildMilliseconds = milliseconds;
		m_stats.averageBuildMilliseconds =
		    m_stats.chunksBuilt == 0 ? milliseconds : m_stats.averageBuildMilliseconds * 0.9 + milliseconds * 0.1;
		m_stats.maxBuildMilliseconds = std::max(m_stats.maxBuildMilliseconds, milliseconds);
		++m_stats.chunksBuilt;
	}
}

void ChunkStreamer::Update(Route& route, BiomeSequence& sequence, double riderDistance, double behindMeters,
                           double aheadMeters)
{
	CollectFinished();
	const double length    = m_settings.chunkLength;
	const int64_t first    = static_cast<int64_t>(std::floor(std::max(0.0, riderDistance - behindMeters) / length));
	const int64_t last     = static_cast<int64_t>(std::floor((riderDistance + aheadMeters) / length));

	// Recycle chunks outside the window.
	for (size_t index = 0; index < m_ready.size();)
	{
		ChunkMesh* mesh = m_ready[index];
		if (mesh->chunkIndex < first || mesh->chunkIndex > last)
		{
			m_released.push_back(mesh->chunkIndex);
			m_newlyReady.erase(std::remove(m_newlyReady.begin(), m_newlyReady.end(), mesh), m_newlyReady.end());
			ReleaseMesh(mesh);
			m_ready.erase(m_ready.begin() + static_cast<ptrdiff_t>(index));
		}
		else
		{
			++index;
		}
	}

	// Request missing chunks, nearest to the rider first.
	int requested = 0;
	for (int64_t offset = 0; offset <= last - first && requested < MAX_REQUESTS_PER_UPDATE; ++offset)
	{
		const int64_t riderChunk = static_cast<int64_t>(std::floor(riderDistance / length));
		const int64_t candidates[2] = {riderChunk + offset, riderChunk - offset};
		for (int64_t candidate : candidates)
		{
			if (candidate < first || candidate > last || IsPendingOrReady(candidate) || requested >= MAX_REQUESTS_PER_UPDATE)
			{
				continue;
			}
			RequestChunk(route, sequence, candidate);
			++requested;
		}
	}
	if (!m_threaded)
	{
		CollectFinished();
	}
	std::sort(m_ready.begin(), m_ready.end(), ByChunkIndex);
	m_stats.readyChunks  = static_cast<int>(m_ready.size());
	m_stats.pendingJobs  = static_cast<int>(m_pendingIndices.size());
	m_stats.pooledMeshes = static_cast<int>(m_allMeshes.size());
}

const std::vector<ChunkMesh*>& ChunkStreamer::GetReady() const
{
	return m_ready;
}

std::vector<const ChunkMesh*> ChunkStreamer::TakeNewlyReady()
{
	std::vector<const ChunkMesh*> result;
	result.swap(m_newlyReady);
	return result;
}

std::vector<int64_t> ChunkStreamer::TakeReleased()
{
	std::vector<int64_t> result;
	result.swap(m_released);
	return result;
}

StreamerStats ChunkStreamer::GetStats() const
{
	return m_stats;
}

void ChunkStreamer::WaitIdle()
{
	if (!m_threaded)
	{
		return;
	}
	std::unique_lock<std::mutex> lock(m_mutex);
	while (!m_jobs.empty() || m_busy)
	{
		m_idle.wait(lock);
	}
}

void ChunkStreamer::WorkerLoop()
{
	for (;;)
	{
		Job job;
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			while (!m_stop && m_jobs.empty())
			{
				m_wake.wait(lock);
			}
			if (m_stop)
			{
				return;
			}
			job = std::move(m_jobs.front());
			m_jobs.pop_front();
			m_busy = true;
		}
		BuildChunk(job.job, *job.mesh);
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_finished.push_back(std::move(job));
			m_busy = false;
		}
		m_idle.notify_all();
	}
}
}  // namespace Struktur::RideWorld
