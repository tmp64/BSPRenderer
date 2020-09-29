#ifndef PATCH_LIST_H
#define PATCH_LIST_H
#include <vector>
#include <map>
#include "main.h"

class PatchRef;

class PatchList {
public:
    struct Iterator {
        size_t m_iIndex;
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
    inline size_t size() { return m_iSize; }

    /**
     * Allocates N patches.
     */
    void allocate(size_t size);

    /**
     * Removes all patches.
     */
    void clear();

    /**
     * Returns iterator to first patch.
     */
    inline Iterator begin() { return Iterator{0}; };

    /**
     * Returns iterator to patch after the last one.
     */
    inline Iterator end() { return Iterator{m_iSize}; };

    /**
     * Returns memory usage of one patch.
     */
    inline size_t getPatchMemoryUsage() {
        //
        auto fn = [](auto &i) {
            using Type = std::remove_reference<decltype(i)>::type::value_type;
            return sizeof(Type);
        };
        return fn(m_flSize) +
            fn(m_vOrigin) +
            fn(m_vNormal) +
            fn(m_pPlane) +
            fn(m_FinalColor) +
            fn(m_pLMPixel) +
            fn(m_ViewFactors);
    }

private:
    size_t m_iSize = 0;

    std::vector<float> m_flSize;
    std::vector<glm::vec3> m_vOrigin;
    std::vector<glm::vec3> m_vNormal;
    std::vector<const Plane *> m_pPlane;
    std::vector<glm::vec3> m_FinalColor;
    std::vector<glm::vec3 *> m_pLMPixel;
    std::vector<std::map<size_t, float>> m_ViewFactors;

    friend class PatchRef;
};

extern PatchList g_Patches;

class PatchRef {
public:
    inline explicit PatchRef(size_t idx) : m_iIndex(idx) {}

    /**
     * Length of a side of the square.
     */
    inline float &getSize() { return g_Patches.m_flSize[m_iIndex]; }

    /**
     * Center point of the square in world-space.
     */
    inline glm::vec3 &getOrigin() { return g_Patches.m_vOrigin[m_iIndex]; }

    /**
     * Normal vector. Points away from front side.
     */
    inline glm::vec3 &getNormal() { return g_Patches.m_vNormal[m_iIndex]; }

    /**
     * Plane in which patch is located.
     */
    inline const Plane * &getPlane() { return g_Patches.m_pPlane[m_iIndex]; }

    /**
     * Final color of the patch.
     */
    inline glm::vec3 &getFinalColor() { return g_Patches.m_FinalColor[m_iIndex]; }

    /**
     * Pointer to lightmap pixel.
     */
    inline glm::vec3 * &getLMPixel() { return g_Patches.m_pLMPixel[m_iIndex]; }

    /**
     * View factors for visible patches.
     */
    inline std::map<size_t, float> &getViewFactors() { return g_Patches.m_ViewFactors[m_iIndex]; }

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
    size_t m_iIndex = std::numeric_limits<size_t>::max();
};

#endif
