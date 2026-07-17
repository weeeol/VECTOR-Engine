#pragma once

#include <vector>
#include <algorithm>
#include <cstdint>
#include <glm/glm.hpp>

namespace VECTOR {

    class Mesh;
    class Material;

    /**
     * @struct RenderCommand
     * @brief A deferred draw call. Collected during scene traversal, sorted and flushed later.
     */
    struct RenderCommand {
        uint64_t sortKey = 0;
        const Mesh* mesh = nullptr;
        const Material* material = nullptr;
        glm::mat4 modelMatrix = glm::mat4(1.0f);
    };

    /**
     * @class RenderQueue
     * @brief Collects draw commands, sorts them by sort key, and flushes them in order.
     *
     * Sorting by (shader << 32 | texture) minimizes expensive GL state changes.
     * The renderer submits commands during scene traversal and calls Flush() once per pass.
     */
    class RenderQueue {
    public:
        RenderQueue() = default;
        ~RenderQueue() = default;

        /**
         * @brief Submit a render command to the queue.
         */
        void Submit(const RenderCommand& cmd);

        /**
         * @brief Sort all commands by sort key, then flush (execute) them.
         * 
         * The onBind callback is called whenever the material changes between commands.
         * The onDraw callback is called for each command to set per-object uniforms and draw.
         * 
         * After flushing, the queue is cleared.
         */
        void SortAndFlush();
        void SortAndFlushGBuffer(class Shader* gBufferShader);

        /**
         * @brief Clear all queued commands without executing them.
         */
        void Clear();

        /**
         * @brief Get the number of draw calls that were issued in the last flush.
         */
        uint32_t GetLastDrawCallCount() const { return m_LastDrawCallCount; }

        /**
         * @brief Get the current number of queued commands.
         */
        size_t GetPendingCount() const { return m_Commands.size(); }

    private:
        std::vector<RenderCommand> m_Commands;
        uint32_t m_LastDrawCallCount = 0;
    };

} // namespace VECTOR
