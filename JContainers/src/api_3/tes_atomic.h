
namespace tes_api_3 {

    using namespace collections;

    class tes_atomic : public class_meta< tes_atomic > {
    public:

        REGISTER_TES_NAME("JAtomic");

        void additionalSetup() {
            metaInfo.comment = "";
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
            tes_context& ctx, object_base* obj, const char* path, F&& func, const T& inputValue,
            const T& initialValue, bool createMissingKeys, const T& onError)
        {
            if (!obj || !path)
                return onError;

            T previousVal = default_value<T>();
            bool assing_succeed = true;

            using internal_item_type = typename item::user2variant_t<T>;

            bool succeed = ca::visit_value(
                *obj, path,
                createMissingKeys ? ca::creative : ca::constant,
                [&](item& value) {
                    if (value.isNull()) {
                        value = initialValue;
                    }
                     
                    if (internal_item_type *asT = value.get<internal_item_type>()) {
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
            tes_context& ctx, object_base* obj, const char* path,
            T inputValue, bool createMissingKeys, T onError)
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

#   define PARAMS_INT   "object path value initialValue=0 createMissingKeys=false onErrorReturn=0"
#   define PARAMS_FLT   "object path value initialValue=0.0 createMissingKeys=false onErrorReturn=0.0"

        REGISTERF(ARGS(performAtomicFunction<SInt32, std::plus<SInt32>>), "fetchAddInt", PARAMS_INT,
"Performs:\n\
    T previous = value.at.path\n\
    value.at.path = value.at.path + value\n\
    return previous"
);
        REGISTERF(ARGS(performAtomicFunction<Float32, std::plus<Float32>>), "fetchAddFlt", PARAMS_FLT, nullptr);

        REGISTERF(ARGS(performAtomicFunction<SInt32, std::multiplies<SInt32>>), "fetchMultInt", PARAMS_INT, "x *= v");
        REGISTERF(ARGS(performAtomicFunction<Float32, std::multiplies<Float32>>), "fetchMultFlt", PARAMS_FLT, nullptr);

        REGISTERF(ARGS(performAtomicFunction<int32_t, std::modulus<int32_t>>), "fetchModInt", PARAMS_INT, "x %= v");

        REGISTERF(ARGS(performAtomicFunction<SInt32, std::divides<SInt32>>), "fetchDivInt", PARAMS_INT, "x /= v");
        REGISTERF(ARGS(performAtomicFunction<Float32, std::divides<Float32>>), "fetchDivFlt", PARAMS_FLT, nullptr);

        REGISTERF(ARGS(performAtomicFunction<uint32_t, std::bit_and<uint32_t>>), "fetchAndInt", PARAMS_INT, "x &= v");
        REGISTERF(ARGS(performAtomicFunction<uint32_t, std::bit_xor<uint32_t>>), "fetchXorInt", PARAMS_INT, "x ^= v");
        REGISTERF(ARGS(performAtomicFunction<uint32_t, std::bit_or<uint32_t>>), "fetchOrInt", PARAMS_INT, "x |= v");

#   undef PARAMS_INT
#   undef PARAMS_FLT

#   define PARAMS_INT   "object path value createMissingKeys=false onErrorReturn="
        REGISTERF(exchange<SInt32>, "exchangeInt", PARAMS_INT "0", "u");
        REGISTERF(exchange<Float32>, "exchangeFlt", PARAMS_INT "0.0", nullptr);
        REGISTERF(exchange<std::string>, "exchangeStr", PARAMS_INT "\"\"", nullptr);

        REGISTERF(exchange<form_ref>, "exchangeForm", PARAMS_INT "None", nullptr);
        REGISTERF(exchange<object_base*>, "exchangeObj", PARAMS_INT "0", nullptr);
#   undef PARAMS_INT

    };

    TES_META_INFO(tes_atomic);

    namespace tes_atomic_testing {

        template<class T>
        struct input
        {
            T on_error, expect_return, expect_new_value, initial_value, value;
        };

        template<class T, class F>
        inline void test_func(
            tes_context& ctx, object_base& obj, const input<T>& value)
        {
            const auto path = "[0]";

            boost::optional<item> previousValue = ca::get(obj, path);

            T result = tes_atomic::performAtomicFunction_(
                ctx, &obj, path, F{}, value.value, value.initial_value, false, value.on_error);

            using internal_item_type = item::user2variant_t<T>;

            EXPECT_EQ(result, value.expect_return);

            boost::optional<item> newValue = ca::get(obj, path);
            EXPECT_NE(newValue, boost::none);
            EXPECT_NOT_NIL(newValue->get<internal_item_type>());
            EXPECT_EQ(value.expect_new_value, *newValue->get<internal_item_type>());

            if (previousValue && previousValue->get<internal_item_type>()
                && newValue && newValue->get<internal_item_type>())
            {
                T functorResult = F{}(*previousValue->get<internal_item_type>(), value.value);
                EXPECT_EQ(*newValue->get<internal_item_type>(), functorResult);
            }
        }

        TEST(tes_atomic, _)
        {
            tes_context_standalone context;
            array& obj = array::make(context);
            obj.u_container().resize(4);

            input<int32_t> inp;
            inp.expect_new_value = 1;
            inp.expect_return = 0;
            inp.value = 1;
            inp.initial_value = 0;

            test_func<int32_t, std::plus<int32_t>>(context, obj, inp);

            inp.expect_new_value = 2;
            inp.expect_return = 1;
            inp.value = 1;
            inp.initial_value = 0;

            test_func<int32_t, std::plus<int32_t>>(context, obj, inp);

            auto ss = default_value<std::string>();

            EXPECT_EQ("", tes_atomic::exchange<std::string>(context, &obj, "[1]", "new-str", false, ""));

    /*
            inp.expect_new_value = 2;
            inp.expect_return = 1;
            inp.value = 1;
            inp.initial_value = 0;

            test_func<int32_t, std::plus<int32_t>>(context, obj, inp);
    */

        }
    }
}
