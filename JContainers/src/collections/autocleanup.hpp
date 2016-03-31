#pragma once

#include <type_traits>

#include "util/string_normalize.h"
#include "util/cstring.h"
#include "skse/skse.h"

#include "collections/collections.h"
#include "collections/access.h"
#include "collections/context.h"

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

        void setPathForAutoremoval(tes_context& context, const char* pluginNameUnsafe, const char* path) {

            auto getPluginPaths = [](const std::string& plugin, tes_context& context)->array& {
                array* paths = nullptr;
                ca::visit_value(
                    context.root(),
                    (".__autocleanup.pathRemoval." + plugin + ".paths").c_str(),
                    ca::creative,
                    [&context, &paths](item& pathsValue) {
                        paths = pathsValue.object()->as<array>();
                        if (!paths) {
                            paths = &array::object(context);
                            pathsValue = paths;
                        }
                    }
                );
                assert(paths);
                return *paths;
            };

            auto insertPath = [](item&& path, array& paths) {
                object_lock g{ paths };
                if (std::find(paths.u_container().begin(), paths.u_container().end(), path) == paths.u_container().end()) {
                    paths.u_container().emplace_back(std::move(path));
                }
            };

            auto const pluginname = util::normalize_string(util::make_cstring(pluginNameUnsafe));

            insertPath(item{ path }, getPluginPaths(pluginname, context));

            ca::assign_creative(context.root(),
                (".__autocleanup.pathRemoval." + pluginname + ".pluginName").c_str(),
                item{ pluginNameUnsafe });
        }

        void unsetPathForAutoremoval(tes_context& context, const char* pluginNameUnsafe, const char* path) {

            auto getPluginPaths = [](const std::string& plugin, tes_context& context)->array* {
                return ca::get(context.root(),
                    (".__autocleanup.pathRemoval." + plugin + ".paths").c_str()
                ).get_value_or(item()).object()->as<array>();
            };

            auto removePath = [](const item& path, array* paths) -> bool {
                if (!paths) {
                    return true;
                }
                object_lock g{ paths };
                auto& cnt = paths->u_container();
                auto itr = std::remove(cnt.begin(), cnt.end(), path);
                if (itr != cnt.end()) {
                    cnt.erase(itr, cnt.end());
                }
                return cnt.empty();
            };

            auto const pluginname = util::normalize_string(util::make_cstring(pluginNameUnsafe));

            bool isLastItemRemoved = removePath(item{ path }, getPluginPaths(pluginname.c_str(), context));

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

            map* pathRemoval = ca::get(context.root(), ".__autocleanup.pathRemoval").get_value_or(item()).object()->as<map>();

            if (pathRemoval) {
                //DataHandler& dataHandler = *DataHandler::GetSingleton();
                auto eraseCondition = [&context, &isPluginLoaded](const map::value_type& p) -> bool {
                    if (auto entry = p.second.object()) {
                        std::string pluginName = ca::get<std::string>(*entry, ".pluginName").get_value_or(std::string());
                        if (isPluginLoaded(pluginName)) {
                            return false; // do not erase entry
                        }

                        if (array* paths = ca::get(*entry, ".paths").get_value_or(item()).object()->as<array>()) {
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

                util::tree_erase_if(pathRemoval->u_container(), eraseCondition);
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


