#pragma once

namespace collections {

/*
    template<class ID>
    class id_generator {
        std::vector<ID> m_freeNums;
        ID m_highest;

    public:

        id_generator() : m_highest(0) {}

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

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & m_freeNums;
            ar & m_highest;
        }

        void u_clear() {
            m_highest = 0;
            m_freeNums.clear();
        }
    };
*/


    template<class id>
    struct id_generator {

        //
        struct range {
            id first;
            id last;

            static range with_first_last(id first_, id last_) {
                return{ first_, last_ };
            }

            bool empty() const {
                return first > last;
            }

            bool is_near(id value) {

            }

            bool contains_or_near(id value) const {
                return
                    (value >= first - 1 && value <= end());
            }

            bool merge_if_can(id value) {
                if (contains_or_near(value)) {

                    if (value <= end()) {
                    }

                    return true;
                }

                return false;
            }

            id shrink_front() {
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

        id newId() {
            assert(_empty_ranges.empty() == false);
            range& rn = *_current_range;
            auto num = rn.shrink_front();

            if (rn.empty()) {
                _current_range = _empty_ranges.erase(_current_range);

                if (_current_range == _empty_ranges.end()) {
                    _current_range = _empty_ranges.begin();
                }
            }

            return num;
        }


        friend class boost::serialization::access;
        BOOST_SERIALIZATION_SPLIT_MEMBER();


        template<class Archive>
        void save(Archive & ar, const unsigned int version) const {
            ar << _empty_ranges;
            size_t currIdx = _current_range - _empty_ranges.begin();
            ar << currIdx;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int version) {
            ar >> _empty_ranges;
            size_t currIdx = 0;
            ar >> currIdx;

            _current_range = _empty_ranges.begin() + currIdx;
        }


        void u_clear() {
            _empty_ranges = {
                range::with_first_last(1, /*std::numeric_limits<id>::max() - 1*/12000)
            };

            _current_range = _empty_ranges.begin();
        }


        int a = 0;
        int b = 0;
        int c = 0;
        int d = 0;

        void reuseId(id val) {
            range fake = range::with_first_last(val, val);
            // itr >= val
            auto right = std::lower_bound(_empty_ranges.begin(), _empty_ranges.end(), fake);

            range *leftRange = (right > _empty_ranges.begin() ? &*(right - 1) : nullptr);
            leftRange = (leftRange && leftRange->end() == val ? leftRange : nullptr);

            range *rightRange = (right < _empty_ranges.end() ? &*(right) : nullptr);
            rightRange = (rightRange && rightRange->begin() - 1 == val ? rightRange : nullptr);

            if (!leftRange && !rightRange) {
                ++d;
                size_t idx = _current_range - _empty_ranges.begin() + (_current_range < right ? 0 : 1);
                _empty_ranges.insert(right, fake);
                _current_range = _empty_ranges.begin() + idx;

                /*
                if (_current_range == _empty_ranges.end()) {
                _current_range = _empty_ranges.begin();
                }
                */

                assert(_current_range != _empty_ranges.end());
            }
            else if (rightRange && leftRange) {
                ++a;
                // merge ranges
                leftRange->last = rightRange->last;
                // _current_range becomes ivnalid each time we modify _empty_ranges
                size_t idx = _current_range - _empty_ranges.begin() - (_current_range < right ? 0 : 1);
                _empty_ranges.erase(right);
                _current_range = _empty_ranges.begin() + idx;

                /*
                if (_current_range == _empty_ranges.end()) {
                _current_range = _empty_ranges.begin();
                }
                */


                assert(_current_range != _empty_ranges.end());
            }
            else if (leftRange) {
                ++b;
                leftRange->last = val;
            }
            else if (rightRange) {
                ++c;
                rightRange->first = val;
            }
        }
    };


}
