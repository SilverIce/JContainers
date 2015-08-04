#pragma once

#include <atomic>
#include <deque>
#include <boost\serialization\version.hpp>
#include <boost\asio\io_service.hpp>
#include <boost\asio\deadline_timer.hpp>
#include "common\IThread.h"

namespace collections {

    class object_registry;

    // The purpose of autorelease_queue (aqueue) is to temporarily own an object and increase an object's lifetime
    class autorelease_queue : boost::noncopyable {
    public:
        typedef std::lock_guard<bshared_mutex> lock;
        typedef object_base::time_point time_point;

        struct object_lifetime_policy {
            static void retain(object_base * p) {
                p->_aqueue_retain();
            }

            static void release(object_base * p) {
                p->_aqueue_release();
            }
        };

        typedef boost::intrusive_ptr_jc<object_base, object_lifetime_policy> queue_object_ref;
        typedef std::deque<queue_object_ref> queue;

    private:

        object_registry& _registry;
        queue _queue;
        time_point _tickCounter;

        spinlock _queue_mutex;

        std::atomic_bool    _stopped;
        
        // it's not overkill to use asio?
        boost::asio::io_service _io;
        boost::asio::io_service::work _work;
        boost::asio::deadline_timer _timer;
        IThread _thread;

        // reusable array for temp objects
        std::vector<queue_object_ref> _toRelease;

    public:

        void u_clear() {
            stop();
            
            _tickCounter = 0;
            _queue.clear();
            _toRelease.clear();
        }

        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const {
            jc_assert(version == 2);
            ar & _tickCounter;
            ar & _queue;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version) {
            ar & _tickCounter;

            switch (version) {
            case 2:
                ar & _queue;
                break;
            case 1: {
                typedef std::deque<std::pair<queue_object_ref, time_point> > queue_old;
                queue_old old;
                ar & old;
                for (const auto& pair : old) {
                    auto object = pair.first.get();
                    if (object) {
                        _queue.push_back(std::move(pair.first));
                        object->_aqueue_push_time = pair.second;
                    }
                }
                break;
            }
            case 0: {
                typedef std::vector<std::pair<Handle, time_point> > queue_old;
                queue_old old;
                ar & old;
                for (const auto& pair : old) {
                    auto object = _registry.u_getObject(pair.first);
                    if (object) {
                        _queue.push_back(object);
                        object->_aqueue_push_time = pair.second;
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
            : _tickCounter(0)
            , _registry(registry)
            // lot of dependencies :(
            , _io()
            , _work(_io)
            , _timer(_io)
        {
            _stopped.store(true, std::memory_order_relaxed);
            _thread.Start([](void *io_ptr) { reinterpret_cast<decltype(_io)*>(io_ptr)->run(); }, &_io);
            start();
            //jc_debug("aqueue created")
        }

        // prolongs object lifetime for ~10 seconds
        void prolong_lifetime(object_base& object, bool isPublic) {
            //jc_debug("aqueue: added id - %u as %s", object._uid(), isPublic ? "public" : "private");

            spinlock::guard g(_queue_mutex);
            object._aqueue_push_time = isPublic ? _tickCounter : time_subtract(_tickCounter, obj_lifeInTicks);
            if (!object.is_in_aqueue()) {
                _queue.push_back(&object);
            }
        }

        void not_prolong_lifetime(object_base& object) {
            if (object.is_in_aqueue()) {
                //jc_debug("aqueue: removed id - %u", object._uid());
                spinlock::guard g(_queue_mutex);
                object._aqueue_push_time = time_subtract(_tickCounter, obj_lifeInTicks);
            }
        }

        // amount of objects in queue
        size_t count() {
            spinlock::guard g(_queue_mutex);
            return u_count();
        }

        queue& u_queue() {
            return _queue;
        }

        size_t u_count() const {
            return _queue.size();
        }

        // starts asynchronouos aqueue run, asynchronouosly releases objects when their time comes, starts timers, 
        void start() {
            if (false == _stopped.exchange(false, std::memory_order_relaxed)) {
                return; //already running
            }

            u_startTimer();
        }

        // stops async. processes launched by @start function,
        void stop() {
            // with _timer_mutex locked it will wait for @tick function execution completion
            // the point is to execute @stop after @tick (so @tick will not auto-restart the timer)
            _stopped.store(true, std::memory_order_relaxed);
            _timer.cancel();

            // wait for completion, ensure it's stoppped
            _timer.wait();
        }

        void u_nullify() {
            for (auto &ref : _queue) {
                ref.jc_nullify();
            }
        }

        ~autorelease_queue() {
            // at this point of time aqueue should be empty as the whole system is in half-alive-half-destroyed state
            // if aqueue is not empty -> object_base::release_from_queue -> accesses died object_registry::removeObject -> crash

            stop();
            _io.stop();

            _thread.Stop();
            WaitForSingleObject(_thread.GetHandle(), INFINITE);

            //jc_debug("aqueue destroyed")
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
            return time_subtract(_tickCounter, time);
        }

    public:

        enum {
            obj_lifetime = 10, // seconds
            tick_duration = 2, // seconds, interval between ticks, interval between aqueue tests its objects for should-be-released state,
            // and releases if needed
            one_tick = 1, // em, one tick is one tick..
        };

        enum {
            obj_lifeInTicks = obj_lifetime / tick_duration, // object's lifetime described in amount-of-ticks
        };

    private:

        void u_startTimer() {

            if (_stopped.load(std::memory_order_relaxed) == true) {
                return;
            }

            boost::system::error_code code;
            _timer.expires_from_now(boost::posix_time::seconds(tick_duration), code);
            assert(!code);

            _timer.async_wait([this](const boost::system::error_code& e) {
                //jc_debug("aqueue code: %s - %u", e.message().c_str(), e.value());
                if (!e) { // !e means no error, no cancel, successful completion
                    this->tick();
                    this->u_startTimer();
				}
				else {
					//jc_debug("aqueue timer was cancelled");
				}
            });
        }

        void tick() {
            {
                spinlock::guard g(_queue_mutex);
                _queue.erase(
                    std::remove_if(_queue.begin(), _queue.end(), [&](const queue_object_ref& ref) {
                        jc_assert(ref.get());
                        auto diff = time_subtract(_tickCounter, ref->_aqueue_push_time) + 1; // +1 because 0,1,2,3,4,5 is 6 ticks
                        bool release = diff >= obj_lifeInTicks;
                        //jc_debug("id - %u diff - %u, rc - %u", ref->_uid(), diff, ref->refCount());

                        // just move out object reference to release it later
                        if (release)
                            _toRelease.push_back(std::move(ref));
                        return release;
                    }),
                    _queue.end());

                // Increments tick counter, _tickCounter += 1
                _tickCounter = time_add(_tickCounter, one_tick);
            }

            //jc_debug("%u objects released", _toRelease.size());

            // How much owners an object may have right now?
            // queue - +1
            // stack may reference
            // tes ..
            // Item..
            _toRelease.clear();
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

BOOST_CLASS_VERSION(collections::autorelease_queue, 2);
