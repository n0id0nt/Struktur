#include "Profiler.h"
#include <algorithm>

Struktur::Debug::Profiler::~Profiler()
{
    for (auto node : m_nodePool)
    {
        delete node;
    }
    DeleteTree(&m_pausedRootNode);
}

void Struktur::Debug::Profiler::BeginFrame()
{
    if (m_isPaused) return;

    m_frameStart = std::chrono::high_resolution_clock::now();
    m_currentNode = &m_rootNode;
    ResetNodePool();
}

void Struktur::Debug::Profiler::EndFrame()
{
    if (m_isPaused) return;

    auto frameEnd = std::chrono::high_resolution_clock::now();
    m_rootNode.duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
        frameEnd - m_frameStart).count();

    // Calculate self durations
    CalculateSelfDuration(&m_rootNode);

    // Store frame time in milliseconds
    float frameTimeMs = m_rootNode.duration_us / 1000.0f;
    m_frameTimeHistory.push_back(frameTimeMs);

    // Maintain history size
    if (m_frameTimeHistory.size() > m_maxFrameHistory) {
        m_frameTimeHistory.pop_front();
    }

    // Clear root for next frame
    m_rootNode.children.clear();
    m_rootNode.duration_us = 0;
    m_rootNode.self_duration_us = 0;
    m_rootNode.call_count = 0;
}

void Struktur::Debug::Profiler::SetPaused(bool paused)
{
    if (paused && !m_isPaused)
    {
        // Copy current state when pausing
        DeleteTree(&m_pausedRootNode);
        m_pausedRootNode = ProfileNode("Frame", 0);

        if (!m_rootNode.children.empty())
        {
            for (auto child : m_rootNode.children)
            {
                m_pausedRootNode.children.push_back(CopyTree(child, &m_pausedRootNode));
            }
            m_pausedRootNode.duration_us = m_rootNode.duration_us;
            m_pausedRootNode.self_duration_us = m_rootNode.self_duration_us;
            m_pausedRootNode.call_count = m_rootNode.call_count;
        }
    }
    m_isPaused = paused;
}

void Struktur::Debug::Profiler::BeginProfile(const std::string& name)
{
    if (m_isPaused) return;

    // Try to find existing child
    ProfileNode* childNode = nullptr;
    for (auto child : m_currentNode->children)
    {
        if (child->name == name)
        {
            childNode = child;
            childNode->call_count++;
            break;
        }
    }

    if (!childNode) 
    {
        childNode = AllocateNode(name, m_currentNode->depth + 1, m_currentNode);
        childNode->call_count = 1;
        m_currentNode->children.push_back(childNode);
    }

    childNode->start = std::chrono::high_resolution_clock::now();
    m_currentNode = childNode;
}

void Struktur::Debug::Profiler::EndProfile()
{
    if (m_isPaused) return;

    auto end = std::chrono::high_resolution_clock::now();
    m_currentNode->duration_us += std::chrono::duration_cast<std::chrono::microseconds>(
        end - m_currentNode->start).count();

    if (m_currentNode->parent) 
    {
        m_currentNode = m_currentNode->parent;
    }
}

Struktur::Debug::ProfileNode* Struktur::Debug::Profiler::AllocateNode(const std::string& name, size_t depth, ProfileNode* parent)
{
    ProfileNode* node;
    if (m_nodePoolIndex < m_nodePool.size())
    {
        node = m_nodePool[m_nodePoolIndex++];
        node->name = name;
        node->depth = depth;
        node->parent = parent;
        node->duration_us = 0;
        node->self_duration_us = 0;
        node->call_count = 0;
        node->children.clear();
    }
    else
    {
        node = new ProfileNode(name, depth, parent);
        m_nodePool.push_back(node);
        m_nodePoolIndex++;
    }
    return node;
}

void Struktur::Debug::Profiler::ResetNodePool()
{
    m_nodePoolIndex = 0;
}

Struktur::Debug::ProfileNode* Struktur::Debug::Profiler::CopyTree(const ProfileNode* source, ProfileNode* parent)
{
    ProfileNode* copy = new ProfileNode(source->name, source->depth, parent);
    copy->duration_us = source->duration_us;
    copy->self_duration_us = source->self_duration_us;
    copy->call_count = source->call_count;

    for (auto child : source->children)
    {
        copy->children.push_back(CopyTree(child, copy));
    }

    return copy;
}

void Struktur::Debug::Profiler::DeleteTree(ProfileNode* node)
{
    for (auto child : node->children)
    {
        DeleteTree(child);
    }
    if (node->parent != nullptr)
    {
        delete node;
    }
}

void Struktur::Debug::Profiler::CalculateSelfDuration(ProfileNode* node)
{
    long long childrenTotal = 0;
    for (auto child : node->children)
    {
        CalculateSelfDuration(child);
        childrenTotal += child->duration_us;
    }
    node->self_duration_us = node->duration_us - childrenTotal;
}