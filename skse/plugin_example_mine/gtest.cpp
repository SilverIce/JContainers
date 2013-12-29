#include "gtest.h"
#include "typedefs_p.h"
#include <stdio.h>
#include <intrin.h>
#include <string>
#include <exception>

namespace testing
{
    static bool IsDisabled(const TestInfo& info) {
        const char * text = "DISABLED";
        return strncmp(info.name2, text, strlen(text)) == 0;
    }

    struct Statistics
    {
        //int countTestsFailed;
        int countChecksFailed;
        int countDisabledTests;
        int countFailedTests;
        int countTotalTests;

        explicit Statistics() {
            memset(this, 0, sizeof(*this));
        }

        void OnTestsComplete() {
            printf("\n");
            printf("%d tests failed\n", countFailedTests);
            printf("%d tests disabled\n", countDisabledTests);
            printf("%d tests total amount\n", countTotalTests);
        }
    };

    struct State : public Statistics {
        bool currentFailed;
    };

    bool runTests(const meta<TestInfo>::list& list)
    {
        State state;
        const meta<TestInfo> * first = list.first;
        while(first) {
            const TestInfo& test = first->info;
            first = first->next;

            state.currentFailed = false;
            ++state.countTotalTests;
            if (IsDisabled(test)) {
                printf("    %s::%s is disabled\n", test.name, test.name2);
                ++state.countDisabledTests;
                continue;
            }

            printf("    %s::%s has been invoked\n", test.name, test.name2);
            try {
                test.function(state);
            } catch(const std::exception& exception) {
                ::testing::check(state, false, __FUNCTION__, "test throws exception"); \
                printf("   of type '%s' message '%s'\n", typeid(exception).name(), exception.what());
            } catch(...) {
                ::testing::check(state, false, __FUNCTION__, "test throws exception"); \
            }

            if (state.currentFailed)
                printf("    %s::%s has been failed!\n", test.name, test.name2);
        }
        state.OnTestsComplete();
        return state.countFailedTests == 0;
    }

    void check(State& teststate, bool result, const char* source, const char* errorMessage)
    {
        if (result)
            return;

        printf("In '%s': %s\n", source, errorMessage);

        ++teststate.countChecksFailed;
        if (!teststate.currentFailed) {
            teststate.currentFailed = true;
            ++teststate.countFailedTests;
        }
        __debugbreak();
    }

    TEST(gtest, test_self)
    {
        EXPECT_TRUE( true );
        EXPECT_FALSE( false );
        EXPECT_EQ( 1, 1);

        EXPECT_THROW( throw "expected_exception", char* );
        EXPECT_NOTHROW( ; );
    }

    TEST_DISABLED(gtest, disabled)
    {
        EXPECT_TRUE( false );
    }
}
