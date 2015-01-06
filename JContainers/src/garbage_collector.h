#pragma once

namespace collections
{
    class garbage_collector
    {
    public:

        typedef std::deque<object_base* > object_list;
        typedef std::hash_set<object_base *> object_set;

        static size_t u_collect(object_registry& registry, autorelease_queue& aqueue, const std::deque<std::reference_wrapper<object_base>>& root_objects) {

            auto findRootObjects = [&registry, &aqueue, &root_objects]() -> object_set {
                object_set roots;// (root_objects.begin(), root_objects.end());
                std::transform(root_objects.begin(), root_objects.end(), std::inserter(roots, roots.begin()), [](object_base& obj) {
                    return &obj;
                });

                for (auto& obj : registry.u_container()) {
                    if (obj->u_is_user_retains()) {
                        roots.insert(obj);
                    }
                }

                for (auto& ref : aqueue.u_queue()) {
                    roots.insert(ref.get());
                }

                return roots;
            };

            // all-objects minus reachable-objects
            auto findNonReachable = [&registry](const object_set& root_objects) -> object_set {

                object_list objects_to_visit(root_objects.begin(), root_objects.end());
                object_set not_reachable = registry.u_container(); // potentially huge op ?

                for (auto& root : root_objects) {
                    not_reachable.erase(root);
                }

                std::function<void(object_base&)> visitor = [&objects_to_visit, &not_reachable](object_base& referenced) {

                    auto itr = not_reachable.find(&referenced);
                    if (itr != not_reachable.end()) {
                        // will succeed in most cases
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


            auto collectGarbage = [&registry](const object_set& garbage) -> size_t {
                for (auto& obj : garbage) {
                    if (obj->noOwners() == false) { // an object is part of a graph
                        // the object's ref. count in the unreachable graphs reaches zero -> all objects are moved into aqueue
                        obj->u_clear();
                    }
                    else {
                        obj->_delete_self();
                    }
                }
                
                return garbage.size();
            };

            return collectGarbage(findNonReachable(findRootObjects()));
        }

    };
}
