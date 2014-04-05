#pragma once

#include <atomic>

namespace collections {

    class object_registry;

    class autorelease_queue {
    public:
        typedef std::lock_guard<bshared_mutex> lock;
        typedef unsigned int time_point;
        //typedef chrono::time_point<chrono::system_clock> time_point;
        typedef std::vector<std::pair<HandleT, time_point> > queue;

    private:

        std::thread _thread;
        bshared_mutex& _mutex;

        object_registry& _registry;
        queue _queue;
        time_point _timeNow;
        
        std::atomic_int_fast8_t _state;
        
        enum queue_state : unsigned char {
            state_none = 0x0,
            state_run = 0x1,
            state_paused = 0x2,
        };

    public:

        void u_clear() {
            u_stop();
            
            _timeNow = 0;
            _queue.clear();
        }

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & _timeNow;
            ar & _queue;
        }

        explicit autorelease_queue(object_registry& registry, bshared_mutex &mt) 
            : _thread()
            , _timeNow(0)
            , _state(state_none)
            , _mutex(mt)
            , _registry(registry)
        {
            start();
        }

        void push(HandleT handle) {
            write_lock g(_mutex);
            _queue.push_back(std::make_pair(handle, _timeNow));
        }

        void start() {
            if ((_state & state_run) == 0) {
                _state = state_run;
                
                write_lock g(_mutex);
                _thread = std::thread(&autorelease_queue::run, std::ref(*this));
            }
        }

        size_t count() {
            read_lock hg(_mutex);
            return u_count();
        }

        queue& u_queue() {
            return _queue;
        }

        size_t u_count() const {
            return _queue.size();
        }

        void setPaused(bool paused) {
            _state |= state_paused;
        }

        void stop() {
            u_stop();
        }
        
        void u_stop() {
            _state = state_none;
            
            if (_thread.joinable()) {
                _thread.join();
            }
        }

        ~autorelease_queue() {
            stop();
        }

    private:
        void cycleIncr() {
            if (_timeNow < (std::numeric_limits<time_point>::max)()) {
                ++_timeNow;
            } else {
                _timeNow = 0;
            }
        }

        time_point lifetimeDiff(time_point time) const {
            if (_timeNow >= time) {
                return _timeNow - time;
            } else {
                return ((std::numeric_limits<time_point>::max)() - time) + _timeNow;
            }
        }

    public:

        enum {
            obj_lifetime = 10, // seconds
            tick_duration = 2, // seconds

            sleep_duration_millis = 100, // milliseconds
        };

        enum {
            tick_duration_millis = 1000 * tick_duration,
            obj_lifeInTicks = obj_lifetime / tick_duration, // ticks
        };

    private:

        static void run(autorelease_queue &self) {
            using namespace std;

            chrono::milliseconds sleepTime(sleep_duration_millis);
            vector<HandleT> toRelease;
            uint32 millisecondCounter = 0; // plain & dumb counter

            while (true) {
                
                unsigned char state = self._state;
                    
                if ((state & state_run) == 0) {
                    break;
                }

                std::this_thread::sleep_for(sleepTime);

                millisecondCounter += sleep_duration_millis;

                if (millisecondCounter >= tick_duration_millis) {

                    millisecondCounter = 0;

                    if ((state & state_paused) == 0) {

                        write_lock g(self._mutex);
                        self._queue.erase(
                            remove_if(self._queue.begin(), self._queue.end(), [&](const queue::value_type& val) {
                                auto diff = self.lifetimeDiff(val.second);
                                bool release = diff >= obj_lifeInTicks;

                                if (release)
                                    toRelease.push_back(val.first);
                                return release;
                        }),
                            self._queue.end());

                        self.cycleIncr();
                    }


                    for(auto& val : toRelease) {
                        auto obj = self._registry.getObject(val);
                        if (obj) {
                            bool deleted = obj->_deleteOrRelease(nullptr);
                            //printf("handle %u %s\n", val, (deleted ? "deleted" : "released"));
                        }
                    }
                    toRelease.clear();
                }
            }

            printf("autorelease_queue finish\n");
        }
    };


}
