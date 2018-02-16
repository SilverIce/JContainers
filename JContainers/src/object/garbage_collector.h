#pragma once

namespace collections
{
    class garbage_collector
    {
    public:

        typedef std::deque<object_base* > object_list;
        typedef std::unordered_set<object_base *> object_set;

        struct result
        {
            size_t garbage_total; // 
            size_t part_of_graphs; //
            size_t root_count;
        };

        static result u_collect(object_registry& registry, autorelease_queue& aqueue) {

            auto findRootObjects = [&registry, &aqueue]() -> object_list {
                object_list roots;// (root_objects.begin(), root_objects.end());

                for (auto& obj : registry.u_all_objects()) {
                    // stack ref. count not taken into account as this ref.count is not persistent
                    if (obj->u_is_user_retains() || obj->is_in_aqueue()) {
                        roots.push_back(obj);
                    }
                }

                return roots;
            };

            // all-objects minus reachable-objects
            auto findNonReachable = [&registry](const object_list& root_objects) -> object_set {

                object_list objects_to_visit(root_objects);
                object_set not_reachable = registry.u_all_objects(); // potentially huge op ?

                for (auto& root : root_objects) {
                    not_reachable.erase(root);
                }

                std::function<void(object_base&)> visitor = [&objects_to_visit, &not_reachable](object_base& referenced) {

                    auto itr = not_reachable.find(&referenced);
                    // is in not_reachable set? then is wasn't visited yet
                    if (itr != not_reachable.end()) {
                        not_reachable.erase(itr);
                        objects_to_visit.push_back(&referenced);
                    }
                };

                object_list to_visit_temp;

                while (!objects_to_visit.empty()) {

                    to_visit_temp.clear();
                    to_visit_temp.swap(objects_to_visit);

                    for (auto& obj : to_visit_temp) {
                        obj->u_visit_referenced_objects(visitor);
                    }
                }

                return not_reachable;
            };

            // all-objects minus reachable-objects
/*
            auto findNonReachable2 = [&registry](const object_set& root_objects) -> object_set {

                object_list objects_to_visit(root_objects.begin(), root_objects.end());
                std::sort(objects_to_visit.begin(), objects_to_visit.end());

                object_list not_reachable(registry.u_container().begin(), registry.u_container().end()); // potentially huge op ?
                std::sort(not_reachable.begin(), not_reachable.end());

                object_list reachable;

                std::function<void(object_base&)> visitor = [&objects_to_visit, &not_reachable](object_base& referenced) {
                    objects_to_visit.push_back(&referenced);
                };

                object_list to_visit_temp;

                while (!objects_to_visit.empty()) {

                    to_visit_temp.clear();
                    to_visit_temp.swap(objects_to_visit);

                    std::setdiff
                    not_reachable -= objects_to_visit;

                    for (auto& obj : to_visit_temp) {
                        obj->u_visit_referenced_objects(visitor);
                    }
                }

                return not_reachable;
            };*/


            auto collectGarbage = [&registry](const object_set& garbage) -> result {
                size_t part_of_graphs = 0;

                for (auto& obj : garbage) {
                    if (obj->noOwners() == false) { // an object is part of a graph
                        // the object's ref. count in the unreachable graphs reaches zero -> all objects are moved into aqueue
                        obj->u_clear();
                        ++part_of_graphs;
                    }
                    else {
                        obj->_delete_self();
                    }
                }
                
                return result{ garbage.size(), part_of_graphs };
            };

            return collectGarbage(findNonReachable(findRootObjects()));
        }

    };
}
