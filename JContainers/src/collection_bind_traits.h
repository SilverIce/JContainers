#pragma once

#include "tes_binding.h"
#include "collections.h"

namespace reflection { namespace binding {

    using namespace collections;

    template<>
    struct J2Tes < object_base* > {
        typedef HandleT tes_type;
    };
    template<> inline HandleT convert2Tes(object_base* obj) {
        return obj ? obj->uid() : 0;
    }
    template<> inline HandleT convert2Tes(array* obj) {
        return obj ? obj->uid() : 0;
    }
    template<> inline HandleT convert2Tes(map* obj) {
        return obj ? obj->uid() : 0;
    }
    template<> inline HandleT convert2Tes(form_map* obj) {
        return obj ? obj->uid() : 0;
    }

    template<>
    struct Tes2J < HandleT > {
        typedef object_base* j_type;
    };

    template<> struct J2Tes < array* > {
        typedef HandleT tes_type;
    };
    template<> struct J2Tes < map* > {
        typedef HandleT tes_type;
    };
    template<> struct J2Tes < form_map* > {
        typedef HandleT tes_type;
    };
    template<> inline object_base* convert2J(HandleT hdl) {
        return tes_context::instance().getObject(hdl);
    }
    template<> inline array* convert2J(HandleT hdl) {
        return tes_context::instance().getObjectOfType<array>(hdl);
    }
    template<> inline map* convert2J(HandleT hdl) {
        return tes_context::instance().getObjectOfType<map>(hdl);
    }
    template<> inline form_map* convert2J(HandleT hdl) {
        return tes_context::instance().getObjectOfType<form_map>(hdl);
    }
    template<>
    struct J2Tes < Handle > {
        typedef HandleT tes_type;
    };
    template<> inline HandleT convert2Tes(Handle hdl) {
        return (HandleT)hdl;
    }

    template<> struct j2Str < object_base * > {
        static function_parameter typeInfo() { return function_parameter_make("int", "object"); }
    };
    template<> struct j2Str < map * > {
        static function_parameter typeInfo() { return function_parameter_make("int", "object"); }
    };
    template<> struct j2Str < array * > {
        static function_parameter typeInfo() { return function_parameter_make("int", "object"); }
    };
    template<> struct j2Str < form_map * > {
        static function_parameter typeInfo() { return function_parameter_make("int", "object"); }
    };
    template<> struct j2Str < HandleT > {
        static function_parameter typeInfo() { return function_parameter_make("int", "object"); }
    };
    template<> struct j2Str < Handle > {
        static function_parameter typeInfo() { return function_parameter_make("int", "object"); }
    };

}
}