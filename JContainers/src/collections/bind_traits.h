#pragma once

#include "reflection/tes_binding.h"
#include "collections/collections.h"
#include "collections/context.h"

namespace reflection { namespace binding {

    using namespace collections;

    template<class T = object_base>
    struct ObjectConverter {

        typedef HandleT tes_type;

        static HandleT convert2Tes(object_base* obj) {
            return obj ? obj->uid() : 0;
        }

        static object_stack_ref_template<T> convert2J(HandleT hdl) {
            return tes_context::instance().getObjectRefOfType<T>((Handle)hdl);
        }
    };

    template<> struct GetConv < object_stack_ref& > : ObjectConverter<>{};

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
            return LookupFormByID(id);
        }
        static FormId convert2J(const TESForm* form) {
            return form ? (FormId)form->formID : FormZero;
        }
    };

    template<> struct j2Str < FormId > : j2Str < TESForm* > {};


    //////////////////////////////////////////////////////////////////////////

    template<class T, class P> struct j2Str < boost::intrusive_ptr_jc<T, P> > {
        static function_parameter typeInfo() { return j2Str<T*>::typeInfo(); }
    };

    template<class T, class P> struct j2Str < boost::intrusive_ptr_jc<T, P>& > {
        static function_parameter typeInfo() { return j2Str<T*>::typeInfo(); }
    };

    struct jc_object_type_info {
        static function_parameter typeInfo() { return function_parameter_make("int", "object"); }
    };

    template<> struct j2Str < object_base * > : jc_object_type_info{};
    template<> struct j2Str < map * > : jc_object_type_info{};
    template<> struct j2Str < array * > : jc_object_type_info{};
    template<> struct j2Str < form_map * > : jc_object_type_info{};
    template<> struct j2Str < integer_map * > : jc_object_type_info{};
    template<> struct j2Str < Handle > : jc_object_type_info{};
}
}
