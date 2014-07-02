
#include <string>
#include <assert.h>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cctype>
#include <locale>
#include <codecvt>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/range/iterator_range.hpp>

namespace collections {

using namespace std;
namespace bs = boost;

namespace {

    typedef basic_string<char32_t, char_traits<char32_t>, allocator<char32_t>> wide_string;
    //typedef std::codecvt<char32_t, char, std::mbstate_t> wide_string_codec;
    typedef std::codecvt_utf8 < char32_t > wide_string_codec;
    typedef std::wstring_convert<wide_string_codec, char32_t > wide_string_converter;
    typedef bs::iterator_range<const char32_t *> wstring_range;

    float charactersPerLine(int total, int maxCharsPerLine) {
        assert(maxCharsPerLine > 0);
        float result = (total) / (1.f + (float)total / maxCharsPerLine);
        return result;
    }

    struct line
    {
        vector<wstring_range> words;

        int charCount() const {
            return std::accumulate(words.begin(), words.end(), 0, [](int val, const wstring_range& str) {
                return val + (int)str.size();
            })
                + (words.size() > 0 ? words.size() - 1 : 0);
        }

        void addWord(const wstring_range& word) {
            words.push_back(word);
        }

        string toUTF8String(wide_string_converter& conv) const {
            std::string result;
            for (const auto& str : words) {
                result.append(conv.to_bytes(str.begin(), str.end()));
                result += ' ';
            }

            return result;
        }
    };

    struct line_set
    {
        vector<line> lines;
        float idealCpl;

        line& operator[] (int idx) {
            return lines[idx];
        }

        line& top()  {
            return lines.back();
        }

        line& newLine() {
            addLine(line());
            return top();
        }

        void addLine(const line& ln) {
            lines.push_back(ln);
        }

        bool isLast(int lineIdx) const {
            return lineIdx == (lines.size() - 1);
        }

        bool isFirst(int lineIdx) const {
            return lineIdx == 0;
        }

        void incrLength(int lineIdx) {
            if (isFirst(lineIdx)) {
            
                if (lines.size() >= 2) {
                    decrLength(1);
                }

                return ;
            }

            auto& from =  lines[lineIdx - 1].words;

            if (from.empty()) {
                return ;
            }

            auto word = from.back();
            from.pop_back();

            auto& to = lines[lineIdx].words;
            to.insert(to.begin(), word);
        }

        void decrLength(int lineIdx) {
            if (isFirst(lineIdx)) {

                if (lines.size() >= 2) {
                    incrLength(1);
                }

                return ;
            }

            auto& from = lines[lineIdx].words;

            if (from.empty()) {
                return ;
            }

            auto word = from.front();
            from.erase(from.begin());

            auto& to = lines[lineIdx - 1].words;
            to.push_back(word);
        }

        float meanSquare() const {
            float avg = average();

            return std::accumulate(lines.begin(), lines.end(), 0.f, [=](float val, const line& ln) {
                return val + powf(ln.charCount() - avg, 2.f);
            })
                / (float)lines.size();
        }

        float charDiffIn(int lineIdx) const {
            return charDiffIn(lines[lineIdx]);
        }

        float charDiffIn(const line& ln) const {
            return ln.charCount() - average();
        }

        float average() const {
            return charCount() / (float)lines.size();
        }

        int charCount() const {
            return std::accumulate(lines.begin(), lines.end(), 0, [](int val, const line& ln) {
                return val + ln.charCount();
            });
        }

    /*
        int minIdx() const {
            return std::min_element(lines.cbegin(), lines.cend(), [=](const line& ln, const line& smallest){
                return charDiffIn(ln) < charDiffIn(smallest);
            })
                - lines.begin();
        }
    */
    /*

        vector<int> sortedIndices() const {
            vector<int> indices;


            int idx = 0;
            std::transform(lines.begin(), lines.end(), std::back_inserter(indices), [&](const line& ln) {
                return idx++;
            });

            std::sort(indices.begin(), indices.end(), [=](int left, int right) {
                return fabs(charDiffIn(left)) < fabs(charDiffIn(right)); 
            });

            return indices;
        }*/

    /*
        int maxIdx() const {
            return std::max_element(lines.cbegin(), lines.cend(), [=](const line& ln, const line& smallest){
                return charDiffIn(ln) < charDiffIn(smallest);
            })
                - lines.begin();
        }*/

        vector<string> utf8Strings(wide_string_converter& conv) const {
            vector<string> result;

            std::transform(lines.begin(), lines.end(), std::back_inserter(result), [&](const line& ln) {
                return ln.toUTF8String(conv);
            });

            return result;
        }
    };

    bool is_blank_or_space(char32_t c) {
        // from https://www.cs.tut.fi/~jkorpela/chars/spaces.html
        switch (c)
        {
        case 0x20:
        case 0xa0:
        case '\n':
        case '\r':
        case '\t':
        case '\b':
        case '\f':

        case 0x2000:
        case 0x2001:
        case 0x2003:
        case 0x2004:
        case 0x2005:
        case 0x2006:
        case 0x2007:
        case 0x2008:
        case 0x2009:
        case 0x200a:
        case 0x200b:

        case 0x202f:
        case 0x205f:

        case 0x3000:
        case 0xfeff:
            return true;
        default:
            return false;
        }
    }

    vector<wstring_range> split_range(wstring_range::iterator beg, wstring_range::iterator end) {
        vector<wstring_range> strings;
		return boost::split(strings, boost::make_iterator_range(beg, end), &is_blank_or_space);
    }

    line_set initialSet(const wstring_range& data, int charsPerLine = 40) {

        float cpl = charactersPerLine(data.size(), charsPerLine);

        line_set result;

        result.idealCpl = cpl;

        //auto last = data.begin();
        auto first = data.begin();

        while (first != data.end())
        {
            auto& line = result.newLine();

            if ((data.end() - first) >= cpl ) {

				auto nearestBreak = [first, cpl, &data]() {
					auto middle = first + (int)cpl;
					auto left = middle, right = middle;

					while (left > first && is_blank_or_space(*left) == false) {
						--left;
					}

					while (right < data.end() && is_blank_or_space(*right) == false) {
						++right;
					}

					return (left != first && middle - left < (right - middle) * 1.2) ? left : right;
				};

				auto selected = nearestBreak();
				line.words = split_range(first, selected);

                first = selected;
            }
            else {
				line.words = split_range(first, data.end());

                first = data.end();
            }
        }

        return result;

    }

    struct operation {
        line_set *lset;
        int lineIdx;
        bool increase;

        operation() {
            lset = nullptr;
            lineIdx = 0;
            increase = false;
        }

        void doIncr(bool incr, int lnIdx, line_set& set) {
            lineIdx = lnIdx;
            increase = incr;
            lset = &set;

            if (incr) {
                set.incrLength(lnIdx);
            } else {
                set.decrLength(lnIdx);
            }
        }

        void revert() {
            doIncr(!increase, lineIdx, *lset);
        }
    };

}

vector<string> wrap_string(const char *csource, int charsPerLine)
{
	assert(csource);

    wide_string_converter converter;
    auto wstr = converter.from_bytes(csource);

    if (wstr.empty()) {
        return vector<string>();
    }

    wstring_range range(&wstr.front(), &wstr.back() + 1);

    line_set set = initialSet(range, charsPerLine);

    operation setOpr;

    float delta = set.meanSquare();

    int loop = 0;
    while (loop < 40) {
        ++loop;

        float oldDelta = set.meanSquare();

        for (size_t i = 0; i < set.lines.size(); ++i) {

            bool needIncr = set.charDiffIn(i) < 0;

            float oldDelta = set.meanSquare();
            setOpr.doIncr(needIncr, i, set);
            float newDelta = set.meanSquare();

            if (newDelta > oldDelta) {
                setOpr.revert();
            } else {
                i = i;
            }

        }

        float newDelta = set.meanSquare();

        if (fabs(oldDelta - newDelta) < 0.00001f) {
            break;
        }
    }

    return set.utf8Strings(converter);
}
}
