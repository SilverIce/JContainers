#pragma once

#include <atomic>
#include <deque>
#include "boost\serialization\version.hpp"

namespace collections {

    class object_registry;

    class autorelease_queue {
    public:
        typedef std::lock_guard<bshared_mutex> lock;
        typedef uint32_t time_point;

        struct object_lifetime_policy {
            static void retain(object_base * p) {
                p->retain();
            }

            static void release(object_base * p) {
                p->release_from_queue();
            }
        };

        typedef boost::intrusive_ptr_jc<object_base, object_lifetime_policy> queue_object_ref;
        typedef std::vector<std::pair<Handle, time_point> > queue_old;
        typedef std::deque<std::pair<queue_object_ref, time_point> > queue;

    private:

        std::thread _thread;
        bshared_mutex _mutex;

        object_registry& _registry;
        queue _queue;
        time_point _timeNow;
        
        std::atomic_int_fast8_t _state;
        
        enum queue_state : unsigned char {
            state_none = 0x0,
            state_run = 0x1,
            //state_paused = 0x2,
        };

    public:

        void u_clear() {
            u_stop();
            
            _timeNow = 0;
            _queue.clear();
        }

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const {
            jc_assert(version == 1);
            ar & _timeNow;
            ar & _queue;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version) {
            ar & _timeNow;

            switch (version) {
            case 1:
                ar & _queue;
                break;
            case 0: {
                queue_old old;
                ar & old;

                for (const auto& pair : old) {
                    auto object = _registry.u_getObject(pair.first);
                    if (object) {
                        _queue.push_back(std::make_pair(object, pair.second));
                    }
                }
                break;
            }
            default:
                jc_assert(false);
                break;
            }
        }

        explicit autorelease_queue(object_registry& registry) 
            : _thread()
            , _timeNow(0)
            , _state(state_none)
            , _mutex()
            , _registry(registry)
        {
            start();
        }

        // prolongs object lifetime for ~10 seconds
        // reduceLifeTimeBy sets lifetime to 10 / reduceLifeTimeInNTimes, zero disables reduction
        void prolong_lifetime(object_base& object, uint32_t reduceLifeTimeInNTimes = 0) {
            write_lock g(_mutex);
            uint32_t pushedTime = reduceLifeTimeInNTimes ? time_subtract(_timeNow, obj_lifetime - obj_lifetime / reduceLifeTimeInNTimes) : _timeNow;
            _queue.push_back(std::make_pair(&object, pushedTime));
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

        void stop() {
            u_stop();
        }
        
        void u_stop() {
            _state = state_none;
            
            if (_thread.joinable()) {
                _thread.join();
            }
        }

        void u_nullify() {
            for (auto &pair : _queue) {
                pair.first.jc_nullify();
            }
        }

        ~autorelease_queue() {
            stop();
        }

    private:
        void cycleIncr() {
            _timeNow = time_add(_timeNow, 1);
        }

    public:
        static time_point time_subtract(time_point minuend, time_point subtrahend) {
            if (minuend >= subtrahend) {
                return minuend - subtrahend;
            }
            else {
                return (std::numeric_limits<time_point>::max)() - (subtrahend - minuend);
            }
        }

        static time_point time_add(time_point a, time_point b) {
            auto max = (std::numeric_limits<time_point>::max)();
            if ((max - a) > b) {
                return a + b;
            }
            else {
                return b - (max - a);
            }
        }

        // result is (_timeNow - time)
        time_point lifetimeDiff(time_point time) const {
            return time_subtract(_timeNow, time);
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
            vector<queue_object_ref> toRelease;
            uint32_t millisecondCounter = 0; // plain & dumb counter

            while (true) {
                
                if ((self._state & state_run) == 0) {
                    break;
                }

                std::this_thread::sleep_for(sleepTime);

                millisecondCounter += sleep_duration_millis;

                if (millisecondCounter >= tick_duration_millis) {

                    millisecondCounter = 0;

                    {
                        write_lock g(self._mutex);
                        self._queue.erase(
                            remove_if(self._queue.begin(), self._queue.end(), [&](const queue::value_type& val) {
                                auto diff = self.lifetimeDiff(val.second);
                                bool release = diff >= obj_lifeInTicks;

                                // just move out object reference to release it later
                                if (release)
                                    toRelease.push_back(val.first);
                                return release;
                        }),
                            self._queue.end());

                        self.cycleIncr();
                    }

                    // How much owners object may have right now?
                    // queue - +1
                    // stack may reference
                    // tes ..
                    // Item..
                    toRelease.clear();
                }
            }
        }
    };

    TEST(autorelease_queue, time_wrapping)
    {
        auto max = (std::numeric_limits<autorelease_queue::time_point>::max)();

        auto res = autorelease_queue::time_add(max, 1);
        EXPECT_TRUE(res == 1);

        res = autorelease_queue::time_add(max, 0);
        EXPECT_TRUE(res == 0);

        res = autorelease_queue::time_add(max, max);
        EXPECT_TRUE(res == max);

        res = autorelease_queue::time_add(max, 10);
        EXPECT_TRUE(res == 10);

        res = autorelease_queue::time_subtract(0, 1);
        EXPECT_TRUE(res == max - 1);

        res = autorelease_queue::time_subtract(10, 20);
        EXPECT_TRUE(res == max - 10);

        // test inversivity:
        uint32_t a = 40, b = 20;
        // if c == b - a then b == c + a and a == b - c
        uint32_t c = autorelease_queue::time_subtract(b, a);

        EXPECT_TRUE(b == autorelease_queue::time_add(a, c));
        EXPECT_TRUE(a == autorelease_queue::time_subtract(b, c));

        a = 8, b = 9;
        // c == a + b, a == c - b and b == c - a
        c = autorelease_queue::time_add(a,b);
        EXPECT_TRUE(a == autorelease_queue::time_subtract(c, b));
        EXPECT_TRUE(b == autorelease_queue::time_subtract(c, a));
    }
}

BOOST_CLASS_VERSION(collections::autorelease_queue, 1);
