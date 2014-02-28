#include "code_producer.h"

#include <boost/algorithm/string.hpp>
#include <assert.h>
#include <vector>

#include "gtest.h"
#include "tes_meta_info.h"


namespace collections {

    namespace code_producer {

        using namespace tes_binding;

        void _pushArgStr(const FunctionMetaInfo& self, int paramIdx, std::string& str) {
            std::vector<std::string> strings;
            boost::split( strings, std::string(self.args), boost::is_space() );

            if (paramIdx < strings.size() && strings[paramIdx] != "*") {
                str += strings[paramIdx];
            }
            else {
                auto types = self.typeStrings();

                auto argName = types[paramIdx + 1]().tes_arg_name; 
                if (argName.empty() == false) {
                    str += argName;
                } else {
                    str += "arg";
                    str += (char)(paramIdx + '0');
                }
            }
        }

        void _pushComment(const char* comment, std::string& str) {
            if (!comment || !*comment) {
                return ;
            }

            str += "\n;/";
            str += comment;
            str += "\n/;\n";
        }

        std::string function_string(const FunctionMetaInfo& self) {
            std::string str;
            auto types = self.typeStrings();

            _pushComment(self.comment, str);

            if (types[0]().tes_type_name != "void") {
                str += types[0]().tes_type_name;
                str += ' ';
            }

            str += "Function ";
            str += self.funcName;
            str += '(';
            for (int i = 1; i < types.size(); ++i) {
                str += types[i]().tes_type_name;
                str += " ";

                int paramIdx = i - 1;
                _pushArgStr(self, paramIdx, str);

                if (i < (types.size() - 1))
                    str += ", ";
            }

            str += ") global native";
            return str;
        }

        std::string produceClassCode(const class_meta_info& self) {

            std::string str;

            _pushComment(self.comment, str);

            str += "Scriptname ";
            str += self.className;

            if (self.extendsClass) {
                (str += " extends ") += self.extendsClass;
            }

            str += " Hidden\n\n";

            for (auto& itm : self.methods) {
                str += function_string(itm);
                str += "\n";
            }

            return str;
        }

        void produceClassToFile(const class_meta_info& self) {
            auto file = fopen((std::string(self.className) + ".psc").c_str(), "w");
            assert(file);
            if (file) {
                auto code = produceClassCode(self);
                fwrite(code.c_str(), 1, code.length(), file);
                fclose(file);
            }
        }

    }

    using namespace std;

/*
    std::string get_file_contents(const char *filename)
    {
        std::ifstream in(filename, std::ios::in | std::ios::binary);
        if (in)
        {
            std::string contents;
            in.seekg(0, std::ios::end);
            contents.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(&contents[0], contents.size());
            in.close();
            return(contents);
        }
        throw(errno);
    }

    using namespace std;*/


    int charactersPerLine(int total, int maxCharsPerLine) {
        assert(maxCharsPerLine > 0);
        int result = (total) / (1 + total / maxCharsPerLine);
        return result + 1;
    }


    bool isBreak(char c) {
        return c == ' ' ||
            c == '.' ||
            c == ',';
    }

    vector<string> testShit(std::string& data, int charsPerLine = 70) {

        using namespace boost;

        int cpl = charactersPerLine(data.length(), charsPerLine);

        vector<string> result;

        //auto last = data.begin();
        auto first = data.begin();

        while (first != data.end())
        {
            if ((data.end() - first) >= cpl ) {

                auto last = first + cpl;

                while (last > first && isBreak(*last) == false) {
                    printf("last is '%c'\n", *last);
                    --last;
                }

                assert(last > first);

                printf("selected last is '%c'\n", *last);

                if (last > first) {
                    printf("last greater\n");
                }

                if (last > first && isBreak(*last)) {
                    // ok, line was found

                    
                     printf("b1\n");
                } else {
                     printf("b2\n");
                    last = first + cpl;
                }

                result.push_back(string(first, last));

                first = last;
            }
            else {
                 result.push_back(string(first, data.end()));
                 first = data.end();
            }
        }


        return result;

    }

    #define STR(...)    #__VA_ARGS__

    TEST(shit, shit3)
    {
        string str = STR(Anyways one thing that has bugged me about Apropos (more particularly Skyrim,
            Papyrus and Debug.Notification) is that it does scale
            the fontsize based on the length of the string you pass
            to Debug.Notification. I have some crude logic that attempts
            to break up messages into segments, based on a greedy algorithm 
            that tries to keep the message segments under a certain size. Still,
            its far from ideal and it really annoys me. What I really need to solve
            this (other than patching Debug.Notification) is a String Partition algorithm, stated like:);
        auto res = testShit(str);
    }

}
