#pragma once

#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "Debug/Assertions.h"

namespace Struktur
{
namespace Resource
{
// Sparse-set backed storage: T lives packed/contiguous in a dense array (cache-friendly to iterate - see
// begin()/end()), while a separate sparse array maps a stable Handle to that element's current position in the
// dense array. Erase() swaps the removed element with the last dense element and pops the back ("swap-and-pop"),
// so the dense array never needs to shift on removal, at the cost of not preserving iteration/insertion order.
// Same technique as EnTT's component pools (entt::basic_sparse_set/storage).
//
// Handles stay valid (and detectably invalid once erased) across other elements' insert/erase - a Handle pairs a
// slot index with a generation counter that increments every time that slot is freed, so a stale Handle referring
// to a since-recycled slot is caught by IsValid()/Resolve() rather than silently resolving to unrelated data.
//
// Erase() relies on move-assigning T (to relocate the last dense element into the erased slot's gap), so T must
// be at least move-assignable. Emplace() constructs T in place and doesn't itself require T to be movable or
// copyable.
template <typename T>
class SparseSet
{
   public:
	struct Handle
	{
		static constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();

		uint32_t index      = kInvalidIndex;
		uint32_t generation = 0;

		bool IsValid() const
		{
			return index != kInvalidIndex;
		}
	};

	template <typename... Args>
	Handle Emplace(Args&&... args)
	{
		uint32_t slot;
		if (!m_freeSlots.empty())
		{
			slot = m_freeSlots.back();
			m_freeSlots.pop_back();
		}
		else
		{
			slot = (uint32_t)m_sparse.size();
			m_sparse.push_back(SparseEntry{});
		}

		uint32_t denseIndex = (uint32_t)m_dense.size();
		m_dense.emplace_back(std::forward<Args>(args)...);
		m_denseToSlot.push_back(slot);

		m_sparse[slot].denseIndex = denseIndex;

		return Handle{slot, m_sparse[slot].generation};
	}

	bool Erase(Handle handle)
	{
		if (!IsValid(handle))
		{
			return false;
		}

		uint32_t slot        = handle.index;
		uint32_t denseIndex  = m_sparse[slot].denseIndex;
		uint32_t lastIndex   = (uint32_t)m_dense.size() - 1;

		if (denseIndex != lastIndex)
		{
			// Relocate the last element into the gap left by the erased one, then fix up the sparse entry for
			// whichever slot that last element belongs to, since its dense position just changed.
			m_dense[denseIndex]       = std::move(m_dense[lastIndex]);
			m_denseToSlot[denseIndex] = m_denseToSlot[lastIndex];
			m_sparse[m_denseToSlot[denseIndex]].denseIndex = denseIndex;
		}
		m_dense.pop_back();
		m_denseToSlot.pop_back();

		// Bump the generation so any Handle still referring to this slot is now detectably stale, even after the
		// slot gets recycled by a later Emplace().
		m_sparse[slot].generation++;
		m_sparse[slot].denseIndex = Handle::kInvalidIndex;
		m_freeSlots.push_back(slot);
		return true;
	}

	bool IsValid(Handle handle) const
	{
		return handle.IsValid() && handle.index < m_sparse.size() &&
		       m_sparse[handle.index].generation == handle.generation &&
		       m_sparse[handle.index].denseIndex != Handle::kInvalidIndex;
	}

	T* Resolve(Handle handle)
	{
		if (!IsValid(handle))
		{
			return nullptr;
		}
		return &m_dense[m_sparse[handle.index].denseIndex];
	}
	const T* Resolve(Handle handle) const
	{
		if (!IsValid(handle))
		{
			return nullptr;
		}
		return &m_dense[m_sparse[handle.index].denseIndex];
	}

	// Unchecked access for callers that already know handle is valid - asserts rather than silently
	// dereferencing a stale slot (see Debug/Assertions.h's ASSERT_MSG - logs FATAL always, DebugBreak()s in
	// Debug builds only).
	T& operator[](Handle handle)
	{
		ASSERT_MSG(IsValid(handle), "SparseSet: handle (index %u, generation %u) is not valid", handle.index,
		          handle.generation);
		return m_dense[m_sparse[handle.index].denseIndex];
	}
	const T& operator[](Handle handle) const
	{
		ASSERT_MSG(IsValid(handle), "SparseSet: handle (index %u, generation %u) is not valid", handle.index,
		          handle.generation);
		return m_dense[m_sparse[handle.index].denseIndex];
	}

	void Clear()
	{
		// Bump every still-occupied slot's generation (same as Erase()) before wiping the dense storage, so a
		// Handle still held elsewhere (e.g. a ResourcePtr a Wren finalizer hasn't gotten around to releasing yet)
		// is permanently and correctly detected as stale by IsValid()/Resolve() afterward. Previously this reset
		// m_sparse to empty, so the very next Emplace() into a recycled slot started back at generation 0 -
		// indistinguishable from a stale pre-Clear Handle for that same slot, which could then resolve into
		// (or alias) a completely different resource, or index into m_dense after it had shrunk back to nothing.
		for (size_t slot = 0; slot < m_sparse.size(); ++slot)
		{
			if (m_sparse[slot].denseIndex != Handle::kInvalidIndex)
			{
				m_sparse[slot].generation++;
				m_sparse[slot].denseIndex = Handle::kInvalidIndex;
			}
		}

		m_dense.clear();
		m_denseToSlot.clear();
		m_freeSlots.clear();
		for (uint32_t slot = 0; slot < (uint32_t)m_sparse.size(); ++slot)
		{
			m_freeSlots.push_back(slot);
		}
	}

	void Reserve(size_t capacity)
	{
		m_dense.reserve(capacity);
		m_denseToSlot.reserve(capacity);
	}

	size_t Size() const
	{
		return m_dense.size();
	}
	bool Empty() const
	{
		return m_dense.empty();
	}

	// Direct iteration over the packed dense array - this is the whole point of the structure, a plain
	// contiguous scan rather than following pointers/hashing through m_sparse.
	typename std::vector<T>::iterator begin()
	{
		return m_dense.begin();
	}
	typename std::vector<T>::iterator end()
	{
		return m_dense.end();
	}
	typename std::vector<T>::const_iterator begin() const
	{
		return m_dense.begin();
	}
	typename std::vector<T>::const_iterator end() const
	{
		return m_dense.end();
	}

   private:
	struct SparseEntry
	{
		uint32_t denseIndex = Handle::kInvalidIndex;
		uint32_t generation = 0;
	};

	std::vector<T> m_dense;
	// Parallel to m_dense - m_denseToSlot[i] is the sparse slot that owns m_dense[i], so Erase()'s swap can find
	// and fix up the sparse entry for whichever element got relocated.
	std::vector<uint32_t> m_denseToSlot;

	// Indexed by Handle::index ("slot"). Never shrinks - freed slots are recycled via m_freeSlots instead, so a
	// slot index stays meaningful (as a stale-but-checkable Handle) for the SparseSet's whole lifetime.
	std::vector<SparseEntry> m_sparse;
	std::vector<uint32_t> m_freeSlots;
};
}  // namespace Resource
}  // namespace Struktur
