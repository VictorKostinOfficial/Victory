#pragma once

#include "set"

enum class QueueIndex {
    eGraphics,
    ePresent,
    eCompute,
    eTransfer
};

struct QueueIndexes {
    uint32_t m_GraphicsQueueIndex{UINT32_MAX};
    uint32_t m_PresentQueueIndex{UINT32_MAX};
    uint32_t m_ComputeQueueIndex{UINT32_MAX};
    uint32_t m_TransferQueueIndex{UINT32_MAX};

    uint32_t GetQueueIndex(QueueIndex queueIndex_) {
        switch (queueIndex_) {
        case QueueIndex::eGraphics:
            return m_GraphicsQueueIndex;
        case QueueIndex::ePresent:
            return m_PresentQueueIndex;
        case QueueIndex::eCompute:
            return m_ComputeQueueIndex;
        case QueueIndex::eTransfer:
            return m_TransferQueueIndex;
        default:
            return UINT32_MAX;
        }
    }

    std::set<uint32_t> GetUniqueIndexes() {
        return {m_GraphicsQueueIndex, m_PresentQueueIndex, m_ComputeQueueIndex, m_TransferQueueIndex};
    }
};