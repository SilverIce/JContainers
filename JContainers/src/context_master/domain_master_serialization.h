#pragma once

#include "boost\serialization\split_free.hpp"
#include "boost\serialization\version.hpp"
//#include "boost\serialization\optional.hpp"

#include "util/istring_serialization.h"

#include "context_master/domain_master.h"

namespace boost {
    namespace serialization {

        template<class Archive>
        void save(Archive & arch, const ::domain_master::master & self, unsigned int version) {
            arch << self.get_default_domain();

            uint32_t domain_count = self.active_domains_map().size();
            arch << domain_count;

            for (auto& pair : self.active_domains_map()) {
                util::istring dom_name;
                arch << pair.first;
                arch << *pair.second;
            }

            arch << self.get_form_observer();
        }

        template<class Archive>
        void load(Archive & archive, ::domain_master::master & self, unsigned int version) {

            archive >> self.get_default_domain();
            uint32_t domain_count = 0;
            archive >> domain_count;

            for (uint32_t i = 0; i < domain_count; ++i) {
                util::istring dom_name;
                archive >> dom_name;
                auto& dom = self.get_or_create_domain_with_name(dom_name);
                archive >> dom;
            }

            archive >> self.get_form_observer();
        }

    }
}

BOOST_SERIALIZATION_SPLIT_FREE(domain_master::master);
//BOOST_CLASS_VERSION(domain_master::master, 1);
