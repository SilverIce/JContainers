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

    id_generator() {
        _empty_ranges.push_back(
            range::with_first_last(1, std::numeric_limits<id>::max() - 1)
            );
    }

    id newId() {
        assert(_empty_ranges.empty() == false);
        range& rn = _empty_ranges.back();
        auto num = rn.shrink_front();
        if (rn.empty()) {
            _empty_ranges.pop_back();
        }

        return num;
    }

    void reuse_id(id val) {
        range fake = range::with_first_last(val, val);
        // itr >= val
        auto right = std::lower_bound(_empty_ranges.begin(), _empty_ranges.end(), fake);

        range *leftRange = (right > _empty_ranges.begin() ? &*(right - 1) : nullptr);
        leftRange = (leftRange && leftRange->end() == val ? leftRange : nullptr);

        range *rightRange = (right < _empty_ranges.end() ? &*(right) : nullptr);
        rightRange = (rightRange && rightRange->begin() - 1 == val ? rightRange : nullptr);

        if (leftRange) {
            leftRange->last = val;
        }

        if (rightRange) {
            rightRange->first = val;
        }

        if (rightRange && leftRange) {
            // merge ranges
            leftRange->last = rightRange->last;
            _empty_ranges.erase(right);
        }

        if (!leftRange && !rightRange) {
            _empty_ranges.insert(right, fake);
        }
    }
};

int main(int argc, char* argv[])
{
    srand((unsigned int)time(nullptr));

    id_generator gen;

    std::deque<id> ids;

    for (int i  = 0; i < 100; ++i) {

        for (int d = 0; d < 253; ++d) {
            ids.push_back( gen.newId() );
        }

        for (int j = 0; j < 252; ++j) {
            auto itr = ids.begin() + rand() % ids.size();
            gen.reuse_id(*itr);
            ids.erase(itr);
        }


    }


	return 0;
}

