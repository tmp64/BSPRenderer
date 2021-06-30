#ifndef PATCH_LIST_H
#define PATCH_LIST_H
#include <vector>
#include <map>
#include <limits>
#include <appfw/utils.h>
#include <rad/types.h>

namespace rad {

class PatchRef;

class PatchList {
public:
    struct Iterator {
        PatchList *m_pList;
        PatchIndex m_iIndex;
        PatchRef operator*();

        inline Iterator &operator++() {
            ++m_iIndex;
            return *this;
        }
        inline Iterator operator++(int) {
            Iterator t(*this);
            ++m_iIndex;
            return t;
        }

        inline Iterator &operator--() {
            --m_iIndex;
            return *this;
        }
        inline Iterator operator--(int) {
            Iterator t(*this);
            --m_iIndex;
            return t;
        }

        inline bool operator!=(const Iterator &other) { return m_iIndex != other.m_iIndex; }
    };

    /**
     * Returns patch count.
     */
    inline PatchIndex size() { return (PatchIndex)m_iSize;
    }

    /**
     * Allocates N patches.
     */
    void allocate(PatchIndex size);

    /**
     * Removes all patches.
     */
    void clear();

    /**
     * Returns iterator to first patch.
     */
    inline Iterator begin() { return Iterator{this, 0}; };

    /**
     * Returns iterator to patch after the last one.
     */
    inline Iterator end() { return Iterator{this, m_iSize}; };

    /**
     * Returns memory usage of one patch.
     */
    inline size_t getPatchMemoryUsage() {
        auto fn = [](auto &i) {
            using Type = std::remove_reference<decltype(i)>::type::value_type;
            return sizeof(Type);
        };

        return fn(m_flSize) + fn(m_vFaceOrigin) + fn(m_vOrigin) + fn(m_vNormal) + fn(m_pPlane) +
               fn(m_FinalColor);
    }

private:
    PatchIndex m_iSize = 0;

    std::vector<float> m_flSize;
    std::vector<glm::vec2> m_vFaceOrigin;
    std::vector<glm::vec3> m_vOrigin;
    std::vector<glm::vec3> m_vNormal;
    std::vector<const Plane *> m_pPlane;
    std::vector<glm::vec3> m_FinalColor;

    friend class PatchRef;
};

class PatchRef {
public:
    inline explicit PatchRef(PatchList &list, PatchIndex idx) : m_List(list), m_iIndex(idx) {}

    /**
     * Length of a side of the square.
     */
    inline float &getSize() { return m_List.m_flSize[m_iIndex]; }

    /**
     * Center point of the square in face-space.
     */
    inline glm::vec2 &getFaceOrigin() { return m_List.m_vFaceOrigin[m_iIndex]; }

    /**
     * Center point of the square in world-space.
     */
    inline glm::vec3 &getOrigin() { return m_List.m_vOrigin[m_iIndex]; }

    /**
     * Normal vector. Points away from front side.
     */
    inline glm::vec3 &getNormal() { return m_List.m_vNormal[m_iIndex]; }

    /**
     * Plane in which patch is located.
     */
    inline const Plane *&getPlane() { return m_List.m_pPlane[m_iIndex]; }

    /**
     * Final color of the patch.
     */
    inline glm::vec3 &getFinalColor() { return m_List.m_FinalColor[m_iIndex]; }

    inline PatchRef &operator++() {
        ++m_iIndex;
        return *this;
    }
    inline PatchRef operator++(int) {
        PatchRef t(*this);
        ++m_iIndex;
        return t;
    }

    inline PatchRef &operator--() {
        --m_iIndex;
        return *this;
    }
    inline PatchRef operator--(int) {
        PatchRef t(*this);
        --m_iIndex;
        return t;
    }

private:
    PatchList &m_List;
    PatchIndex m_iIndex = std::numeric_limits<PatchIndex>::max();
};

} // namespace rad

#endif
