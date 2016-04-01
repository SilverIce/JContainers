#pragma once

#include <type_traits>

#include "boost/range/algorithm/remove_if.hpp"
#include "util/string_normalize.h"
#include "util/cstring.h"
#include "skse/skse.h"

#include "collections/collections.h"
#include "collections/access.h"
#include "collections/context.h"
#include "collections/functions.h"

namespace collections {

    namespace autocleanup {

        template<
            class Arg1, class Arg2, class R,
            class Func = R(Arg1&&, Arg2&&)
        >
        struct flipped_func {
            Func _f;

            R operator()(Arg1&& a1, Arg2&& a2) {
                return _f(std::forward<Arg2>(a2), std::forward<Arg1>(a1));
            }
        };

        template<
            class Arg1, class Arg2, class R,
            class Func = R(Arg1&&, Arg2&&),
            class Result = flipped_func<Arg1, Arg2, R>
        >
        static Result flip(Func && f) {
            return Result{ std::forward<Func>(f) };
        }

#define JC_AUTOCLEANUP_PATH ".__internals.pathRemoval" 

        namespace Entry {
            map& make(tes_context& ctx, const std::string& name) {
                return map::objectWithInitializer([&](map& me){
                    me.u_set("pluginName", name);
                    me.u_set("paths", array::object(ctx));
                },
                    ctx);
            }

            array* get_paths(map& m) {
                return ca::get(m, ".paths").get_value_or(item()).object()->as<array>();
            }

            std::string get_name(map& m) {
                return ca::get<std::string>(m, ".pluginName").get_value_or(std::string());
            }

            void set_name(map& m, const std::string& name) {
                ca::assign_creative(m, ".pluginName", name);// .get_value_or(std::string());
            }
        }

        static array* pathRemovalEntries(tes_context& context) {
            return ca::get(context.root(), JC_AUTOCLEANUP_PATH)
                .get_value_or(item())
                .object()->as<array>();
        }

        static map* findAutoCleanupEntry(tes_context& context, const char* pluginNameUnsafe) {
            const array* entries = ca::get(context.root(), JC_AUTOCLEANUP_PATH)
                .get_value_or(item())
                .object()->as<array>();

            if (entries) {
                map* e = nullptr;

                object_lock l{ entries };
                auto itr = boost::find_if(entries->u_container(), [&pluginNameUnsafe](const item& itm) {
                    auto m = itm.object()->as<map>();
                    return m ? Entry::get_name(*m) == pluginNameUnsafe : false;
                });

                return itr != entries->u_container().cend() ? (*itr).object()->as<map>() : nullptr;
            }

            return nullptr;
        }

        static map& makeAutoCleanupEntry(tes_context& context, const char* pluginNameUnsafe) {
            array* entries = nullptr;
            ca::visit_value(
                context.root(),
                JC_AUTOCLEANUP_PATH,
                ca::creative,
                [&context, &entries](item& entriesVal) {
                    entries = entriesVal.object()->as<array>();
                    if (!entries) {
                        entries = &array::object(context);
                        entriesVal = entries;
                    }
                }
            );

            map* e = nullptr;

            object_lock l{entries};
            auto itr = boost::find_if(entries->u_container(), [&pluginNameUnsafe](const item& itm) {
                auto m = itm.object()->as<map>();
                return m ? Entry::get_name(*m) == pluginNameUnsafe : false;
            });

            if (itr == entries->end()) {
                e = &Entry::make(context, pluginNameUnsafe);
                entries->u_push(e);
            }
            else {
                e = (*itr).object()->as<map>();
                assert(e);
            }

            return *e;
        }

        void setPathForAutoremoval(tes_context& context, const char* pluginNameUnsafe, const char* path) {
            map& entry = makeAutoCleanupEntry(context, pluginNameUnsafe);
            array_functions::uniqueInsert(item{ path }, Entry::get_paths(entry));
        }

        void unsetPathForAutoremoval(tes_context& context, const char* pluginNameUnsafe, const char* path) {
            auto entry = findAutoCleanupEntry(context, pluginNameUnsafe);
            auto paths = entry ? Entry::get_paths(*entry) : nullptr;
            array_functions::uniqueRemove(item{ path }, paths);
            // Now remove the entry if there are no more members (except paths)
            // But we don't need to do this because we are free to remove the entry when plugin will be disabled
        }

        void u_autocleanup(tes_context& context) {
            u_autocleanup(context, [](const std::string& espModName) {
                return skse::is_plugin_loaded(espModName.c_str());
            });
        }

        void u_autocleanup(tes_context& context, std::function<bool(const std::string&)>&& isPluginLoaded) {

            /*
                Horrible c++ code below which is roughly equal to:

                int entries = JDB.solveObj(".__autocleanup.pathRemoval")
                string k = JMap.nextKey(entries)
                while k
                    int entry = JMap.getObj(entries, k)
                    string plugin = JMap.getStr(entry, "pluginName")

                    if Game.GetModByName(plugin) == 0xff
                        int paths = JMap.getObj(entry, "paths")
                        int i = 0
                        while i < JArray.count(paths)
                            JDB.setObj(JArray.getStr(paths, i), 0)
                            i += 1
                        endwhile
                    endif

                    k = JMap.nextKey(entries, k)
                endwhile
                */

            array* pathRemoval = pathRemovalEntries(context);

            if (pathRemoval) {
                //DataHandler& dataHandler = *DataHandler::GetSingleton();
                auto eraseCondition = [&context, &isPluginLoaded](const item& p) -> bool {
                    if (auto entry = p.object()->as<map>()) {
                        std::string pluginName = Entry::get_name(*entry);
                        if (isPluginLoaded(pluginName)) {
                            return false; // do not erase entry
                        }

                        if (array* paths = Entry::get_paths(*entry)) {
                            for (item& val : paths->u_container()) {
                                if (auto* path = val.strValue()) {
                                    auto accInfo = ca::access_constant(context.root(), path);
                                    if (accInfo) {
                                        ca::u_erase_key(accInfo->collection, accInfo->key);
                                    }
                                }
                            }
                        }
                    }

                    return true;
                };

                auto itr = boost::remove_if(pathRemoval->u_container(), eraseCondition);
                if (itr != pathRemoval->u_container().end()) {
                    pathRemoval->u_container().erase(itr, pathRemoval->u_container().end());
                }
            }
        }

        TEST(collections, autocleanup) {
            tes_context ctx;

            ca::assign_creative(ctx.root(), ".my.path", item{ "v" });
            ca::assign_creative(ctx.root(), ".my2.path", item{ "v" });
            EXPECT_EQ(*ca::get<std::string>(ctx.root(), ".my.path"), "v");

            setPathForAutoremoval(ctx, "FakePlugin.esp", ".my");
            u_autocleanup(ctx, [](const std::string& pluginName) { return pluginName != "FakePlugin.esp"; });

            EXPECT_FALSE(ca::get(ctx.root(), ".my.path"));
            EXPECT_FALSE(ca::get(ctx.root(), ".my"));

            EXPECT_TRUE(ca::get(ctx.root(), ".my2.path"));
            //===

            setPathForAutoremoval(ctx, "FakePlugin2.esp", ".my2.path");
            setPathForAutoremoval(ctx, "FakePlugin2.esp", ".my2.path2");

            u_autocleanup(ctx, [](const std::string& pluginName) { return pluginName != "FakePlugin2.esp"; });
            EXPECT_FALSE(ca::get(ctx.root(), ".my2.path"));
            EXPECT_FALSE(ca::get(ctx.root(), ".my2.path2"));
            EXPECT_TRUE(ca::get(ctx.root(), ".my2"));
        }

        TEST(collections, autocleanup_2) {
            tes_context ctx;

            ca::assign_creative(ctx.root(), ".my.path", item{ "v" });
            EXPECT_EQ(*ca::get<std::string>(ctx.root(), ".my.path"), "v");

            setPathForAutoremoval(ctx, "FakePlugin.esp", ".my");
            unsetPathForAutoremoval(ctx, "FakePlugin.esp", ".my");

            u_autocleanup(ctx, [](const std::string& pluginName) { return pluginName != "FakePlugin.esp"; });

            EXPECT_TRUE(ca::get(ctx.root(), ".my.path"));
            EXPECT_TRUE(ca::get(ctx.root(), ".my"));
        }
    }
}


