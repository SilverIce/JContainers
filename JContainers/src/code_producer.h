#pragma once

#include <string>

namespace collections {

    namespace tes_binding {
        struct class_meta_info;
    }

    // Produces script files using meta class information
    namespace code_producer {

        std::string produceClassCode(const tes_binding::class_meta_info& self);
        void produceClassToFile(const tes_binding::class_meta_info& self);

    }

}
