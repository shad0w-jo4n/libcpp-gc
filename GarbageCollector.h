#ifndef GC_GARBAGECOLLECTOR_H
#define GC_GARBAGECOLLECTOR_H

#include <condition_variable>
#include <chrono>
#include <memory>
#include <mutex>
#include <list>

#define gc(instance) GarbageCollector::getInstance()->alloc(instance)

using namespace std::chrono_literals;

/**
 * Garbage Collector
 */
class GarbageCollector {
private:
    std::mutex _mutex;
    std::list<std::shared_ptr<void>> _pointers;

    inline static std::mutex _cvMutex;
    inline static std::mutex _globalMutex;
    inline static std::condition_variable _cv;
    inline static std::shared_ptr<GarbageCollector> _instance;

    GarbageCollector() = default;

    /**
     * Collect garbage
     */
    void collect() {
        auto lock = std::lock_guard<std::mutex>(_mutex);

        for (auto iterator = _pointers.begin(); iterator != _pointers.end();) {
            if (iterator->unique()) {
                *iterator = nullptr;
                iterator = _pointers.erase(iterator);
            }
        }
    }

public:
    /**
     * Interval of garbage collector working
     */
    inline static std::chrono::duration gcInterval = 10s;

    /**
     * Enable instance tracking in the garbage collector
     *
     * @tparam TType - Type of instance
     * @param instance - Concrete instance
     * @return Smart pointer (shared_ptr) to the instance
     *
     * @example
     *
     * // Common usage
     * int main() {
     *     return GarbageCollector::runtime([]() {
     *         auto example = GarbageCollector::getInstance()->alloc(new Example());
     *
     *         return 0;
     *     });
     * }
     *
     * // Macros usage
     * int main() {
     *     return GarbageCollector::runtime([]() {
     *         auto example = gc(new Example());
     *
     *         return 0;
     *     });
     * }
     */
    template<class TType>
    std::shared_ptr<TType> alloc(TType *instance) {
        auto lock = std::lock_guard<std::mutex>(_mutex);
        auto pointer = std::shared_ptr<TType>(instance);

        _pointers.push_back(pointer);

        return pointer;
    }

    /**
     * Get singleton instance of the garbage collector
     *
     * @return Smart pointer (shared_ptr) to the garbage collector
     */
    static std::shared_ptr<GarbageCollector> getInstance() {
        if (!_instance) {
            auto lock = std::lock_guard<std::mutex>(_globalMutex);

            if (!_instance) {
                _instance = std::shared_ptr<GarbageCollector>(new GarbageCollector);
            }
        }

        return _instance;
    }

    /**
     * Decorate a function for the garbage collector instance.
     *
     * Garbage collection is performed in a separate thread.
     * It collects garbage once GarbageCollector::gcInterval.
     * After executing the function, it sends signal to stop and finally collect.
     *
     * @param function - Scope for garbage collector instance
     * @return Code from result of function param
     *
     * @example
     *
     * int main() {
     *     return GarbageCollector::runtime([]() {
     *         auto example = gc(new Example());
     *
     *         return 0;
     *     });
     * }
     */
    static int runtime(int (*function)()) {
        auto gcStopSignal = false;

        std::thread garbageCollectionThread([&gcStopSignal]() {
            while (true) {
                auto lock = std::unique_lock<std::mutex>(_cvMutex);

                auto isNotTimeout = _cv.wait_for(lock, gcInterval, [&gcStopSignal] {
                    return gcStopSignal;
                });

                getInstance()->collect();

                if (isNotTimeout) {
                    break;
                }
            }
        });

        auto code = function();

        {
            auto lock = std::lock_guard<std::mutex>(_cvMutex);
            gcStopSignal = true;
        }

        _cv.notify_one();
        garbageCollectionThread.join();

        return code;
    }
};


#endif //GC_GARBAGECOLLECTOR_H
