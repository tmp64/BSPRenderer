#ifndef VIS_H
#define VIS_H
#include <appfw/utils.h>
#include <bsp/level.h>

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct SurfaceRaycastHit {
    int surface = -1;
    glm::vec3 point = glm::vec3(0, 0, 0);
    float distance = INFINITY;
    int entIndex = -1;
};

struct EntityRaycastHit {
    int entity = -1;
    glm::vec3 point = glm::vec3(0, 0, 0);
    float distance = INFINITY;
};

static constexpr float MAX_RAYCAST_DIST = 8192;

class Vis : appfw::NoMove {
public:
    static inline Vis &get() { return *m_spInstance; }
    Vis();
    ~Vis();

    //! Sets the level
    void setLevel(bsp::Level *pLevel);

    //! Returns whether AABB (mins, maxs) may be visible from point origin based on PVS data.
    bool boxInPvs(const glm::vec3 &origin, const glm::vec3 &mins, const glm::vec3 &maxs);

    //! Checks if a ray intersects with a surface. All other surfaces are ignored.
    bool rayIntersectsWithSurface(const Ray &ray, int surfIdx, SurfaceRaycastHit &hit,
                                  float maxDist);

    //! Casts a ray to a world or entity surface.
    //! @param  hit     The closest surface hit
    //! @returns whether hit something or not
    bool raycastToSurface(const Ray &ray, SurfaceRaycastHit &hit, float maxDist = MAX_RAYCAST_DIST);

    //! Casts a ray to a world surface.
    //! @param  hit     The closest surface hit
    //! @returns whether hit something or not
    bool raycastToWorldSurface(const Ray &ray, SurfaceRaycastHit &hit,
                               float maxDist = MAX_RAYCAST_DIST);

    //! Casts a ray to a brush entity surface.
    //! @param  hit     The closest surface hit
    //! @returns whether hit something or not
    bool raycastToEntitySurface(const Ray &ray, SurfaceRaycastHit &hit,
                                float maxDist = MAX_RAYCAST_DIST);

    //! Casts a ray to an entity.
    //! @param  hit     The closest entity hit
    //! @returns whether hit something or not
    bool raycastToEntity(const Ray &ray, EntityRaycastHit &hit, float maxDist = MAX_RAYCAST_DIST);

    //! Casts a ray to an AABB.
    //! @param  ray         The ray
    //! @param  mins        Min bounds
    //! @param  maxs        Max bounds
    //! @param  hitpoint    Intersection point if hit
    //! @returns whether hit the AABB or not.
    static bool raycastToAABB(const Ray &ray, glm::vec3 mins, glm::vec3 maxs, glm::vec3 &hitpoint);

private:
    static constexpr int MAX_BOX_LEAFS = 256;

    struct LeafList {
        int count;
        int maxcount;
        bool overflowed;
        short *list;
        glm::vec3 mins, maxs;
        int topnode; // for overflows where each leaf can't be stored individually
    };

    bsp::Level *m_pLevel = nullptr;
    std::array<uint8_t, bsp::MAX_MAP_LEAFS / 8> m_VisBuf;

    //! Returns true if AABB is visible in VIS.
    bool isBoxVisible(const glm::vec3 &mins, const glm::vec3 &maxs, const uint8_t *visbits);

    //! Fills specified list with leafs in which AABB is located.
    //! @returns number of list items
    int boxLeafNums(const glm::vec3 &mins, const glm::vec3 &maxs, appfw::span<short> list);
    void boxLeafNums_r(LeafList &ll, int node);

    void raycastRecursiveWorldNodes(int node, const Ray &ray, SurfaceRaycastHit &hit);

    static inline Vis *m_spInstance = nullptr;
};

#endif
