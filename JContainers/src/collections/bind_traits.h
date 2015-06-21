#pragma once

#include "reflection/tes_binding.h"
#include "collections/collections.h"
#include "collections/context.h"

namespace reflection { namespace binding {

    using namespace collections;

    struct ObjectConverter {

        typedef HandleT tes_type;

        static HandleT convert2Tes(object_base* obj) {
            return obj ? obj->uid() : 0;
        }

        static object_stack_ref convert2J(HandleT hdl) {
            return tes_context::instance().getObjectRef((Handle)hdl);
        }

    };

    template<class T, class P> struct GetConv < boost::intrusive_ptr_jc<T, P>& > : ObjectConverter{
        static boost::intrusive_ptr_jc<T, P> convert2J(HandleT hdl) {
            return tes_context::instance().getObjectRefOfType<T>((Handle)hdl);
        }
    };

    template<class T>
    struct ObjectConverterT {

        typedef HandleT tes_type;

        static HandleT convert2Tes(object_base* obj) {
            return obj ? obj->uid() : 0;
        }

        static typename T::ref convert2J(HandleT hdl) {
            return tes_context::instance().getObjectRefOfType<T>((Handle)hdl);
        }
    };

    template<> struct GetConv < object_stack_ref& > : ObjectConverter{};
    //template<> struct GetConv < object_stack_ref > : ObjectConverter{};

    template<> struct GetConv < object_base* > : ObjectConverter{};
    template<> struct GetConv < array* > : ObjectConverterT< array >{};
    template<> struct GetConv < map* > : ObjectConverterT< map >{};
    template<> struct GetConv < form_map* > : ObjectConverterT< form_map >{};
    template<> struct GetConv < integer_map* > : ObjectConverterT < integer_map >{};

    //////////////////////////////////////////////////////////////////////////

    template<> struct GetConv < Handle > {
        typedef HandleT tes_type;

        static HandleT convert2Tes(Handle obj) {
            return obj;
        }
        static Handle convert2J(HandleT hdl) {
            return static_cast<Handle>(hdl);
        }
    };

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
