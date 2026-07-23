#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <atomic>
#include <future>
#include <stdexcept>

namespace VECTOR {

    class JobSystem {
    public:
        // Singleton pattern
        static JobSystem& Get() {
            static JobSystem instance;
            return instance;
        }

        // Initialize the thread pool with a specific number of threads (0 = hardware concurrency - 1)
        void Initialize(uint32_t numThreads = 0);

        // Shutdown the thread pool and join all threads
        void Shutdown();

        // Submit a job to be executed asynchronously.
        // Returns a std::future to allow waiting for the result.
        template<typename F, typename... Args>
        auto Execute(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;

        // Wait until all current jobs are finished
        void Wait();

        JobSystem(const JobSystem&) = delete;
        JobSystem& operator=(const JobSystem&) = delete;

    private:
        JobSystem() = default;
        ~JobSystem();

        void WorkerLoop();

        std::vector<std::thread> m_Workers;
        std::queue<std::function<void()>> m_JobQueue;

        std::mutex m_QueueMutex;
        std::condition_variable m_Condition;
        std::condition_variable m_WaitCondition;

        std::atomic<bool> m_Stop{ false };
        std::atomic<uint32_t> m_ActiveJobs{ 0 };
    };

    // Template implementation
    template<typename F, typename... Args>
    auto JobSystem::Execute(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using ReturnType = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            if (m_Stop) {
                throw std::runtime_error("Execute on stopped JobSystem");
            }

            m_JobQueue.emplace([task]() {
                (*task)();
            });
            m_ActiveJobs++;
        }

        m_Condition.notify_one();
        return result;
    }

} // namespace VECTOR
