#pragma once

#include <chrono>
#include <deque>
#include <string>
#include <vector>

namespace Struktur
{
namespace Debug
{
struct ProfileNode
{
	std::string name;
	long long duration_us;
	long long self_duration_us;
	long long accumulated_duration_us;  // Total across all calls
	size_t call_count;
	size_t depth;
	ProfileNode* parent;
	std::vector<ProfileNode*> children;
	std::chrono::time_point<std::chrono::high_resolution_clock> start;

	ProfileNode(const std::string& n, size_t d, ProfileNode* p = nullptr)
	    : name(n),
	      duration_us(0),
	      self_duration_us(0),
	      accumulated_duration_us(0),
	      call_count(0),
	      depth(d),
	      parent(p)
	{
	}
};

class Profiler
{
public:
	Profiler()
	    : m_rootNode("Frame", 0),
	      m_pausedRootNode("Frame", 0),
	      m_currentNode(&m_rootNode),
	      m_maxFrameHistory(120),
	      m_isPaused(false),
	      m_targetFrameTimeMs(16.67f)
	{
	}
	~Profiler();

	void BeginFrame();
	void EndFrame();
	void BeginProfile(const std::string& name);
	void EndProfile();

	const ProfileNode* GetRootNode() const
	{
		return m_isPaused ? &m_pausedRootNode : &m_rootNode;
	}
	const std::deque<float>& GetFrameTimeHistory() const
	{
		return m_frameTimeHistory;
	}

	void SetPaused(bool paused);
	bool IsPaused() const
	{
		return m_isPaused;
	}

	float GetTargetFrameTimeMs() const
	{
		return m_targetFrameTimeMs;
	}
	void SetTargetFrameTimeMs(float ms)
	{
		m_targetFrameTimeMs = ms;
	}

private:
	ProfileNode m_rootNode;
	ProfileNode m_pausedRootNode;
	ProfileNode* m_currentNode;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_frameStart;
	std::deque<float> m_frameTimeHistory;
	size_t m_maxFrameHistory;
	bool m_isPaused;
	float m_targetFrameTimeMs;

	// Node pool for reuse
	std::vector<ProfileNode*> m_nodePool;
	size_t m_nodePoolIndex;

	ProfileNode* AllocateNode(const std::string& name, size_t depth, ProfileNode* parent);
	void ResetNodePool();
	void CalculateSelfDuration(ProfileNode* node);
	ProfileNode* CopyTree(const ProfileNode* source, ProfileNode* parent = nullptr);
	void DeleteTree(ProfileNode* node);
};

class ScopedProfile
{
public:
	ScopedProfile(Profiler* profiler, const std::string& name)
	    : m_profiler(profiler)
	{
		if (!profiler->IsPaused())
		{
			profiler->BeginProfile(name);
			m_active = true;
		}
		else
		{
			m_active = false;
		}
	}

	void EndProfile()
	{
		if (m_active)
		{
			m_profiler->EndProfile();
			m_active = false;
		}
	}

	~ScopedProfile()
	{
		EndProfile();
	}

private:
	Profiler* m_profiler;
	bool m_active;
};
}  // namespace Debug
}  // namespace Struktur

// Macros
#ifdef DEBUG
	#define PROFILE_FUNCTION() \
		Struktur::Debug::ScopedProfile profiler_##__LINE__(&(context.GetProfiler()), __FUNCTION__)
	#define PROFILE_SCOPE(name) Struktur::Debug::ScopedProfile profiler_##__LINE__(&(context.GetProfiler()), name)
	#define PROFILE_BEGIN_SCOPE(variable, name) \
		Struktur::Debug::ScopedProfile profiler_##variable(&(context.GetProfiler()), name);
	#define PROFILE_END_SCOPE(variable) profiler_##variable.EndProfile()
	#define PROFILE_BEGIN_FRAME()       context.GetProfiler().BeginFrame()
	#define PROFILE_END_FRAME()         context.GetProfiler().EndFrame()
#else
	#define PROFILE_FUNCTION()
	#define PROFILE_SCOPE(name)
	#define PROFILE_BEGIN_SCOPE(variable, name)
	#define PROFILE_END_SCOPE(variable)
	#define PROFILE_BEGIN_FRAME()
	#define PROFILE_END_FRAME()
#endif
