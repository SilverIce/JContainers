
namespace tes_api_3 {

    using namespace collections;

    class tes_atomic : public class_meta< tes_atomic > {
    public:

        typedef object_base* ref;

        REGISTER_TES_NAME("JAtomic");

        void additionalSetup() {
            metaInfo.comment = "uu";
        }

        
/*
Int function atomicFetchAdd(int object, string path, int value, bool createMissingKeys=false, int initialValue=0, int onErrorReturn=0) Global Native

Int function atomicFetchAnd(int object, string path, int value, bool createMissingKeys=false, int initialValue=0, int onErrorReturn=0) Global Native
Int function atomicFetchXOR(int object, string path, int value, bool createMissingKeys=false, int initialValue=0, int onErrorReturn=0) Global Native
Int function atomicFetchOr(int object, string path, int value, bool createMissingKeys=false, int initialValue=0, int onErrorReturn=0) Global Native
Int function atomicFetchMul(int object, string path, int value, bool createMissingKeys=false, int initialValue=0, int onErrorReturn=0) Global Native
Int function atomicFetchDiv(int object, string path, int value, bool createMissingKeys=false, int initialValue=0, int onErrorReturn=0) Global Native
*/
        template<class T, class F>
        static T performAtomicFunction_(
            tes_context& ctx, object_base* obj, const char* path, F&& func, T inputValue,
            T initialValue, bool createMissingKeys, T onError)
        {
            if (!obj || !path)
                return onError;

            T previousVal = default_value<T>();
            bool assing_succeed = true;

            using internal_item_type = typename item::_user2variant<T>::variant_type;

            bool succeed = ca::visit_value(
                *obj, path,
                createMissingKeys ? ca::creative : ca::constant,
                [&previousVal, &inputValue, &func, &initialValue, &assing_succeed](item& value) {
                    if (value.isNull()) {
                        value = initialValue;
                    }
                    else if (internal_item_type *asT = value.get<internal_item_type>()) {
                        previousVal = const_cast<const internal_item_type&>(*asT);
                        *asT = func(const_cast<const internal_item_type&>(*asT), inputValue);
                    }
                    else {
                        assing_succeed = false;
                    }
                });

            return (succeed && assing_succeed) ? previousVal : onError;
        }

        struct ignore_first {
            template<class T, class D>
            T&& operator()(const D&, T&& newValue) const {
                return std::forward<T>(newValue);
            }
        };

        template<class T>
        static T exchange(
            tes_context& ctx, object_base* obj, const char* path, T inputValue, bool createMissingKeys, T onError)
        {
            return performAtomicFunction_(ctx, obj, path, ignore_first{}, inputValue, inputValue, createMissingKeys, onError);
        }

        template<class T, class F>
        static T performAtomicFunction(
            tes_context& ctx, object_base* obj, const char* path, T inputValue,
            T initialValue, bool createMissingKeys, T onError)
        {
            return performAtomicFunction_(ctx, obj, path, F{}, inputValue, initialValue, createMissingKeys, onError);
        }

#   define PARAMSSSS   "object path value initialValue=0 createMissingKeys=false onErrorReturn=0"

        REGISTERF(ARGS(performAtomicFunction<SInt32, std::plus<SInt32>>), "fetchAddInt", PARAMSSSS,
"Performs:\n\
    T previous = value.at.path\n\
    value.at.path = value.at.path + value\n\
    return previous"
);
        REGISTERF(ARGS(performAtomicFunction<Float32, std::plus<Float32>>), "fetchAddFlt", PARAMSSSS, nullptr);

        REGISTERF(ARGS(performAtomicFunction<SInt32, std::multiplies<SInt32>>), "fetchMultInt", PARAMSSSS, "x *= v");
        REGISTERF(ARGS(performAtomicFunction<Float32, std::multiplies<Float32>>), "fetchMultFlt", PARAMSSSS, nullptr);

        REGISTERF(ARGS(performAtomicFunction<uint32_t, std::modulus<int32_t>>), "fetchModInt", PARAMSSSS, "x %= v");

        REGISTERF(ARGS(performAtomicFunction<SInt32, std::divides<SInt32>>), "fetchDivInt", PARAMSSSS, "x /= v");
        REGISTERF(ARGS(performAtomicFunction<Float32, std::divides<Float32>>), "fetchDivFlt", PARAMSSSS, nullptr);

        REGISTERF(ARGS(performAtomicFunction<uint32_t, std::bit_and<uint32_t>>), "fetchAndInt", PARAMSSSS, "x &= v");
        REGISTERF(ARGS(performAtomicFunction<uint32_t, std::bit_xor<uint32_t>>), "fetchXorInt", PARAMSSSS, "x ^= v");
        REGISTERF(ARGS(performAtomicFunction<uint32_t, std::bit_or<uint32_t>>), "fetchOrInt", PARAMSSSS, "x |= v");

#   undef PARAMSSSS


#   define PARAMSSSS   "object path value createMissingKeys=false onErrorReturn=0"

        REGISTERF(exchange<SInt32>, "exchangeInt", PARAMSSSS, "u");
        REGISTERF(exchange<Float32>, "exchangeFlt", PARAMSSSS, nullptr);
        REGISTERF(exchange<std::string>, "exchangeStr", PARAMSSSS, nullptr);

        REGISTERF(exchange<form_ref>, "exchangeForm", PARAMSSSS, nullptr);
        REGISTERF(exchange<object_base*>, "exchangeObj", PARAMSSSS, nullptr);

#   undef PARAMSSSS
    };

    TES_META_INFO(tes_atomic);
}
