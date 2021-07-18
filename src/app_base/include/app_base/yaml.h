#ifndef APP_BASE_YAML_H
#define APP_BASE_YAML_H
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

namespace YAML {
template <glm::length_t L, typename T>
struct convert<glm::vec<L, T, glm::defaultp>> {
    static inline Node encode(const glm::vec<L, T, glm::defaultp> &rhs) {
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        for (glm::length_t i = 0; i < L; i++) {
            node.push_back(rhs[i]);
        }
        return node;
    }

    static inline bool decode(const Node &node, glm::vec<L, T, glm::defaultp> &rhs) {
        if (!node.IsSequence() || node.size() != L) {
            return false;
        }

        for (glm::length_t i = 0; i < L; i++) {
            rhs[i] = node[i].as<T>();
        }

        return true;
    }
};
} // namespace YAML

#endif
