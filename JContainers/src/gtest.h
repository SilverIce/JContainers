#pragma once

#include "../dep/googletest/googletest/googletest/include/gtest/gtest.h"
//#include "meta.h"


#define EXPECT_NOT_NIL(expr) EXPECT_NE((expr), nullptr)
#define EXPECT_NIL(expr) EXPECT_EQ((expr), nullptr)


/*
namespace testing
{
    struct State;
    typedef void (*TestFn)(State&);
    struct TestInfo {
        TestFn function;
        const char* name;
        const char* name2;
    };

    inline TestInfo createInfo(TestFn function, const char* name, const char* name2) {
        TestInfo t = {function,name,name2};
        return t;
    }

    bool runTests(const meta<TestInfo>::list& list);
    void check(State& testState, bool, const char* source, const char* errorMessage);

    inline void check(State* testState, bool res, const char* source, const char* errorMessage) {
        check(*testState, res, source, errorMessage);
    }

    struct Fixture {
        State* testState;
        Fixture() : testState(nullptr) {}
    };

    template<class T>
    inline void FixtureTest(State& st) {
        T fixt;
        fixt.testState = &st;
        fixt.test();
    }

#   define TEST_F_CUSTOM_CLASS(FixtureClass, name2) \
        static const meta<::testing::TestInfo> testInfo_##FixtureClass##_##name2( \
            ::testing::createInfo(&::testing::FixtureTest<FixtureClass >, #FixtureClass, #name2));

#   define TEST_F(FixtureBase, name, name2) \
        struct Fixture_##name##_##name2 : public FixtureBase { \
            void test(); \
        }; \
        static const meta<::testing::TestInfo> testInfo_##name##_##name2( \
            ::testing::createInfo(&::testing::FixtureTest<Fixture_##name##_##name2 >, #name, #name2)); \
        \
        void Fixture_##name##_##name2::test()

#   define TEST_F_DISABLED(FixtureBase, name, name2) TEST_F(FixtureBase, name, DISABLED_##name2)


#   define TEST(name, name2) \
        void TESTFUNC_NAME(name,name2)(::testing::State&); \
        static const meta<::testing::TestInfo> testInfo_##name##_##name2( \
            ::testing::createInfo(&TESTFUNC_NAME(name,name2),#name,#name2)); \
        void TESTFUNC_NAME(name,name2)(::testing::State& testState)

#   ifdef TEST_COMPILATION_DISABLED
#       define TEST2(name, name2, ...) TEST2_NOT_COMPILED(name, name2, __VA_ARGS__)
#   else
#       define TEST2(name, name2, ...) \
            void TESTFUNC_NAME(name,name2)(::testing::State& testState) { __VA_ARGS__;} \
            static const meta<::testing::TestInfo> testInfo_##name##_##name2( \
                ::testing::createInfo(&TESTFUNC_NAME(name,name2),#name,#name2));
#   endif

#   define TESTFUNC_NAME(name, name2) TestFunc_##name##_##name2

#   define TEST2_NOT_COMPILED(name, name2, ...) TEST(name, NOT_COMPILED_##name2) {}

#   define TEST_DISABLED(name, name2) TEST(name, DISABLED_##name2)

#   define _LOCATION  __FUNCTION__ " line " STR(__LINE__)

#   define EXPECT_TRUE(expression) ::testing::check(testState, expression, _LOCATION, #expression " is false");
#   define EXPECT_FALSE(expression)  ::testing::check(testState, !(expression), _LOCATION, #expression " is true");
#   define EXPECT_EQ(a, b) ::testing::check(testState, (a) == (b), _LOCATION, #a " != " #b);

#   define EXPECT_NOT_NIL(expression) ::testing::check(testState, (expression) != nullptr, _LOCATION, #expression " is null ");
#   define EXPECT_NIL(expression) ::testing::check(testState, (expression) == nullptr, _LOCATION, #expression " is not null ");

#   define EXPECT_THROW(expression, exception) \
        try { \
            expression; \
            ::testing::check(testState, false, _LOCATION, "'" #expression "' does not throws nor '" #exception "' nor any other exception"); \
        } \
        catch( const exception& ) {;} \
        catch(...) { \
            ::testing::check(testState, false, _LOCATION,  "'" #expression "' does not throws '" #exception "' exception, but throws unknown exception"); \
            throw; \
        }

#   define EXPECT_NOTHROW(expression) \
        try { \
            expression; \
        } catch(...) { \
            ::testing::check(testState, false, _LOCATION, "'" #expression "' throws exception"); \
        }
}
*/
