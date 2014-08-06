#pragma once

namespace collections {


    template<class ID>
    struct id_generator_0 {
        std::vector<ID> m_freeNums;
        ID m_highest; // i.e. [m_highest + 1, max(ID)] is free range

        id_generator_0() : m_highest(0) {}
        /*
        ID newId() {
            if (!m_freeNums.empty()) {
                int num = m_freeNums.back();
                m_freeNums.pop_back();
                return num;
            }

            ++m_highest;
            return m_highest;
        }

        void reuseId(ID num) {
            m_freeNums.push_back(num);
        }

        void u_clear() {
            m_highest = 0;
            m_freeNums.clear();
        }
        */
        template<class Archive>
        void load(Archive & ar) {
            ar & m_freeNums;
            ar & m_highest;
        }
    };


    template<
        class id,
        id min_identifier,
        id max_identifier
    >
    struct id_generator : boost::noncopyable {

        struct range {
            id first;
            id last;

            static range with_first_last(id first_, id last_) {
                return{ first_, last_ };
            }

            bool empty() const {
                return first > last;
            }

            id new_id() {
                assert(empty() == false);
                return first++;
            }

            id begin() const {
                return first;
            }

            id end() const {
                return last + 1;
            }

            bool operator < (const range& other) const {
                return first < other.first;
            }

            bool operator > (const range& other) const {
                return first > other.first;
            }

            template<class Archive>
            void serialize(Archive & ar, const unsigned int version) {
                ar & first;
                ar & last;
            }
        };

        std::deque<range> _empty_ranges;
        typename std::deque<range>::iterator _current_range;

        id_generator() {
            u_clear();
        }

        id new_id() {
            assert(_empty_ranges.empty() == false);
            range& rn = *_current_range;
            auto num = rn.new_id();

            if (rn.empty()) {
                _current_range = _empty_ranges.erase(_current_range);

                if (_current_range == _empty_ranges.end()) {
                    _current_range = _empty_ranges.begin();
                }
            }

            return num;
        }

        void u_clear() {
            _empty_ranges = {
                range::with_first_last(min_identifier, max_identifier)
            };
            _current_range = _empty_ranges.begin();
        }

        bool is_free_id(id val) const {
            range fake = range::with_first_last(val, val);
            // right > val
            auto right = std::upper_bound(_empty_ranges.begin(), _empty_ranges.end(), fake);
            const range *leftRange = (right > _empty_ranges.begin() ? &*(right - 1) : nullptr);

            return (leftRange && leftRange->begin() <= val && val < leftRange->end());
        }

        bool is_valid() const {
            auto noOverlap = [&]() {
                for (size_t i = 1; i < _empty_ranges.size(); ++i) {
                    const auto& prev = _empty_ranges[i - 1];
                    const auto& curr = _empty_ranges[i];

                    if (!(prev.begin() < curr.begin() && prev.end() < curr.end())) {
                        return false;
                    }
                }

                return true;
            };

            return !_empty_ranges.empty() && _current_range != _empty_ranges.end() && noOverlap();
        }

        void reuse_id(id val) {
            range fake = range::with_first_last(val, val);
            // right >= val
            auto right = std::lower_bound(_empty_ranges.begin(), _empty_ranges.end(), fake);

            range *leftRange = (right > _empty_ranges.begin() ? &*(right - 1) : nullptr);
            leftRange = (leftRange && leftRange->end() == val ? leftRange : nullptr);

            range *rightRange = (right != _empty_ranges.end() ? &*(right) : nullptr);
            rightRange = (rightRange && rightRange->begin() - 1 == val ? rightRange : nullptr);

            if (!leftRange && !rightRange) {
                size_t idx = _current_range - _empty_ranges.begin() + (_current_range < right ? 0 : 1);
                _empty_ranges.insert(right, fake);
                _current_range = _empty_ranges.begin() + idx;

                assert(_current_range != _empty_ranges.end());
            }
            else if (rightRange && leftRange) {
                // merge ranges
                leftRange->last = rightRange->last;
                // _current_range becomes ivnalid each time we modify _empty_ranges
                size_t idx = _current_range - _empty_ranges.begin() - (_current_range < right ? 0 : 1);
                _empty_ranges.erase(right);
                _current_range = _empty_ranges.begin() + idx;

                assert(_current_range != _empty_ranges.end());
            }
            else if (leftRange) {
                leftRange->last = val;
            }
            else if (rightRange) {
                rightRange->first = val;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        
        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();

        template<class Archive>
        void save(Archive & ar, const unsigned int version) const {
            size_t currIdx = _current_range - _empty_ranges.begin();
            ar << _empty_ranges << currIdx;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version) {

            switch (version)
            {
            default:
                jc_assert(false);
                break;
            case 1: {
                size_t currIdx = 0;

                ar >> _empty_ranges >> currIdx;

                _current_range = _empty_ranges.begin() + currIdx;
            }
                break;
            case 0:
                load_0(ar);
                break;
            }
        }

        template <class Archive> void load_0(Archive& ar) {
            _empty_ranges.clear();
            _current_range = _empty_ranges.end();

            id_generator_0<id> old;
            old.load(ar);

            std::sort(old.m_freeNums.begin(), old.m_freeNums.end());
            std::unique(old.m_freeNums.begin(), old.m_freeNums.end());

            for (const auto& num : old.m_freeNums) {

                if (!_empty_ranges.empty() && num - _empty_ranges.back().last == 1) {
                    _empty_ranges.back().last = num;
                }
                else {
                    _empty_ranges.push_back(range::with_first_last(num, num));
                }
            }

            _empty_ranges.push_back(range::with_first_last(old.m_highest + 1, max_identifier));
            _current_range = _empty_ranges.end() - 1;
        }
    };

    // 2147483647 - 2 identifiers is not enough? anyone?
    typedef id_generator<HandleT, 1, 0x7FFFFFFF - 1> id_generator_type;


#   ifndef TEST_COMPILATION_DISABLED

    TEST(id_generator, t)
    {
        srand((unsigned int)time(nullptr));

        typedef uint8_t id_t;

        id_generator<id_t, 1, 200> gen;

        std::deque<id_t> ids;

        for (int i = 0; i < 100; ++i) {

            for (int d = 0; d < 50; ++d) {
                auto id = gen.new_id();
                EXPECT_TRUE(std::find(ids.begin(), ids.end(), id) == ids.end()); // no duplicates
                ids.push_back(id);

                EXPECT_TRUE(gen.is_valid());
            }

            for (int j = 0; j < 49; ++j) {
                auto itr = ids.begin() + rand() % ids.size();
                gen.reuse_id(*itr);
                ids.erase(itr);

                EXPECT_TRUE(gen.is_valid());
            }
        }
    }

#   endif

}

BOOST_CLASS_VERSION(collections::id_generator_type, 1);
