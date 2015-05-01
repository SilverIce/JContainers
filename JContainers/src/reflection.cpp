#include "reflection.h"

#include <map>
#include "gtest.h"
#include "skse\PapyrusVM.h"

#include "code_producer.hpp"

namespace reflection {

    void function_info::bind(VMClassRegistry *registry, const char *className) const {
        registrator(registry, className, name.c_str());
        registry->SetFunctionFlags(className, name.c_str(), VMClassRegistry::kFunctionFlag_NoWait);
    }

    static auto makeDB = []() {
        std::map<istring, class_info> classDB;

        for (auto & item : meta<class_info_creator>::getListConst()) {
            class_info& info = item();

            auto found = classDB.find(info.className());
            if (found != classDB.end()) {
                found->second.merge_with_extension(info);
            }
            else {
                classDB[info.className()] = info;
            }
        }

        return classDB;
    };

    const std::map<istring, class_info>& class_database() {
        static std::map<istring, class_info> classDB = makeDB();
        return classDB;
    }

    const function_info* find_function_of_class(const char * functionName, const char *className) {

        const function_info * fInfo = nullptr;

        auto& db = class_database();
        auto itr = db.find(className);
        if (itr != db.end()) {
            auto& cls = itr->second;
            fInfo = cls.find_function(functionName);
        }

        return fInfo;
    }

    TEST(reflection, _)
    {

        class test_class : public reflection::class_meta_mixin_t<test_class> {
        public:

            REGISTER_TES_NAME("test_class");

            test_class() {
                metaInfo.comment = "a class to test reflection";
            }

            static void nothing() {}
            REGISTERF2(nothing, "", "does absolutely nothing");
        };

        TES_META_INFO(test_class);

        // just test whether is doesn't crash and not empty
        auto db = makeDB();

        EXPECT_TRUE(db.find("test_class") != db.end());
        const class_info& cls = db.find("test_class")->second;

        EXPECT_TRUE(cls.className() == "test_class");

        auto func = cls.find_function("nothing");
        EXPECT_TRUE(func != nullptr);
        EXPECT_TRUE(func->name == "nothing");
        EXPECT_TRUE(func->c_func == &test_class::nothing);
    }
}
