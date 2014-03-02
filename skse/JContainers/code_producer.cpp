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
}
