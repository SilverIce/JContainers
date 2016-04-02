
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <assert.h>
#include <vector>
#include <fstream>

#include "gtest.h"
#include "reflection/reflection.h"

#include "reflection/tes_binding.h"

namespace reflection {

    namespace code_producer {

        void _pushArgStr(const function_info& self, int paramIdx, std::string& str) {
            std::vector<istring> strings;
            boost::split(strings, self.argument_names, boost::is_space());

            if (paramIdx < strings.size() && strings[paramIdx] != "*") {
                str += strings[paramIdx].c_str();
            }
            else {
                auto types = self.param_list_func();

                auto argName = types[paramIdx + 1]().tes_arg_name; 
                if (argName.empty() == false) {
                    str += argName;
                } else {
                    str += "arg";
                    str += (char)(paramIdx + '0');
                }
            }
        }

        void _pushComment(const std::string& comment, std::string& str) {
            if (comment.empty()) {
                return ;
            }

            std::string commentStr(comment);
            boost::replace_all(commentStr, "\n", "\n    ");

            str += "\n;/  ";
            str += commentStr;
            str += "\n/;\n";
        }

        std::string function_to_string(const function_info& self) {
            std::string str;

            auto types = self.param_list_func();

            if (types[0]().tes_type_name != "void") {
                str += types[0]().tes_type_name;
                str += ' ';
            }

            str += "function ";
            str += self.name.c_str();
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

        std::string produceClassCode(const class_info& self) {

            std::string str;

            _pushComment(self.comment, str);

            str += "ScriptName ";
            str += self.className().c_str();

            if (!self.extendsClass.empty()) {
                (str += " extends ") += self.extendsClass.c_str();
            }

            for (auto& itm : self.methods) {
                _pushComment(itm.comment(), str);
                str += function_to_string(itm);
                str += "\n";
            }

            for (auto& itm : self.text_blocks) {
                str += itm.get_text();
                str += "\n";
            }

            return str;
        }

        void produceClassToFile(const class_info& self, const std::string& directoryPath) {

            boost::filesystem::path p(directoryPath);

            p /= self.className().c_str();
            p += ".psc";

            std::ofstream file(p.generic_string());
            file << produceClassCode(self);
        }

        void produceAmalgamatedCodeToFile(const std::map<istring, class_info>& classes, const std::string& directoryPath) {
            using std::endl;

            auto fileName = "JContainers_DomainsExample";

            boost::filesystem::path p(directoryPath);
            p /= fileName;
            p += ".psc";


            std::ofstream file(p.generic_string());

            file << "ScriptName " << fileName << endl;

            function_info tmp;

            for (auto& cls : classes) {
                file << endl << "; " << cls.first.c_str() << endl << endl;

                for (auto& func : cls.second.methods) {
                    if (!func.isStateless()) {
                        tmp = func;
                        tmp.name = cls.first + '_' + tmp.name;

                        file << function_to_string(tmp) << endl;
                    }
                }
            }
        }
    }
}
