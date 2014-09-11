#pragma once

namespace collections
{
    class garbage_collector
    {
    public:

        typedef std::deque<object_base *> object_list;
        typedef std::hash_set<object_base *> object_set;

        static void u_collect(object_registry& registry, autorelease_queue& aqueue, const object_list& root_objects) {

            auto findRootObjects = [&registry, &aqueue, &root_objects]() -> object_set {
                object_set roots(root_objects.begin(), root_objects.end());

                for (auto& obj : registry.u_container()) {
                    if (obj->u_is_user_retains()) {
                        roots.insert(obj);
                    }
                }

                for (auto& pair : aqueue.u_queue()) {
                    roots.insert(pair.first.get());
                }

                return roots;
            };

            // all-objects minus reachable-objects
            auto findNonReachable = [&registry](const object_set& root_objects) -> object_set {

                object_list objects_to_visit(root_objects.begin(), root_objects.end());
                object_set not_visited_yet = registry.u_container();

                std::function<void(object_base&)> visitor = [&objects_to_visit, &not_visited_yet](object_base& referenced) {

                    auto itr = not_visited_yet.find(&referenced);
                    if (itr != not_visited_yet.end()) {
                        not_visited_yet.erase(itr);

                        objects_to_visit.push_back(&referenced);
                    }
                };

                while (true) {

                    object_list to_visit;
                    to_visit.swap(objects_to_visit);

                    for (auto& obj : to_visit) {
                        obj->u_visit_referenced_objects(visitor);
                    }
                }

                return not_visited_yet;
            };

            auto collectGarbage = [](const object_set& garbage) {
                for (auto& obj : garbage) {
                    obj->u_clear();
                }

                // ref. count in unreachable graphs reaches zero -> all objects are moved into aqueue
            };

            collectGarbage(findNonReachable(findRootObjects()));
        }

    };
}
