#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "Engine/Jobs/Task.h"
#include "Engine/World/Biome.h"
#include "Engine/World/ChunkBuilder.h"
#include "Engine/World/Route.h"

namespace Struktur::World
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

// Streams chunks along the road: builds the ones in [rider - behind, rider + ahead] as background tasks on the job
// system (or inline when `jobs` is null, e.g. tests), hands finished ones to the renderer, and recycles the ones that
// fall behind into a fixed pool - meshes are reused, so memory stays flat over any distance. Every call is made from
// one thread (the main thread); only BuildChunk runs on workers, writing to a mesh nothing else touches meanwhile.
class ChunkStreamer
{
public:
	explicit ChunkStreamer(Jobs::JobSystem* jobs);
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
	// Blocks until no build is in flight (tests, screenshots), helping run them meanwhile.
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
	void FinishJob(Job& job);
	bool IsPendingOrReady(int64_t chunkIndex) const;

	// A build running as a task. The Job lives on the heap so its address stays put while a worker builds into it.
	// `claimed` is set by whichever comes first: the build starting, or Configure() taking the mesh back from a build
	// that hasn't started (so stale queued builds don't hold meshes until a worker gets round to them).
	struct InFlight
	{
		std::unique_ptr<Job> job;
		std::unique_ptr<std::atomic<bool>> claimed;
		Jobs::Task<void> task;
	};
	void ReclaimUnstarted();

	Jobs::JobSystem* m_jobs;
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

	std::vector<InFlight> m_inFlight;

	StreamerStats m_stats;
};
}  // namespace Struktur::World
