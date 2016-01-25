#pragma once

#include "util/stl_ext.h"
#include "reflection/tes_binding.h"
#include "collections/collections.h"
#include "collections/context.h"

namespace reflection { namespace binding {

    using namespace collections;

    template<class T = object_base>
    struct ObjectConverter {

        typedef HandleT tes_type;

        static HandleT convert2Tes(object_base* obj) {
            return (HandleT)(obj ? obj->uid() : Handle::Null);
        }

        static object_stack_ref_template<T> convert2J(HandleT hdl) {
            auto ref = tes_context::instance().getObjectRefOfType<T>((Handle)hdl);
            if (!ref && hdl != util::to_integral(Handle::Null)) {
                JC_log("Warning: access to non-existing object with id 0x%X", hdl);
            }
            return ref;
        }
    };

    template<> struct GetConv < object_stack_ref > : ObjectConverter<>{};

    template<> struct GetConv < object_base* > : ObjectConverter<>{};
    template<> struct GetConv < array* > : ObjectConverter< array >{};
    template<> struct GetConv < map* > : ObjectConverter< map >{};
    template<> struct GetConv < form_map* > : ObjectConverter< form_map >{};
    template<> struct GetConv < integer_map* > : ObjectConverter < integer_map >{};

    //////////////////////////////////////////////////////////////////////////

    template<> struct GetConv < Handle > : StaticCastValueConverter<Handle, HandleT> {};

    //////////////////////////////////////////////////////////////////////////

    template<> struct GetConv < FormId > {
        typedef TESForm* tes_type;
        static TESForm* convert2Tes(FormId id) {
            return LookupFormByID((uint32_t)id);
        }
        static FormId convert2J(const TESForm* form) {
            return form ? (FormId)form->formID : FormId::Zero;
        }
    };

    /////////////////

    template<> struct GetConv < form_watching::form_ref > {
        typedef TESForm* tes_type;
        static TESForm* convert2Tes(const form_ref& id) {
            return LookupFormByID((uint32_t)id.get());
        }
        static form_watching::form_ref convert2J(const TESForm* form) {
            return make_weak_form_id(form, tes_context::instance());
        }
    };
}
}
