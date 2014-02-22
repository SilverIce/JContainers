#pragma once

#include <vector>

namespace collections {

    template<class ID>
    class id_generator {
        std::vector<ID> m_freeNums;
        ID m_highest;

    public:

        id_generator() : m_highest(0) {}

        ID newId() {
            if (!m_freeNums.empty()) {
                int num = m_freeNums.back();
                m_freeNums.pop_back();
                return num;
            }

            ++m_highest;
            return m_highest;
        }

        void reuseId(ID num) {
            m_freeNums.push_back(num);
        }

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & m_freeNums;
            ar & m_highest;
        }

        void u_clear() {
            m_highest = 0;
            m_freeNums.clear();
        }
    };

}
