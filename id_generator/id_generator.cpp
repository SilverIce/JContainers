// id_generator.cpp : Defines the entry point for the console application.
//

#include <deque>
#include <stdint.h>
#include <limits>
#include <algorithm>
#include <assert.h>
#include <time.h>


typedef uint32_t id;

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
};

struct id_generator {
    //
    std::deque<range> _empty_ranges;
    std::deque<range>::iterator _current_range;

    id_generator() {
        _empty_ranges.push_back(
            range::with_first_last(1, /*std::numeric_limits<id>::max() - 1*/12000)
            );

        _current_range = _empty_ranges.begin();
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

    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;

    void reuse_id(id val) {
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

int main(int argc, char* argv[])
{
    srand((unsigned int)time(nullptr));

    id_generator gen;

    std::deque<id> ids;

    for (int i  = 0; i < 1000; ++i) {

        for (int d = 0; d < 50; ++d) {
            ids.push_back( gen.newId() );
        }

        for (int j = 0; j < 40; ++j) {
            auto itr = ids.begin() + rand() % ids.size();
            gen.reuse_id(*itr);
            ids.erase(itr);
        }


    }


	return 0;
}

