// Procedural ride world: generator determinism, grade and curvature limits, terrain self-intersection, chunk recycling,
// cadence -> animation frame maths, manifest/placeholder resolution, biome and sprite data.
#include <doctest.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "Engine/Ride/RideSettings.h"
#include "Engine/Ride/TrainerController.h"
#include "Engine/RideWorld/AssetManifest.h"
#include "Engine/RideWorld/Biome.h"
#include "Engine/RideWorld/ChunkBuilder.h"
#include "Engine/Jobs/JobSystem.h"
#include "Engine/RideWorld/ChunkStreamer.h"
#include "Engine/RideWorld/ElevationProfile.h"
#include "Engine/RideWorld/Noise.h"
#include "Engine/RideWorld/Placeholder.h"
#include "Engine/RideWorld/RideWorld.h"
#include "Engine/RideWorld/RiderAnimation.h"
#include "Engine/RideWorld/Route.h"
#include "Engine/RideWorld/SpriteDescriptor.h"
#include "Engine/RideWorld/WorldSettings.h"

using namespace Struktur::RideWorld;

static std::string ReadTextFile(const std::filesystem::path& path)
{
	std::ifstream stream(path, std::ios::binary);
	std::stringstream buffer;
	buffer << stream.rdbuf();
	return buffer.str();
}

static std::filesystem::path AssetsDir()
{
	return std::filesystem::path(STRUKTUR_ASSETS_DIR);
}

static BiomeList LoadShippedBiomes()
{
	auto biomes = std::make_shared<std::vector<BiomeDefinition>>();
	std::vector<std::filesystem::path> files;
	for (const auto& entry : std::filesystem::directory_iterator(AssetsDir() / "World" / "Biomes"))
	{
		if (entry.path().extension() == ".json")
		{
			files.push_back(entry.path());
		}
	}
	std::sort(files.begin(), files.end());
	for (const std::filesystem::path& file : files)
	{
		BiomeDefinition biome;
		std::string error;
		REQUIRE_MESSAGE(ParseBiome(ReadTextFile(file), biome, error), file.string() << ": " << error);
		biomes->push_back(biome);
	}
	return biomes;
}

static bool ExistsInAssets(const std::string& path)
{
	return std::filesystem::exists(AssetsDir() / path);
}

// --- noise --------------------------------------------------------------------------------------------------------

TEST_CASE("world noise is deterministic and bounded")
{
	CHECK(Hash(42, 1, 2, 3) == Hash(42, 1, 2, 3));
	CHECK(Hash(42, 1, 2, 3) != Hash(43, 1, 2, 3));
	for (int step = 0; step < 2000; ++step)
	{
		const double x = step * 0.173;
		const double value = Fbm2D(7, x, x * 0.5, 4);
		CHECK(value >= -1.0);
		CHECK(value <= 1.0);
		CHECK(Fbm1D(9, x, 3) == Fbm1D(9, x, 3));
	}
	Random first(5);
	Random second(5);
	for (int step = 0; step < 100; ++step)
	{
		CHECK(first.NextU32() == second.NextU32());
	}
}

// --- route ---------------------------------------------------------------------------------------------------------

TEST_CASE("route generation is deterministic per seed")
{
	RouteSettings settings;
	settings.seed = 1234;
	Route first(settings);
	Route second(settings);
	first.EnsureGenerated(5000.0);
	second.EnsureGenerated(5000.0);
	for (double distance = 0.0; distance < 5000.0; distance += 37.0)
	{
		const RouteSample fromFirst  = first.SampleAt(distance);
		const RouteSample fromSecond = second.SampleAt(distance);
		CHECK(fromFirst.x == fromSecond.x);
		CHECK(fromFirst.z == fromSecond.z);
		CHECK(fromFirst.curvature == fromSecond.curvature);
	}
	settings.seed = 999;
	Route other(settings);
	other.EnsureGenerated(5000.0);
	CHECK(std::abs(other.SampleAt(4000.0).z - first.SampleAt(4000.0).z) > 1.0);
}

TEST_CASE("route keeps arc-length spacing, the curvature limit and its heading over 100 km")
{
	for (uint64_t seed : {1ull, 77ull, 4242ull})
	{
		RouteSettings settings;
		settings.seed                 = seed;
		settings.minCurveRadiusMeters = 90.0;
		Route route(settings);
		double maxCurvature = 0.0;
		double minForward   = 1.0;
		double tightest     = 0.0;
		for (double distance = 0.0; distance < 100000.0; distance += 2000.0)
		{
			route.EnsureGenerated(distance + 2000.0);
			for (int64_t index = std::max<int64_t>(route.GetFirstIndex() + 4, static_cast<int64_t>(distance));
			     index < static_cast<int64_t>(distance + 2000.0); ++index)
			{
				const RouteSample& sample = route.GetSample(index);
				const RouteSample& next   = route.GetSample(index + 1);
				maxCurvature              = std::max(maxCurvature, std::abs(sample.curvature));
				minForward                = std::min(minForward, sample.tangentX);
				// Arc-length resampling: consecutive samples are one spacing apart (chord ~ arc at this radius).
				const double chord = std::hypot(next.x - sample.x, next.z - sample.z);
				CHECK(chord == doctest::Approx(1.0).epsilon(0.002));
			}
			tightest = std::max(tightest, maxCurvature);
			route.PruneBefore(distance + 1500.0);
		}
		INFO("seed " << seed << " max curvature " << maxCurvature << " (limit " << 1.0 / 90.0 << ")");
		CHECK(maxCurvature <= 1.0 / 90.0);
		// The route uses most of the allowed curvature (tight curves exist).
		CHECK(tightest > 0.5 / 90.0);
		// Heading within 75 degrees of the mean direction: never turns back on itself.
		CHECK(minForward >= std::cos(75.0 * 3.14159265358979 / 180.0) - 0.02);
		// Pruning keeps memory bounded.
		CHECK(route.GetSampleCount() < 4000);
	}
}

// --- elevation -----------------------------------------------------------------------------------------------------

TEST_CASE("elevation profile limits grade and grade change, and history never moves")
{
	ElevationLimits limits;
	limits.maxGradePercent        = 12.0;
	limits.maxGradeChangePerMeter = 0.05;
	ElevationProfile profile(limits);
	std::vector<double> targets(1200, 20.0);  // asks for 20 %: must be capped at 12
	double rider = 0.0;
	double previousHeightAt100 = 0.0;
	for (int frame = 0; frame < 600; ++frame)
	{
		profile.Update(rider, targets, 200.0);
		if (frame == 300)
		{
			previousHeightAt100 = profile.HeightAt(100.0);
		}
		if (frame > 300 && rider > 150.0)
		{
			CHECK(profile.HeightAt(100.0) == doctest::Approx(previousHeightAt100));
		}
		rider += 0.5;
	}
	double previousGrade = profile.GradeAt(profile.GetStart());
	for (double distance = profile.GetStart() + 1.0; distance < profile.GetEnd(); distance += 1.0)
	{
		const double grade = profile.GradeAt(distance);
		CHECK(std::abs(grade) <= 12.0 + 1e-9);
		CHECK(std::abs(grade - previousGrade) <= 0.05 + 1e-9);
		CHECK((profile.HeightAt(distance) - profile.HeightAt(distance - 1.0)) * 100.0 == doctest::Approx(previousGrade).epsilon(0.01));
		previousGrade = grade;
	}
	CHECK(profile.GradeAt(rider + 400.0) == doctest::Approx(12.0));
}

TEST_CASE("re-deriving the profile with unchanged targets reproduces the road ahead")
{
	ElevationProfile profile;
	std::vector<double> targets(800);
	for (size_t index = 0; index < targets.size(); ++index)
	{
		targets[index] = 5.0;
	}
	profile.Update(10.0, targets, 100.0);
	const double before = profile.HeightAt(300.0);
	std::vector<double> shifted(targets.begin() + 3, targets.end());
	profile.Update(13.0, shifted, 100.0);
	CHECK(profile.HeightAt(300.0) == doctest::Approx(before).epsilon(1e-6));
}

// --- biomes --------------------------------------------------------------------------------------------------------

TEST_CASE("shipped biomes parse and the sequence is seeded and blends smoothly")
{
	const BiomeList biomes = LoadShippedBiomes();
	REQUIRE(biomes->size() >= 5);
	for (const BiomeDefinition& biome : *biomes)
	{
		CHECK_FALSE(biome.palette.empty());
		CHECK_FALSE(biome.props.empty());
	}
	BiomeSequence first(biomes, 11, "auto");
	BiomeSequence second(biomes, 11, "auto");
	int changes      = 0;
	int previousFrom = first.BlendAt(0.0).from;
	for (double distance = 0.0; distance < 60000.0; distance += 25.0)
	{
		const BiomeBlend fromFirst  = first.BlendAt(distance);
		const BiomeBlend fromSecond = second.BlendAt(distance);
		CHECK(fromFirst.from == fromSecond.from);
		CHECK(fromFirst.weight == fromSecond.weight);
		CHECK(fromFirst.weight >= 0.0f);
		CHECK(fromFirst.weight <= 1.0f);
		if (fromFirst.from != previousFrom)
		{
			++changes;
			previousFrom = fromFirst.from;
		}
	}
	CHECK(changes >= 6);
	BiomeSequence forced(biomes, 11, "night");
	for (double distance = 0.0; distance < 30000.0; distance += 500.0)
	{
		CHECK((*biomes)[static_cast<size_t>(forced.BlendAt(distance).from)].id == "night");
	}
}

TEST_CASE("biome parser rejects junk and clamps values")
{
	BiomeDefinition biome;
	std::string error;
	CHECK_FALSE(ParseBiome("not json", biome, error));
	CHECK_FALSE(ParseBiome("{\"name\":\"no id\"}", biome, error));
	REQUIRE(ParseBiome("{\"id\":\"x\",\"fog\":{\"density\":5},\"terrain\":{\"amplitude\":-4},\"props\":[{\"role\":\"rock\",\"density\":1e9}]}",
	                   biome, error));
	CHECK(biome.fogDensity <= 0.05);
	CHECK(biome.terrainAmplitude >= 0.0);
	CHECK(biome.props[0].density <= 200.0);
}

// --- terrain -------------------------------------------------------------------------------------------------------

static double SignedArea(const std::vector<float>& grid, const size_t corners[4])
{
	float xs[4];
	float zs[4];
	for (int corner = 0; corner < 4; ++corner)
	{
		xs[corner] = grid[corners[corner] * 2];
		zs[corner] = grid[corners[corner] * 2 + 1];
	}
	double area       = 0.0;
	for (int corner = 0; corner < 4; ++corner)
	{
		const int next = (corner + 1) % 4;
		area += static_cast<double>(xs[corner]) * zs[next] - static_cast<double>(xs[next]) * zs[corner];
	}
	return area * 0.5;
}

TEST_CASE("terrain never folds over itself on the inside of bends over 100 km")
{
	const BiomeList biomes = LoadShippedBiomes();
	WorldSettings settings;
	settings.seed        = 3;
	settings.flatShading = false;
	settings.quality     = Quality::MEDIUM;
	RideWorld world(settings, biomes, nullptr);
	RideWorldInputs inputs;
	inputs.deltaSeconds         = 0.25;
	inputs.speedMetersPerSecond = 64.0 / 0.25 / 4.0;  // one chunk every 4 updates
	int checkedChunks   = 0;
	int flippedQuads    = 0;
	double worstArea    = 0.0;
	int64_t lastChecked = -1;
	while (world.GetDistance() < 100000.0)
	{
		world.Update(inputs);
		for (const ChunkMesh* chunk : world.GetChunks())
		{
			if (chunk->chunkIndex <= lastChecked)
			{
				continue;
			}
			lastChecked = chunk->chunkIndex;
			++checkedChunks;
			const int rows    = chunk->terrainRows;
			const int columns = chunk->terrainColumns;
			for (int row = 0; row + 1 < rows; ++row)
			{
				for (int column = 0; column + 1 < columns; ++column)
				{
					const size_t corners[4] = {static_cast<size_t>(row * columns + column),
					                           static_cast<size_t>((row + 1) * columns + column),
					                           static_cast<size_t>((row + 1) * columns + column + 1),
					                           static_cast<size_t>(row * columns + column + 1)};
					// Rows run along the road, columns towards the left: a non-folded quad has negative area in (x, z)
					// with this ordering (or ~0 when columns collapse at the inner limit).
					const double area = SignedArea(chunk->gridXZ, corners);
					if (area > 1e-3)
					{
						++flippedQuads;
						worstArea = std::max(worstArea, area);
					}
				}
			}
		}
	}
	INFO("checked " << checkedChunks << " chunks, worst flipped area " << worstArea);
	CHECK(checkedChunks > 1500);
	CHECK(flippedQuads == 0);
}

TEST_CASE("chunk recycling keeps the pool and buffers flat")
{
	const BiomeList biomes = LoadShippedBiomes();
	WorldSettings settings;
	settings.seed = 8;
	RideWorld world(settings, biomes, nullptr);
	RideWorldInputs inputs;
	inputs.deltaSeconds         = 0.1;
	inputs.speedMetersPerSecond = 40.0;
	int poolAfterWarmup         = -1;
	size_t capacityAfterWarmup  = 0;
	while (world.GetDistance() < 30000.0)
	{
		world.Update(inputs);
		world.GetStreamer().TakeNewlyReady();
		world.GetStreamer().TakeReleased();
		const StreamerStats stats = world.GetStreamer().GetStats();
		size_t capacity = 0;
		for (const ChunkMesh* chunk : world.GetChunks())
		{
			capacity += chunk->terrain.capacity() + chunk->road.capacity() + chunk->water.capacity();
		}
		if (world.GetDistance() > 3000.0 && poolAfterWarmup < 0)
		{
			poolAfterWarmup     = stats.pooledMeshes;
			capacityAfterWarmup = capacity;
		}
		if (poolAfterWarmup >= 0)
		{
			CHECK(stats.pooledMeshes <= poolAfterWarmup);
			// Reused vectors: never more than the warm-up total plus one chunk's worth of slack per mesh.
			CHECK(capacity <= capacityAfterWarmup * 2 + 1);
		}
	}
	CHECK(poolAfterWarmup > 0);
	CHECK(world.GetStreamer().GetStats().chunksBuilt > 400);
}

TEST_CASE("job-system and inline chunk builds produce the same meshes")
{
	const BiomeList biomes = LoadShippedBiomes();
	WorldSettings settings;
	settings.seed = 21;
	// 4 workers, and 0 (jobs queue up and run when the world waits for them - the threads-off path).
	for (int workers : {4, 0})
	{
	CAPTURE(workers);
	Struktur::Jobs::JobSystem jobs(Struktur::Jobs::JobSystemConfig{workers});
	RideWorld inlineWorld(settings, biomes, nullptr);
	RideWorld threadedWorld(settings, biomes, &jobs);
	RideWorldInputs inputs;
	inputs.deltaSeconds         = 0.1;
	inputs.speedMetersPerSecond = 10.0;
	for (int frame = 0; frame < 20; ++frame)
	{
		inlineWorld.Update(inputs);
		threadedWorld.Update(inputs);
	}
	inlineWorld.WaitForChunks();
	threadedWorld.WaitForChunks();
	const ChunkMesh* inlineChunk   = nullptr;
	const ChunkMesh* threadedChunk = nullptr;
	for (const ChunkMesh* chunk : inlineWorld.GetChunks())
	{
		if (chunk->chunkIndex == 3)
		{
			inlineChunk = chunk;
		}
	}
	for (const ChunkMesh* chunk : threadedWorld.GetChunks())
	{
		if (chunk->chunkIndex == 3)
		{
			threadedChunk = chunk;
		}
	}
	REQUIRE(inlineChunk != nullptr);
	REQUIRE(threadedChunk != nullptr);
	REQUIRE(inlineChunk->terrain.size() == threadedChunk->terrain.size());
	for (size_t index = 0; index < inlineChunk->terrain.size(); index += 17)
	{
		CHECK(inlineChunk->terrain[index].x == threadedChunk->terrain[index].x);
		CHECK(inlineChunk->terrain[index].y == threadedChunk->terrain[index].y);
		CHECK(inlineChunk->terrain[index].abgr == threadedChunk->terrain[index].abgr);
	}
	size_t propsInline   = 0;
	size_t propsThreaded = 0;
	for (const auto& list : inlineChunk->props)
	{
		propsInline += list.size();
	}
	for (const auto& list : threadedChunk->props)
	{
		propsThreaded += list.size();
	}
	CHECK(propsInline == propsThreaded);
	}
}

TEST_CASE("changing chunk settings while builds are in flight recycles them safely")
{
	const BiomeList biomes = LoadShippedBiomes();
	Struktur::Jobs::JobSystem jobs(Struktur::Jobs::JobSystemConfig{4});
	WorldSettings settings;
	settings.seed = 5;
	RideWorld world(settings, biomes, &jobs);
	RideWorldInputs inputs;
	inputs.deltaSeconds         = 0.1;
	inputs.speedMetersPerSecond = 30.0;
	for (int frame = 0; frame < 300; ++frame)
	{
		world.Update(inputs);
		if (frame % 7 == 3)
		{
			// Flip a setting that rebuilds every chunk, mid-stream: queued builds are cancelled, running ones finish
			// and come back stale.
			settings.flatShading = !settings.flatShading;
			world.SetSettings(settings);
		}
	}
	world.WaitForChunks();
	const StreamerStats stats = world.GetStreamer().GetStats();
	CHECK(stats.pendingJobs == 0);
	CHECK(stats.readyChunks > 0);
	for (const ChunkMesh* chunk : world.GetChunks())
	{
		CHECK(chunk->chunkIndex >= 0);
		CHECK_FALSE(chunk->terrain.empty());
	}
	// Meshes are recycled, not leaked: the pool stays around what the window needs plus what was in flight.
	CHECK(stats.pooledMeshes < 64);
}

TEST_CASE("a world destroyed with builds in flight waits for them")
{
	const BiomeList biomes = LoadShippedBiomes();
	Struktur::Jobs::JobSystem jobs(Struktur::Jobs::JobSystemConfig{2});
	for (int round = 0; round < 20; ++round)
	{
		WorldSettings settings;
		settings.seed = static_cast<uint32_t>(round);
		RideWorld world(settings, biomes, &jobs);
		RideWorldInputs inputs;
		inputs.deltaSeconds         = 0.1;
		inputs.speedMetersPerSecond = 30.0;
		world.Update(inputs);  // requests the first chunks, then goes out of scope immediately
	}
	CHECK(jobs.GetQueuedCount() == 0);
}

// --- grade behaviour -----------------------------------------------------------------------------------------------

TEST_CASE("SIM manual grade: the road blends towards the set grade and the rider's grade follows")
{
	const BiomeList biomes = LoadShippedBiomes();
	WorldSettings settings;
	RideWorld world(settings, biomes, nullptr);
	RideWorldInputs inputs;
	inputs.deltaSeconds         = 0.1;
	inputs.speedMetersPerSecond = 8.0;
	inputs.manualGradePercent   = 6.0;
	double previous             = world.GetRoadGradePercent();
	for (int frame = 0; frame < 400; ++frame)
	{
		world.Update(inputs);
		const double grade = world.GetRoadGradePercent();
		// Smooth: never more than the change limit over the distance ridden this frame (+ interpolation slack).
		CHECK(std::abs(grade - previous) <= settings.maxGradeChangePerMeter * 0.8 + 0.02);
		previous = grade;
	}
	CHECK(world.GetRoadGradePercent() == doctest::Approx(6.0).epsilon(0.01));

	inputs.manualGradePercent = 18.0;  // beyond the world's limit
	for (int frame = 0; frame < 600; ++frame)
	{
		world.Update(inputs);
	}
	CHECK(world.GetRoadGradePercent() == doctest::Approx(settings.maxGradePercent).epsilon(0.01));
}

TEST_CASE("ERG grade is cosmetic and follows workout intensity")
{
	CHECK(GradeForIntensity(0.5) < 0.0);
	CHECK(GradeForIntensity(0.65) == doctest::Approx(0.0));
	CHECK(GradeForIntensity(1.05) > 5.0);
	CHECK(GradeForIntensity(3.0) == doctest::Approx(9.0));
	CHECK(GradeForIntensity(std::nan("")) == 0.0);

	const BiomeList biomes = LoadShippedBiomes();
	WorldSettings settings;
	RideWorld world(settings, biomes, nullptr);
	RideWorldInputs inputs;
	inputs.deltaSeconds         = 0.1;
	inputs.speedMetersPerSecond = 8.0;
	inputs.ergMode              = true;
	inputs.manualGradePercent   = -8.0;  // ignored in ERG
	inputs.intensityAhead.assign(600, 1.05);
	for (int frame = 0; frame < 600; ++frame)
	{
		world.Update(inputs);
	}
	CHECK(world.GetRoadGradePercent() == doctest::Approx(GradeForIntensity(1.05)).epsilon(0.02));
}

TEST_CASE("trainer controller simulates the world's road grade in SIM")
{
	using namespace Struktur::Ride;
	TrainerController controller;
	controller.SetGradePercent(2.0);
	controller.SetRoadGrade(5.5);
	ControlInputs inputs;
	for (int step = 0; step < 50; ++step)
	{
		inputs.now = step * 0.2;
		controller.Update(inputs, nullptr);
	}
	CHECK(controller.GetEffectiveGradePercent() == doctest::Approx(5.5));
	CHECK(controller.GetGradePercent() == doctest::Approx(2.0));
	controller.SetRoadGrade(std::nullopt);
	for (int step = 50; step < 100; ++step)
	{
		inputs.now = step * 0.2;
		controller.Update(inputs, nullptr);
	}
	CHECK(controller.GetEffectiveGradePercent() == doctest::Approx(2.0));
}

TEST_CASE("same seed, same world: camera and grade match between two worlds")
{
	const BiomeList biomes = LoadShippedBiomes();
	WorldSettings settings;
	settings.seed        = 99;
	settings.gradeSource = GradeSource::ROUTE;
	RideWorld first(settings, biomes, nullptr);
	RideWorld second(settings, biomes, nullptr);
	RideWorldInputs inputs;
	inputs.deltaSeconds         = 1.0 / 60.0;
	inputs.speedMetersPerSecond = 9.0;
	for (int frame = 0; frame < 3000; ++frame)
	{
		first.Update(inputs);
		second.Update(inputs);
	}
	CHECK(first.GetCamera().x == second.GetCamera().x);
	CHECK(first.GetCamera().z == second.GetCamera().z);
	CHECK(first.GetRoadGradePercent() == second.GetRoadGradePercent());
	CHECK(std::abs(first.GetRoadGradePercent()) <= settings.maxGradePercent + 1e-9);
}

// --- rider animation -----------------------------------------------------------------------------------------------

TEST_CASE("cadence to crank frame maths")
{
	CHECK(CrankFrame(0.0, 12) == 0);
	CHECK(CrankFrame(0.0833, 12) == 0);
	CHECK(CrankFrame(0.0834, 12) == 1);
	CHECK(CrankFrame(0.5, 12) == 6);
	CHECK(CrankFrame(0.999, 12) == 11);
	CHECK(CrankFrame(1.0, 12) == 0);
	CHECK(CrankFrame(3.25, 8) == 2);
	CHECK(CrankFrame(-0.25, 8) == 6);
	CHECK(MinimumFramesPerRevolution(45.0) == 8);
	CHECK(MinimumFramesPerRevolution(30.0) == 12);
	// 8 m/s on a 0.34 m wheel = 3.74 rev/s: a 6-spoke wheel at 60 fps advances 0.37 gaps/frame (fine), at 16 m/s 0.75
	// (aliases), a 32-spoke wheel aliases well before that.
	CHECK_FALSE(WheelAliases(3.74, 6, 60.0));
	CHECK(WheelAliases(7.5, 6, 60.0));
	CHECK(WheelAliases(3.74, 32, 60.0));
}

TEST_CASE("rider animation: phase follows smoothed cadence, zero freezes, missing sensor idles")
{
	RiderAnimation animation;
	const double frame = 1.0 / 60.0;
	for (int step = 0; step < 600; ++step)  // 10 s at 90 rpm: smoothing has settled
	{
		animation.Update(frame, 90.0, 8.0, 0.8, 0.0);
	}
	CHECK(animation.GetSmoothedCadence() == doctest::Approx(90.0).epsilon(0.001));
	CHECK(animation.GetPose() == RiderPose::PEDALLING);
	const double before = animation.GetCrankPhase();
	for (int step = 0; step < 60; ++step)  // one more second: 1.5 revolutions
	{
		animation.Update(frame, 90.0, 8.0, 0.8, 0.0);
	}
	double advanced = animation.GetCrankPhase() - before;
	advanced -= std::floor(advanced);
	CHECK(advanced == doctest::Approx(0.5).epsilon(0.01));

	// Jittery input is smoothed: alternating 80/100 rpm stays near 90.
	for (int step = 0; step < 120; ++step)
	{
		animation.Update(frame, step % 2 == 0 ? 80.0 : 100.0, 8.0, 0.8, 0.0);
	}
	CHECK(animation.GetSmoothedCadence() == doctest::Approx(90.0).epsilon(0.02));

	animation.Update(frame, 0.0, 8.0, 0.0, 0.0);
	CHECK(animation.GetPose() == RiderPose::COASTING);
	const double frozen = animation.GetCrankPhase();
	for (int step = 0; step < 30; ++step)
	{
		animation.Update(frame, 0.0, 8.0, 0.0, 0.0);
	}
	CHECK(animation.GetCrankPhase() == frozen);
	CHECK(animation.GetFrame(12, 3) == 3);

	animation.Update(frame, std::nullopt, 8.0, std::nullopt, 0.0);
	CHECK(animation.GetPose() == RiderPose::IDLE);

	// Out of the saddle above 125 % FTP, back down below 110 %.
	animation.Update(frame, 80.0, 6.0, 1.4, 0.0);
	CHECK(animation.GetPose() == RiderPose::OUT_OF_SADDLE);
	animation.Update(frame, 80.0, 6.0, 1.2, 0.0);
	CHECK(animation.GetPose() == RiderPose::OUT_OF_SADDLE);
	animation.Update(frame, 80.0, 6.0, 1.0, 0.0);
	CHECK(animation.GetPose() == RiderPose::PEDALLING);

	// Wheel: angular speed = speed / radius.
	RiderAnimation wheel;
	wheel.Update(0.1, 90.0, 6.8, 0.8, 0.0);
	CHECK(wheel.GetWheelAngle() == doctest::Approx(6.8 / 0.34 * 0.1).epsilon(0.001));
	// Lean into a left bend.
	for (int step = 0; step < 200; ++step)
	{
		wheel.Update(frame, 90.0, 12.0, 0.8, 1.0 / 90.0);
	}
	CHECK(wheel.GetLeanDegrees() > 5.0);
}

// --- manifest, placeholders, sprite descriptor ---------------------------------------------------------------------

TEST_CASE("asset manifest parses, resolves real files and placeholders for every role")
{
	AssetManifest manifest;
	std::string error;
	std::vector<std::string> warnings;
	REQUIRE_MESSAGE(ParseManifest(ReadTextFile(AssetsDir() / "World" / "manifest.json"), manifest, error, warnings), error);
	CHECK(warnings.empty());
	const char* required[] = {"rider_legs", "rider_wheel", "rider_shadow", "tree_pine", "tree_broadleaf", "tree_palm",
	                          "bush", "rock", "grass", "sign_post", "guardrail", "lamp_post", "building",
	                          "finish_banner", "sky_clouds", "sky_sun", "sky_moon", "sky_stars", "mountains_far",
	                          "mountains_near", "water", "terrain_grass", "terrain_rock", "terrain_snow",
	                          "terrain_sand", "road_asphalt", "road_markings", "particle_rain", "particle_snow"};
	for (const char* role : required)
	{
		CHECK_MESSAGE(manifest.Find(role) != nullptr, role);
	}
	const std::vector<std::string>& shapes = KnownPlaceholderShapes();
	for (const AssetRole& role : manifest.roles)
	{
		CHECK_MESSAGE(std::find(shapes.begin(), shapes.end(), role.placeholderShape) != shapes.end(), role.role);
		const PlaceholderImage image = GeneratePlaceholder(role, 1);
		CHECK(image.width == role.width * role.frames);
		CHECK(image.height == role.height * role.rows);
		size_t opaque = 0;
		for (size_t pixel = 3; pixel < image.pixels.size(); pixel += 4)
		{
			opaque += image.pixels[pixel] > 0 ? 1 : 0;
		}
		CHECK_MESSAGE(opaque > 0, role.role << " placeholder is empty");
	}
	// The four roles mapped to existing assets resolve; the rest fall back to placeholders.
	const std::vector<ResolvedAsset> resolved = ResolveAssets(manifest, ExistsInAssets);
	int found = 0;
	for (const ResolvedAsset& asset : resolved)
	{
		CHECK(asset.placeholderName == "placeholder_" + asset.role->role);
		found += asset.found ? 1 : 0;
	}
	CHECK(found >= 7);  // shadow, cloud, rain, snow, two tree roles from the nature tileset, the Explorer
	const std::string markdown = FormatMissingAssetsMarkdown(resolved);
	CHECK(markdown.find("`rider_legs`") != std::string::npos);
	CHECK(markdown.find("`particle_rain`") == std::string::npos);
	CHECK(FormatMissingAssetsLines(resolved).size() == resolved.size() - static_cast<size_t>(found));
}

TEST_CASE("manifest regions cut frames out of a sheet, bottom-centred in equal cells")
{
	AssetManifest manifest;
	std::string error;
	std::vector<std::string> warnings;
	REQUIRE(ParseManifest("{\"roles\":[{\"role\":\"trees\",\"file\":\"sheet.png\",\"regions\":[[0,0,2,2],[2,0,4,3],[9,9,9]]}]}",
	                      manifest, error, warnings));
	REQUIRE(manifest.roles.size() == 1);
	const AssetRole& role = manifest.roles[0];
	CHECK(role.regions.size() == 2);
	CHECK(role.frames == 2);
	CHECK(warnings.size() == 1);  // the 3-number region
	// A 6x3 sheet: region 0 red, region 1 green.
	std::vector<uint8_t> sheet(6 * 3 * 4, 0);
	for (int y = 0; y < 3; ++y)
	{
		for (int x = 0; x < 6; ++x)
		{
			uint8_t* pixel = &sheet[(static_cast<size_t>(y) * 6 + x) * 4];
			pixel[x < 2 ? 0 : 1] = 255;
			pixel[3]             = 255;
		}
	}
	const PlaceholderImage strip = ComposeRegions(sheet.data(), 6, 3, role);
	CHECK(strip.width == 8);  // 2 cells of the widest region (4)
	CHECK(strip.height == 3);
	// Cell 0: the 2x2 red frame sits bottom-centre (x 1..2, y 1..2); the top row stays empty.
	CHECK(strip.pixels[(0 * 8 + 1) * 4 + 3] == 0);
	CHECK(strip.pixels[(1 * 8 + 1) * 4 + 0] == 255);
	CHECK(strip.pixels[(2 * 8 + 2) * 4 + 0] == 255);
	CHECK(strip.pixels[(2 * 8 + 0) * 4 + 3] == 0);
	// Cell 1: green fills x 4..7.
	CHECK(strip.pixels[(0 * 8 + 4) * 4 + 1] == 255);
	CHECK(strip.pixels[(2 * 8 + 7) * 4 + 1] == 255);
	// Real data: the shipped tree and Explorer regions lie inside their sheets.
	AssetManifest shipped;
	REQUIRE(ParseManifest(ReadTextFile(AssetsDir() / "World" / "manifest.json"), shipped, error, warnings));
	const AssetRole* trees    = shipped.Find("tree_broadleaf");
	const AssetRole* explorer = shipped.Find("rider_explorer");
	REQUIRE(trees != nullptr);
	REQUIRE(explorer != nullptr);
	CHECK(trees->frames == 3);
	CHECK(explorer->frames == 9);
	for (const std::array<int, 4>& region : trees->regions)
	{
		CHECK(region[0] + region[2] <= 384);  // TilesetNature.png is 384 x 336
		CHECK(region[1] + region[3] <= 336);
	}
	for (const std::array<int, 4>& region : explorer->regions)
	{
		CHECK(region[0] + region[2] <= 1380);  // explorer.png is 1380 x 1408
		CHECK(region[1] + region[3] <= 1408);
	}
}

static bool NothingExists(const std::string&)
{
	return false;
}

TEST_CASE("manifest rejects unsafe paths and duplicates")
{
	AssetManifest manifest;
	std::string error;
	std::vector<std::string> warnings;
	CHECK_FALSE(ParseManifest("[]", manifest, error, warnings));
	REQUIRE(ParseManifest("{\"roles\":[{\"role\":\"a\",\"file\":\"../../secret.png\"},{\"role\":\"a\",\"file\":\"x.png\"},"
	                      "{\"role\":\"b\",\"file\":\"C:/x.png\",\"placeholder\":{\"shape\":\"rock\"}}]}",
	                      manifest, error, warnings));
	REQUIRE(manifest.roles.size() == 2);
	CHECK(manifest.roles[0].file.empty());
	CHECK(manifest.roles[1].file.empty());
	CHECK(warnings.size() == 3);
	const std::vector<ResolvedAsset> resolved = ResolveAssets(manifest, NothingExists);
	CHECK_FALSE(resolved[0].found);
	// Unknown shape -> checker box, never empty.
	AssetRole odd;
	odd.placeholderShape = "nonsense";
	CHECK_FALSE(GeneratePlaceholder(odd, 1).pixels.empty());
}

TEST_CASE("rider sprite descriptor parses and matches the defaults")
{
	SpriteDescriptor descriptor;
	std::string error;
	REQUIRE_MESSAGE(ParseSpriteDescriptor(ReadTextFile(AssetsDir() / "World" / "Rider" / "rider.json"), descriptor, error), error);
	// The shipped rider: wheel + bike frame + the Explorer's run cycle drawn over them (8 frames per revolution, the
	// idle frame after them).
	REQUIRE(descriptor.layers.size() == 3);
	const SpriteLayer& explorer = descriptor.layers[2];
	CHECK(explorer.role == "rider_explorer");
	CHECK(explorer.motion == SpriteLayerMotion::CRANK);
	CHECK(explorer.framesPerRevolution == 8);
	CHECK(explorer.framesPerRevolution >= MinimumFramesPerRevolution(45.0));
	CHECK(explorer.idleColumn == 8);
	CHECK(explorer.rectX < 0);  // reaches outside the frame
	CHECK(explorer.rectY + explorer.rectHeight > descriptor.frameHeight);
	const SpriteDescriptor defaults = DefaultSpriteDescriptor();
	CHECK(defaults.layers.size() == 4);
	CHECK(defaults.layers[2].stillRow == 2);
	CHECK_FALSE(ParseSpriteDescriptor("{\"kind\":\"sheet\"}", descriptor, error));
	CHECK_FALSE(ParseSpriteDescriptor("{\"kind\":\"weird\",\"layers\":[{\"role\":\"x\"}]}", descriptor, error));
}

TEST_CASE("world settings round-trip through ride_settings.json")
{
	Struktur::Ride::RideSettings settings;
	settings.world.seed        = 777;
	settings.world.quality     = Quality::HIGH;
	settings.world.gradeSource = GradeSource::ROUTE;
	settings.world.comfortMode = true;
	settings.world.biome       = "alpine";
	std::string warning;
	const Struktur::Ride::RideSettings loaded = Struktur::Ride::RideSettings::FromJson(settings.ToJson(), warning);
	CHECK(warning.empty());
	CHECK(loaded.world.seed == 777);
	CHECK(loaded.world.quality == Quality::HIGH);
	CHECK(loaded.world.gradeSource == GradeSource::ROUTE);
	CHECK(loaded.world.comfortMode);
	CHECK(loaded.world.biome == "alpine");
	const Struktur::Ride::RideSettings bad =
	    Struktur::Ride::RideSettings::FromJson("{\"world\":{\"quality\":\"ultra\",\"cameraHeight\":9,\"seed\":-5}}", warning);
	CHECK(bad.world.cameraHeightMeters == doctest::Approx(3.0));
	CHECK(bad.world.quality == Quality::MEDIUM);
	CHECK_FALSE(warning.empty());
	CHECK(ParametersFor(Quality::LOW).drawDistanceMeters < ParametersFor(Quality::HIGH).drawDistanceMeters);
}
