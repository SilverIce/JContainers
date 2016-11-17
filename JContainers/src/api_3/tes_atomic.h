
namespace tes_api_3 {

    using namespace collections;

    class tes_atomic : public class_meta< tes_atomic > {
    public:

        REGISTER_TES_NAME("JAtomic");

        void additionalSetup() {
            metaInfo.comment =
                ""
                "\nThis way you can even, probably, implement true locks and etc";
        }

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
                [&](item& item_value) {
                    if (item_value.isNull()) {
                        item_value = func(initialValue, inputValue);
                    }
                    else if (internal_item_type *asT = item_value.get<internal_item_type>()) {
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
        struct compare_exchange_func {
            T& newValue;

            T& operator()(T& oldValue, T& comparer) const {
                if (d != comparer)
                    return newValue;
                return oldValue;
            }
        };

        template<class T>
        static T exchange(
            tes_context& ctx, object_base* obj, const char* path,
            T inputValue, bool createMissingKeys, T onError)
        {
            return performAtomicFunction_(ctx, obj, path, ignore_first{}, inputValue, inputValue, createMissingKeys, onError);
        }

        template<class T>
        static T compareExchange(
            tes_context& ctx, object_base* obj, const char* path,
            T newValue, T comparer, bool createMissingKeys, T onError)
        {
            if (!obj || !path)
                return onError;

            T previousVal = default_value<T>();
            bool succeed = ca::visit_value(
                *obj, path,
                createMissingKeys ? ca::creative : ca::constant,
                [&](item& itemValue) {
                    if (itemValue == comparer) {

                        if (auto* valuePtr = itemValue.get<T>()) {
                            previousVal = std::move(*valuePtr);
                        }

                        itemValue = std::move(newValue);
                    } else {

                        if (const auto* valuePtr = itemValue.get<T>()) {
                            previousVal = *valuePtr;
                        }

                    }
                });

            return succeed ? previousVal : onError;
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
"A group of the functions that perform various math on the value at the @path of the container. Returns previos value:\n\
\n\
    T previousValue = container.path\n\
    container.path = someMathFunction(container.path, value)\n\
    return previousValue\n\
\n\
If the value at the @path is None, then the @initialValue being read and passed into math function instead of None.\n\
If @createMissingKeys is True, the function attemps to create missing @path elements."
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
        REGISTERF(exchange<SInt32>, "exchangeInt", PARAMS_INT "0",
"Exchanges the value at the @path with the @value. Returns previous value.");

        REGISTERF(exchange<Float32>, "exchangeFlt", PARAMS_INT "0.0", nullptr);
        REGISTERF(exchange<std::string>, "exchangeStr", PARAMS_INT "\"\"", nullptr);

        REGISTERF(exchange<form_ref>, "exchangeForm", PARAMS_INT "None", nullptr);
        REGISTERF(exchange<object_base*>, "exchangeObj", PARAMS_INT "0", nullptr);
#   undef PARAMS_INT

#   define PARAMS_INT   "object path value comparand createMissingKeys=false onErrorReturn="
        REGISTERF(compareExchange<SInt32>, "compareExchangeInt", PARAMS_INT "0",
"Compares the value at the @path with the @comparand and, if they are equal, exchanges the value at the @path with the @value. Returns previous value.");

        REGISTERF(compareExchange<Float32>, "compareExchangeFlt", PARAMS_INT "0.0", nullptr);
        //REGISTERF(compareExchange<std::string>, "compareExchangeStr", PARAMS_INT "\"\"", nullptr);

        REGISTERF(compareExchange<form_ref>, "compareExchangeForm", PARAMS_INT "None", nullptr);
        REGISTERF(compareExchange<object_base*>, "compareExchangeObj", PARAMS_INT "0", nullptr);
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
