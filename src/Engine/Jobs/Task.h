#pragma once

#include "Engine/Jobs/JobSystem.h"

#include <cassert>
#include <optional>

// Tasks: jobs you can ask about from the update loop.
//
//   Task<Image> load = jobs.Run([path](TaskContext& context) { return DecodeImage(path, context); });
//   ...
//   if (load.IsComplete()) { Upload(load.GetResult()); }               // poll each frame, or:
//   load.ThenOnMainThread([](Task<Image>& done) { if (done.Succeeded()) Upload(done.TakeResult()); });
//
//  - Work is T() or T(TaskContext&). Through the context it reports progress (0-1) and checks for cancellation.
//  - Cancel() asks the task to stop. A task that hasn't started yet never runs its work; a running one stops when it
//    next checks IsCancelled(). Either way it ends as CANCELLED, with no result.
//  - Work that returns StepResult is stepped: it is called again after each CONTINUE, with other work (and every
//    higher-lane job) getting a turn in between. With zero workers each step is one job for RunPending() /
//    RunMainThreadWork(), so the per-frame budget applies between steps. Stepped tasks are Task<void>.
//  - ThenOnMainThread() queues a follow-up for the main thread, run by JobSystem::RunMainThreadWork() once the task
//    has completed, cancelled included. Follow-ups of a task dropped by Shutdown() never run.
//  - Copies of a Task share the same state. Tasks are meant for coarse work (loads, builds, streaming); each one
//    allocates a small shared state, unlike plain Schedule().
namespace Struktur::Jobs
{
enum class TaskStatus : uint8_t
{
	PENDING,    // queued, or waiting for dependencies
	RUNNING,    // started (a stepped task stays RUNNING between steps)
	SUCCEEDED,  // finished; the result is available
	CANCELLED,  // stopped by Cancel(), no result
	DROPPED,    // never finished because the job system shut down, no result
};

inline const char* ToString(TaskStatus status)
{
	switch (status)
	{
		case TaskStatus::RUNNING:
			return "running";
		case TaskStatus::SUCCEEDED:
			return "succeeded";
		case TaskStatus::CANCELLED:
			return "cancelled";
		case TaskStatus::DROPPED:
			return "dropped";
		default:
			return "pending";
	}
}

// The state every copy of a Task shares. The engine's one deliberate shared ownership in the job system: the running
// work, every copy of the handle, queued follow-ups and a script's garbage-collected Task can each be the last user.
struct TaskStateBase
{
	explicit TaskStateBase(JobSystem* owner) : system(owner) {}

	JobSystem* system;
	std::atomic<TaskStatus> status{TaskStatus::PENDING};
	std::atomic<bool> cancelRequested{false};
	std::atomic<float> progress{0.0f};
	std::mutex mutex;  // guards followUps and the move to a final status
	std::vector<std::unique_ptr<JobFunction>> followUps;

	bool IsFinal(TaskStatus value) const { return value >= TaskStatus::SUCCEEDED; }

	void Complete(TaskStatus final)
	{
		std::vector<std::unique_ptr<JobFunction>> discarded;
		std::lock_guard<std::mutex> lock(mutex);
		if (final == TaskStatus::SUCCEEDED)
		{
			progress.store(1.0f, std::memory_order_relaxed);
		}
		if (final == TaskStatus::DROPPED || followUps.empty())
		{
			// Shutting down: the follow-ups are discarded (and with them their references to this state).
			discarded.swap(followUps);
			status.store(final, std::memory_order_release);
			return;
		}
		// Queue the follow-ups and publish the status together, under the main-thread queue's lock: whoever sees the
		// task complete also finds its follow-ups queued, and a follow-up never runs before the status is final.
		std::lock_guard<std::mutex> queueLock(system->m_mainThreadMutex);
		for (std::unique_ptr<JobFunction>& followUp : followUps)
		{
			system->m_mainThreadQueue.push_back(std::move(followUp));
		}
		followUps.clear();
		status.store(final, std::memory_order_release);
	}

	void AddFollowUp(std::unique_ptr<JobFunction> followUp)
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			const TaskStatus current = status.load(std::memory_order_acquire);
			if (!IsFinal(current))
			{
				followUps.push_back(std::move(followUp));
				return;
			}
			if (current == TaskStatus::DROPPED)
			{
				return;
			}
		}
		system->PostToMainThread(std::move(followUp));  // already complete: it runs at the next RunMainThreadWork()
	}
};

template <typename T>
struct TaskState : TaskStateBase
{
	using TaskStateBase::TaskStateBase;
	std::optional<T> result;
};

template <>
struct TaskState<void> : TaskStateBase
{
	using TaskStateBase::TaskStateBase;
};

// What a task's work sees: progress reporting and the cancellation flag.
class TaskContext
{
public:
	explicit TaskContext(TaskStateBase& state) : m_state(state) {}

	bool IsCancelled() const { return m_state.cancelRequested.load(std::memory_order_acquire); }
	void SetProgress(float progress) { m_state.progress.store(std::clamp(progress, 0.0f, 1.0f), std::memory_order_relaxed); }

private:
	TaskStateBase& m_state;
};

template <typename T>
class Task
{
public:
	Task() = default;

	bool IsValid() const { return m_state != nullptr; }
	// An invalid task reports DROPPED (and so counts as complete), like an invalid JobHandle.
	TaskStatus GetStatus() const { return m_state ? m_state->status.load(std::memory_order_acquire) : TaskStatus::DROPPED; }
	// True once the task has finished, been cancelled or been dropped.
	bool IsComplete() const { return GetStatus() >= TaskStatus::SUCCEEDED; }
	bool Succeeded() const { return GetStatus() == TaskStatus::SUCCEEDED; }
	bool WasCancelled() const { return GetStatus() == TaskStatus::CANCELLED; }
	bool WasDropped() const { return GetStatus() == TaskStatus::DROPPED; }
	// 0-1 as reported by the work; 1 once it succeeded.
	float GetProgress() const { return m_state ? m_state->progress.load(std::memory_order_relaxed) : 0.0f; }

	void Cancel() const
	{
		if (m_state)
		{
			m_state->cancelRequested.store(true, std::memory_order_release);
		}
	}

	// The underlying job, for dependencies and JobSystem::Wait(). It completes a moment after the task's status
	// turns final, so poll the task itself rather than the handle.
	JobHandle GetHandle() const { return m_handle; }

	// The result, only after the task succeeded.
	template <typename Value = T>
	    requires(!std::is_void_v<Value>)
	const Value* TryGetResult() const
	{
		return Succeeded() ? &*static_cast<TaskState<T>*>(m_state.get())->result : nullptr;
	}
	template <typename Value = T>
	    requires(!std::is_void_v<Value>)
	const Value& GetResult() const
	{
		assert(Succeeded());
		return *static_cast<TaskState<T>*>(m_state.get())->result;
	}
	// Moves the result out (every copy of this task sees the moved-from value afterwards).
	template <typename Value = T>
	    requires(!std::is_void_v<Value>)
	Value TakeResult()
	{
		assert(Succeeded());
		return std::move(*static_cast<TaskState<T>*>(m_state.get())->result);
	}

	// Runs followUp(Task<T>&) on the main thread, inside JobSystem::RunMainThreadWork(), once the task is complete.
	template <typename FollowUp>
	void ThenOnMainThread(FollowUp&& followUp) const
	{
		if (!m_state)
		{
			return;
		}
		std::unique_ptr<JobFunction> work = std::make_unique<JobFunction>();
		work->Set([task = *this, callback = std::forward<FollowUp>(followUp)]() mutable { callback(task); });
		m_state->AddFollowUp(std::move(work));
	}

private:
	friend class JobSystem;
	Task(std::shared_ptr<TaskStateBase> state, JobHandle handle) : m_state(std::move(state)), m_handle(handle) {}

	std::shared_ptr<TaskStateBase> m_state;
	JobHandle m_handle;
};

// Calls work(context) or work(), whichever it takes.
template <typename Work>
decltype(auto) InvokeTaskWork(Work& work, TaskContext& context)
{
	if constexpr (std::is_invocable_v<Work&, TaskContext&>)
	{
		return work(context);
	}
	else
	{
		return work();
	}
}

template <typename Work>
using TaskWorkResult = std::decay_t<decltype(InvokeTaskWork(std::declval<Work&>(), std::declval<TaskContext&>()))>;

template <typename Work>
constexpr bool IS_STEPPED_TASK = std::is_same_v<TaskWorkResult<Work>, StepResult>;

template <typename Work>
using TaskValue = std::conditional_t<IS_STEPPED_TASK<Work>, void, TaskWorkResult<Work>>;

// The job a task schedules. If it is destroyed without finishing (dropped at shutdown), the task reports DROPPED.
template <typename T, typename Work>
class TaskRunner
{
public:
	TaskRunner(std::shared_ptr<TaskState<T>> state, Work&& work) : m_state(std::move(state)), m_work(std::move(work)) {}
	TaskRunner(std::shared_ptr<TaskState<T>> state, const Work& work) : m_state(std::move(state)), m_work(work) {}
	TaskRunner(TaskRunner&& other) noexcept
	    : m_state(std::move(other.m_state)),
	      m_work(std::move(other.m_work)),
	      m_finished(other.m_finished)
	{
	}
	TaskRunner(const TaskRunner&)            = delete;
	TaskRunner& operator=(const TaskRunner&) = delete;
	TaskRunner& operator=(TaskRunner&&)      = delete;

	~TaskRunner()
	{
		if (m_state && !m_finished)
		{
			m_state->Complete(TaskStatus::DROPPED);
		}
	}

	auto operator()()
	{
		if constexpr (IS_STEPPED_TASK<Work>)
		{
			if (m_state->cancelRequested.load(std::memory_order_acquire))
			{
				Finish(TaskStatus::CANCELLED);
				return StepResult::DONE;
			}
			m_state->status.store(TaskStatus::RUNNING, std::memory_order_release);
			TaskContext context(*m_state);
			if (InvokeTaskWork(m_work, context) == StepResult::CONTINUE &&
			    !m_state->cancelRequested.load(std::memory_order_acquire))
			{
				return StepResult::CONTINUE;
			}
			Finish(m_state->cancelRequested.load(std::memory_order_acquire) ? TaskStatus::CANCELLED : TaskStatus::SUCCEEDED);
			return StepResult::DONE;
		}
		else
		{
			if (m_state->cancelRequested.load(std::memory_order_acquire))
			{
				Finish(TaskStatus::CANCELLED);
				return;
			}
			m_state->status.store(TaskStatus::RUNNING, std::memory_order_release);
			TaskContext context(*m_state);
			if constexpr (std::is_void_v<T>)
			{
				InvokeTaskWork(m_work, context);
			}
			else
			{
				m_state->result.emplace(InvokeTaskWork(m_work, context));
			}
			if (m_state->cancelRequested.load(std::memory_order_acquire))
			{
				if constexpr (!std::is_void_v<T>)
				{
					m_state->result.reset();
				}
				Finish(TaskStatus::CANCELLED);
				return;
			}
			Finish(TaskStatus::SUCCEEDED);
		}
	}

private:
	void Finish(TaskStatus status)
	{
		m_finished = true;
		m_state->Complete(status);
	}

	std::shared_ptr<TaskState<T>> m_state;
	Work m_work;
	bool m_finished = false;
};

template <typename Work>
auto JobSystem::RunWith(Work&& work, Lane lane, const JobHandle* dependencies, size_t count)
{
	using WorkType = std::decay_t<Work>;
	using Value    = TaskValue<WorkType>;
	std::shared_ptr<TaskState<Value>> state = std::make_shared<TaskState<Value>>(this);
	const JobHandle handle = ScheduleWith(TaskRunner<Value, WorkType>(state, std::forward<Work>(work)), lane, dependencies, count);
	return Task<Value>(std::move(state), handle);
}

template <typename Work>
auto JobSystem::Run(Work&& work, Lane lane)
{
	return RunWith(std::forward<Work>(work), lane, nullptr, 0);
}

template <typename Work>
auto JobSystem::Run(Work&& work, Lane lane, const JobHandle& dependency)
{
	return RunWith(std::forward<Work>(work), lane, &dependency, 1);
}

template <typename Work>
auto JobSystem::Run(Work&& work, Lane lane, const std::vector<JobHandle>& dependencies)
{
	return RunWith(std::forward<Work>(work), lane, dependencies.data(), dependencies.size());
}
}  // namespace Struktur::Jobs
