#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "Engine/RideWorld/Biome.h"
#include "Engine/RideWorld/ChunkBuilder.h"
#include "Engine/RideWorld/Route.h"

namespace Struktur::RideWorld
{
struct StreamerStats
{
	double lastBuildMilliseconds    = 0.0;
	double averageBuildMilliseconds = 0.0;  // exponential average
	double maxBuildMilliseconds     = 0.0;
	int pendingJobs                 = 0;
	int readyChunks                 = 0;
	int pooledMeshes                = 0;  // every mesh ever allocated (ready + pending + free)
	int64_t chunksBuilt             = 0;
};

// Streams chunks along the road: builds the ones in [rider - behind, rider + ahead] on a worker thread (or inline
// when threaded = false, e.g. tests and single-threaded builds), hands finished ones to the renderer, and recycles
// the ones that fall behind into a fixed pool - meshes are reused, so memory stays flat over any distance.
class ChunkStreamer
{
public:
	explicit ChunkStreamer(bool threaded);
	~ChunkStreamer();

	ChunkStreamer(const ChunkStreamer&)            = delete;
	ChunkStreamer& operator=(const ChunkStreamer&) = delete;

	// New settings/biomes: every chunk is rebuilt (released ones are reported through TakeReleased).
	void Configure(const ChunkSettings& settings, BiomeList biomes, std::shared_ptr<const PropTypeTable> propTypes);
	const ChunkSettings& GetSettings() const;

	void Update(Route& route, BiomeSequence& sequence, double riderDistance, double behindMeters, double aheadMeters);

	// Ready chunks, nearest first. Pointers stay valid until the chunk is released.
	const std::vector<ChunkMesh*>& GetReady() const;
	// Chunks that became ready / were released since the last call (renderer uploads / destroys GPU buffers).
	std::vector<const ChunkMesh*> TakeNewlyReady();
	std::vector<int64_t> TakeReleased();

	StreamerStats GetStats() const;
	// Blocks until no job is pending (tests, screenshots).
	void WaitIdle();

private:
	struct Job
	{
		ChunkJob job;
		ChunkMesh* mesh   = nullptr;
		uint32_t generation = 0;
	};

	ChunkMesh* AcquireMesh();
	void ReleaseMesh(ChunkMesh* mesh);
	void RequestChunk(Route& route, BiomeSequence& sequence, int64_t chunkIndex);
	void CollectFinished();
	void WorkerLoop();
	bool IsPendingOrReady(int64_t chunkIndex) const;

	bool m_threaded;
	ChunkSettings m_settings;
	BiomeList m_biomes;
	std::shared_ptr<const PropTypeTable> m_propTypes;
	uint32_t m_generation;

	std::vector<std::unique_ptr<ChunkMesh>> m_allMeshes;
	std::vector<ChunkMesh*> m_freeMeshes;
	std::vector<ChunkMesh*> m_ready;
	std::vector<int64_t> m_pendingIndices;
	std::vector<const ChunkMesh*> m_newlyReady;
	std::vector<int64_t> m_released;

	// Worker side.
	mutable std::mutex m_mutex;
	std::condition_variable m_wake;
	std::condition_variable m_idle;
	std::deque<Job> m_jobs;
	std::vector<Job> m_finished;
	bool m_busy;
	bool m_stop;
	std::thread m_worker;

	StreamerStats m_stats;
};
}  // namespace Struktur::RideWorld
