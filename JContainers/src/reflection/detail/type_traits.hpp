
namespace reflection {  namespace binding {

    template<>
        function_parameter type_info<void>() { return function_parameter_make("void", nullptr); }

    template<>
        function_parameter type_info<bool>() { return function_parameter_make("bool", nullptr); }

    template<>
        function_parameter type_info<skse::string_ref>() { return function_parameter_make("string", nullptr); }

    template<> 
        function_parameter type_info< const char* >() { return function_parameter_make("string", nullptr); }

    template<>
        function_parameter type_info< Float32 >() { return function_parameter_make("float", nullptr); }
    template<> 
        function_parameter type_info< SInt32 >() { return function_parameter_make("int", nullptr); }
    template<>
        function_parameter type_info< UInt32 >() { return function_parameter_make("int", "object"); }
    template<>
        function_parameter type_info< TESForm * >() { return function_parameter_make("form", nullptr); }
    template<>
        function_parameter type_info< BGSListForm * >() { return function_parameter_make("FormList", nullptr); }
}
}
