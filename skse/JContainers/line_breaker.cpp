// line_breaker.cpp : Defines the entry point for the console application.
//


#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <numeric>
#include <algorithm>
#include <sstream>
#include <conio.h>
//#include <list>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include "boost/algorithm/string/classification.hpp"

using namespace std;

namespace {

    float charactersPerLine(int total, int maxCharsPerLine) {
        assert(maxCharsPerLine > 0);
        float result = (total) / (1.f + (float)total / maxCharsPerLine);
        return result;
    }

    struct line
    {
        vector<string> words;

        int charCount() const {
            return std::accumulate(words.begin(), words.end(), 0, [](int val, const string& str) {
                return val + (int)str.length();
            });
        }

        void addWord(const string& word) {
            words.push_back(word);
        }

        static line lineFromString(const string& source) {
            line ln;
            string into;
            stringstream ssin(source);
            while (ssin.good()) {
                ssin >> into;
                ln.words.push_back(into);
            }

            return ln;
        }

        void print() const {
            cout << charCount() << ' ';
            for (auto&str : words) {
                cout << str << ' ';
            }
        }

        string wholeString() const {
            return boost::algorithm::join(words, " ");
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
            //||
            // c == '.' ||
           // c == '(' || c == ')' ||
           // c == ','
            ;
    }


    vector<string> substringRange(string::iterator beg, string::iterator end) {

        vector<string> strings;
        boost::split( strings, boost::make_iterator_range(beg, end), boost::is_any_of(" \n") );

        return strings;

       /* vector<string> strings;

        auto wb = beg, we = beg;
        while (we <= end)  {

            while (wb < end && isBreak(*wb) == true) {
                ++wb;
            }

            we = wb;

            while (we <= end && isBreak(*we) == false) {
                ++we;
            }

            strings.push_back(string(wb, we));

            wb = we = (we + 1);
        }

        return strings;*/
    }

    line_set initialSet(std::string& data, int charsPerLine = 40) {



        float cpl = charactersPerLine(data.length(), charsPerLine);

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


    #define  STR(...)   #__VA_ARGS__

    struct operation {
        line_set *lset;
        int lineIdx;
        bool increase;

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

    string source = csource;


/*
    string source = STR(is a 2005 American documentary film by Steve Anderson, which argues the titular word is key
        to discussions on freedom of speech and censorship. It provides perspectives from art,
        linguistics and society. Oxford English Dictionary editor Jesse Sheidlower, journalism analyst David Shaw,
        and linguists Reinhold Aman and Geoffrey Nunberg explain the terms evolution. Comedian Billy Connolly states it can
        be understood regardless of ones background, and musician Alanis Morissette says its taboo nature gives it power.
        The film contains the last interview of author Hunter S. Thompson before his suicide. It features animated sequences by Bill Plympton.
        The documentary was first screened at the AFI Film Festival at ArcLight Hollywood. The New York Times critic A. O. Scott called
        the film a battle between advocates of morality and supporters of freedom of expression; a review by the American Film Institute said
        this freedom must extend to words that offend. Other reviewers criticized the films length and repetitiveness. Its DVD
        was released in the US and the UK and used in university cour);*/


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

