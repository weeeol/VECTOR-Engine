#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/Logger.hpp"

namespace VECTOR {

    JobSystem::~JobSystem() {
        if (!m_Stop) {
            Shutdown();
        }
    }

    void JobSystem::Initialize(uint32_t numThreads) {
        if (numThreads == 0) {
            // Leave 1 thread for the OS/Main thread
            uint32_t cores = std::thread::hardware_concurrency();
            numThreads = (cores > 1) ? (cores - 1) : 1;
        }

        m_Stop = false;
        m_ActiveJobs = 0;

        VECTOR_LOG_INFO(std::string("Initializing JobSystem with ") + std::to_string(numThreads) + " threads");

        for (uint32_t i = 0; i < numThreads; ++i) {
            m_Workers.emplace_back(&JobSystem::WorkerLoop, this);
        }
    }

    void JobSystem::Shutdown() {
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_Stop = true;
        }

        m_Condition.notify_all();

        for (std::thread& worker : m_Workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        
        m_Workers.clear();
        VECTOR_LOG_INFO("JobSystem shutdown successfully");
    }

    void JobSystem::Wait() {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_WaitCondition.wait(lock, [this]() {
            return m_ActiveJobs == 0 && m_JobQueue.empty();
        });
    }

    void JobSystem::WorkerLoop() {
        while (true) {
            std::function<void()> job;

            {
                std::unique_lock<std::mutex> lock(m_QueueMutex);
                m_Condition.wait(lock, [this]() {
                    return m_Stop || !m_JobQueue.empty();
                });

                if (m_Stop && m_JobQueue.empty()) {
                    return;
                }

                job = std::move(m_JobQueue.front());
                m_JobQueue.pop();
            }

            // Execute the job
            job();

            {
                std::unique_lock<std::mutex> lock(m_QueueMutex);
                m_ActiveJobs--;
                if (m_ActiveJobs == 0 && m_JobQueue.empty()) {
                    m_WaitCondition.notify_all();
                }
            }
        }
    }

} // namespace VECTOR
