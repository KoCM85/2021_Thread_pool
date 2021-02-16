#ifndef THREAD_POOL_H

#define THREAD_POOL_H

#include <thread>
#include <future>
#include <functional>
#include <queue>
#include <atomic>
#include <shared_mutex>
#include <mutex>
#include <random>
#include <utility>
#include <array>
#include <string>

namespace my {
    /* this function writes exceptions to the file 
        error_message_in - exception about which message will be writen to the file
        file_name_in - at that file will be writen info about errors
    */
    void write_error(const std::exception& error_message_in, const std::string& file_name_in = "errors");
}

namespace my {
    // this is Singleton
    // this class randomly return task or number
    class random_data {
        const short min_task = 1;       // min random value for tasks
        const short max_task = 5;       // max random value for tasks
        const unsigned min_val = 1;     // min random value for number
        const unsigned max_val = 5000;  // max random value for number

        // this 4 data used for get random value
        std::random_device rand_dev;
        std::mt19937 mt;
        std::uniform_int_distribution<short> distribution_task;
        std::uniform_int_distribution<unsigned> distribution_val;

    private:
        random_data();

        ~random_data() = default;
    public:
        random_data(const random_data&) = delete;
        random_data(random_data&& obj) = delete;

        random_data& operator= (const random_data&) = delete;
        random_data& operator=(random_data&& obj) = delete;

    public:
        /* creates instance random_data
            returns reference at the instance random_data
        */
        static random_data& create_instance();

        /* calculates random task
            returns random task
        */
        std::function<void()> get_task();

        /* calculates random number
            returns random number
        */
        unsigned get_val() {
            return distribution_val(mt);
        }
    };

    // used to save parameters about thread
    struct thread_param {
        std::atomic<bool> thread_wait;
        std::promise<void> wake_thread;
    };

    // class used to store and manage threads that will perform tasks
    template<size_t number_threads>
    class thread_pool {
        std::array<std::pair<thread_param, std::thread>, number_threads> threads; // array with threads and threads parameter
        short unsigned current_thread; // currently free thread that be used to perform task
        short unsigned attempt_get_thread; // attempts to get free thread

    public:
        thread_pool();

        thread_pool(const thread_pool&) = delete;
        thread_pool(thread_pool&& obj) = delete;

        thread_pool& operator= (const thread_pool&) = delete;
        thread_pool& operator=(thread_pool&& obj) = delete;

    public:
        /* calculates size of the array(threads)
            returns size of the array(threads)
        */
        const size_t& size() const {
            return threads.size();
        }

        /* calculates free thread
            returns index to the free thread
        */
        const short unsigned& free_thread();

        /* calculates data at index
            returns data stored on the array
        */
        std::pair<thread_param, std::thread>& operator[](const size_t index) {
            return threads[index];
        }

    private:
        // calculates next free thread
        void next();
    };


    // class used for controlling threads (that will perform tasks) and queue of the tasks (stores tasks that will performs at the threads)
    template<class type_fucn, size_t number_threads>
    class controller {
        using func_t = std::function<type_fucn>;

        std::queue<func_t> tasks; // queue store tasks
        thread_pool<number_threads> threads_exec; // store and manage threads that will perform tasks
        std::shared_mutex sh_mut; // shared mutex controlling access to the data

    public:
        controller() = default;

        controller(const controller&) = delete;
        controller(controller&& obj) = delete;

        controller& operator= (const controller&) = delete;
        controller& operator=(controller&& obj) = delete;

    public:
        // push task to the queue
        void push(func_t&& func_obj);

        // emplace task to the queue
        void emplace(type_fucn&& func_obj);

        /* calculates size of the queue(tasks)
            returns size of the queue(tasks)
        */
        size_t size();

        /* initializes threads_exec, called only once
           in loop loads task from queue(tasks) and executes
           before thread perform task, he waits until function(listen) wakes up him
        */
        void init_threads();

        /* in loop listen queue(tasks), called only once
           gets free thread and wakes up that thread to perform task
        */
        void listen();
    };
}


// defines for class thread_pool
namespace my {

    template<size_t number_threads>
    thread_pool<number_threads>::thread_pool() : current_thread(0), attempt_get_thread(0) {
        for (auto&& value : threads) {
            value.first.thread_wait.store(true, std::memory_order::memory_order_relaxed);
        }
    }

    template<size_t number_threads>
    const short unsigned& thread_pool<number_threads>::free_thread() {
        short unsigned current_thread_temp;

        while (!threads[current_thread].first.thread_wait.load(std::memory_order::memory_order_relaxed)) {
            next();
            ++attempt_get_thread;

            if (attempt_get_thread * 2 > threads.size())
                std::this_thread::yield();
        }

        attempt_get_thread = 0;
        current_thread_temp = current_thread;
        next();

        return current_thread_temp;
    }


    template<size_t number_threads>
    void thread_pool<number_threads>::next() {
        if (current_thread < threads.size() - 1) {
            ++current_thread;
        }
        else {
            current_thread = 0;
        }
    }
}

// defines for class controller
namespace my {
    template<class type_fucn, size_t number_threads>
    void controller<type_fucn, number_threads>::push(func_t&& func_obj) {
        std::lock_guard<std::shared_mutex> lock(sh_mut);
        tasks.push(func_obj);
    }

    template<class type_fucn, size_t number_threads>
    void controller<type_fucn, number_threads>::emplace(type_fucn&& func_obj) {
        std::lock_guard<std::shared_mutex> lock(sh_mut);
        tasks.emplace(func_obj);
    }

    template<class type_fucn, size_t number_threads>
    size_t controller<type_fucn, number_threads>::size() {
        std::shared_lock<std::shared_mutex> lock(sh_mut);
        return tasks.size();
    }

    template<class type_fucn, size_t number_threads>
    void controller<type_fucn, number_threads>::init_threads() try {
        auto lambda = [this](const size_t index) {
            func_t func;
            std::unique_lock<std::shared_mutex> lock(sh_mut, std::defer_lock);

            while (true) {
                threads_exec[index].first.wake_thread.get_future().wait();

                lock.lock();
                func = tasks.front();
                tasks.pop();
                lock.unlock();

                func();

                threads_exec[index].first.wake_thread = std::promise<void>();
                std::atomic_thread_fence(std::memory_order::memory_order_release);
                threads_exec[index].first.thread_wait.store(true, std::memory_order::memory_order_relaxed);
            }
        };


        for (size_t index = 0; index < threads_exec.size(); ++index) {
            threads_exec[index].second = std::thread(lambda, index);
            threads_exec[index].second.detach();
        }
    }
    catch (const std::exception& except) {
        write_error(except);
    }

    template<class type_fucn, size_t number_threads>
    void controller<type_fucn, number_threads>::listen() try {
        while (true) {
            if (size()) {
                short unsigned thread_index = threads_exec.free_thread();

                threads_exec[thread_index].first.thread_wait.store(false, std::memory_order::memory_order_relaxed);
                std::atomic_thread_fence(std::memory_order::memory_order_release);
                threads_exec[thread_index].first.wake_thread.set_value();
            }
            else {
                std::this_thread::yield();
            }
        }
    }
    catch (const std::exception& except) {
        write_error(except);
    }
}

#endif // THREAD_POOL_H