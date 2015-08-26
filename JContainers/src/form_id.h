#pragma once

#include <cstdint>

namespace collections {

    using FormIdUnredlying = uint32_t;

    enum class FormId : FormIdUnredlying {
        Zero = 0,
    };

    enum class FormHandle : uint64_t {
    };

    enum {
        FormGlobalPrefix = 0xFF,
    };
}
