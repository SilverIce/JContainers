#pragma once

#include "typedefs.h"
#include "meta.h"

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

#   define TEST_FIXTURE(FixtureType) \
        TEST(fixture, FixtureType) { \
            FixtureType mix; \
            fix.testing::fixture::teststate = teststate; \
            fix.FixtureType::test(); \
        }

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

#   define TEST_REGISTER(function) \
        TEST(function, none) { extern void function(::testing::State&); function(testState); }

#   define EXPECT_TRUE(expression) ::testing::check(testState, expression, __FUNCTION__, #expression " is false");
#   define EXPECT_FALSE(expression)  ::testing::check(testState, !(expression), __FUNCTION__, #expression " is true");
#   define EXPECT_EQ(a, b) ::testing::check(testState, (a) == (b), __FUNCTION__, #a " != " #b);

#   define EXPECT_THROW(expression, exception) \
        try { \
            expression; \
            ::testing::check(testState, false, __FUNCTION__, "'" #expression "' does not throws nor '" #exception "' nor any other exception"); \
        } \
        catch( const exception& ) {;} \
        catch(...) { \
            ::testing::check(testState, false, __FUNCTION__,  "'" #expression "' does not throws '" #exception "' exception, but throws unknown exception"); \
            throw; \
        }

#   define EXPECT_NOTHROW(expression) \
        try { \
            expression; \
        } catch(...) { \
            ::testing::check(testState, false, __FUNCTION__, "'" #expression "' throws exception"); \
        }
}
