
namespace reflection {  namespace binding {

    template<>
        function_parameter typeInfo<void>() { return function_parameter_make("void", nullptr); }

    template<>
        function_parameter typeInfo<bool>() { return function_parameter_make("bool", nullptr); }

    template<>
        function_parameter typeInfo<skse::string_ref>() { return function_parameter_make("string", nullptr); }

    template<> 
        function_parameter typeInfo< const char* >() { return function_parameter_make("string", nullptr); }

    template<>
        function_parameter typeInfo< Float32 >() { return function_parameter_make("float", nullptr); }
    template<> 
        function_parameter typeInfo< SInt32 >() { return function_parameter_make("int", nullptr); }
    template<>
        function_parameter typeInfo< UInt32 >() { return function_parameter_make("int", nullptr); }
    template<>
        function_parameter typeInfo< TESForm * >() { return function_parameter_make("form", nullptr); }
    template<>
        function_parameter typeInfo< BGSListForm * >() { return function_parameter_make("FormList", nullptr); }
}
}
