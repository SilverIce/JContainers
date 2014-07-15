
#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <numeric>
#include <algorithm>
#include <sstream>
#include <conio.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/range/iterator_range.hpp>

using namespace std;
namespace bs = boost;

namespace {

    typedef bs::iterator_range<const char *> cstring;

    float charactersPerLine(int total, int maxCharsPerLine) {
        jc_assert(maxCharsPerLine > 0);
        float result = (total) / (1.f + (float)total / maxCharsPerLine);
        return result;
    }

    struct line
    {
        vector<cstring> words;

        int charCount() const {
            return std::accumulate(words.begin(), words.end(), 0, [](int val, const cstring& str) {
                return val + (int)str.size();
            })
                + (words.size() > 0 ? words.size() - 1 : 0);
        }

        void addWord(const cstring& word) {
            words.push_back(word);
        }

        void print() const {
            cout << charCount() << ' ';
            for (const auto& str : words) {
                cout << str << ' ';
            }
        }

        string wholeString() const {
            std::string result;
            for (const auto& str : words) {
                result.append(str.begin(), str.size());
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

        void print() const {
            for (auto& line : lines) {
                line.print();
                cout << endl;
            }
        }

        vector<string> strings() const {
            vector<string> result;

            std::transform(lines.begin(), lines.end(), std::back_inserter(result), [&](const line& ln) {
                return ln.wholeString();
            });

            return result;
        }
    };


    bool isBreak(char c) {
        return
            c == ' ' || 
            c == '\n'
            ;
    }

    vector<cstring> substringRange(cstring::iterator beg, cstring::iterator end) {
        vector<cstring> strings;
        boost::split( strings, boost::make_iterator_range(beg, end), &isBreak );
        return boost::split( strings, boost::make_iterator_range(beg, end), &isBreak );
    }

    line_set initialSet(const cstring& data, int charsPerLine = 40) {

        float cpl = charactersPerLine(data.size(), charsPerLine);

        line_set result;

        result.idealCpl = cpl;

        //auto last = data.begin();
        auto first = data.begin();

        while (first != data.end())
        {
            auto& line = result.newLine();

            if ((data.end() - first) >= cpl ) {

                auto middle = first + (int)cpl;
                auto left = middle, right = middle;

                while (left > first && isBreak(*left) == false) {
                    //printf("last is '%c'\n", *left);
                    --left;
                }

                while (right < data.end() && isBreak(*right) == false) {
                    //printf("last is '%c'\n", *right);
                    ++right;
                }

                auto selected = (middle - left < (right - middle) * 1.2) ? left : right;

                line.words = substringRange(first, selected);

                first = selected;
            }
            else {
                line.words = substringRange(first, data.end());

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

vector<string> StringUtil_wrapString(const char *csource, int charsPerLine)
{
    cstring source = cstring(csource, csource + strlen(csource));

    line_set set = initialSet(source, charsPerLine);

    //cout << "ideal " << set.idealCpl << endl;

    operation setOpr;

    float delta = set.meanSquare();

    //auto sorted = set.sortedIndices();

    //set.print();

    int loop = 0;
    while (loop < 40)
    {
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

    //cout << endl;

    //set.print();

   // getch();

    return set.strings();
}

