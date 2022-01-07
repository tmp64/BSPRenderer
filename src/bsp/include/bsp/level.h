#ifndef BSP_LEVEL_H
#define BSP_LEVEL_H
#include <array>
#include <cstdint>
#include <stdexcept>
#include <map>
#include <optional>
#include <memory>
#include <appfw/utils.h>
#include <appfw/appfw.h>
#include <appfw/span.h>
#include <bsp/bsp_types.h>

namespace bsp {

class LevelFormatException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Level {
public:
    class EntityList;

    class EntityListItem {
    public:
        inline bool hasValue(const std::string &key) const { return m_Keys.find(key) != m_Keys.end(); }

        template <typename T>
        inline T getValue(const std::string &key, const std::optional<T> &defVal = std::nullopt) const {
            auto it = m_Keys.find(key);

            if (it == m_Keys.end()) {
                if (defVal.has_value()) {
                    return defVal.value();
                } else {
                    throw std::runtime_error(std::string("key '") + key + "' not found");
                }
            } else {
                T ret;
                if (appfw::convertStringToVal(it->second, ret)) {
                    return ret;
                } else {
                    throw std::runtime_error(std::string("key '") + key + "' is in wrong format");
                }
            }
        }

    private:
        std::map<std::string, std::string> m_Keys;

        friend class Level::EntityList;
    };

    class EntityList {
    public:
        EntityList() = default;

        /**
         * Initializes entity list from entity lump.
         */
        EntityList(const std::vector<char> &entityLump);

        /**
         * Returns "worldspawn" entity.
         */
        inline const EntityListItem &getWorldspawn() const { return *m_pWorldspawn; }

        /**
         * Returns pointer to an entity with specified targetname or nullptr.
         */
        const EntityListItem *findEntityByName(const std::string &targetname, const EntityListItem *pPrev = nullptr) const;

        /**
         * Returns pointer to an entity with specified classname or nullptr.
         */
        const EntityListItem *findEntityByClassname(const std::string &classname,
                                                    const EntityListItem *pPrev = nullptr) const;

        inline auto begin() { return m_Items.begin(); }
        inline auto end() { return m_Items.end(); }
        inline auto begin() const { return m_Items.begin(); }
        inline auto end() const { return m_Items.end(); }

        inline auto rbegin() { return m_Items.rbegin(); }
        inline auto rend() { return m_Items.rend(); }
        inline auto rbegin() const { return m_Items.rbegin(); }
        inline auto rend() const { return m_Items.rend(); }

        inline size_t size() const { return m_Items.size(); }

        inline EntityListItem &operator[](size_t idx) { return m_Items[idx]; }
        inline const EntityListItem &operator[](size_t idx) const { return m_Items[idx]; }

    private:
        std::vector<EntityListItem> m_Items;
        EntityListItem *m_pWorldspawn = nullptr;
    };

    /**
     * Constructs an empty level.
     */
    Level() = default;

    /**
     * Loads BSP from supplied data.
     */
    Level(appfw::span<uint8_t> data);

    /**
     * Loads BSP from specified .bsp file.
     */
    Level(const std::string &filename);

    /**
     * Loads BSP from a .bsp file.
     */
    void loadFromFile(const fs::path &path);

    /**
     * Loads BSP from supplied contents of .bsp.
     */
    void loadFromBytes(appfw::span<uint8_t> data);

    //! Returns the vertices of a face.
    std::vector<glm::vec3> getFaceVertices(const bsp::BSPFace &face) const;

    /**
     * Traces a line and returns contents of hit leaf
     * @returns (CONTENTS_SOLID or CONTENTS_SKY) or CONTENTS_EMPTY if didn't hit.
     */
    int traceLine(glm::vec3 from, glm::vec3 to) const;

    /**
     * Finds leaf which contains the point.
     * @return  Negative int pointing to the leaf.
     */
    int pointInLeaf(glm::vec3 p) const noexcept;

    /**
     * Decompresses (RLE) PVS data for a leaf.
     * @param   leaf    Negative leaf index
     * @param   buf     Buffer of size at least (bsp::MAX_MAP_LEAFS / 8) to decompress PVS into
     * @return Poitner into buf or to a static buffer if not vis is loaded
     */
    const uint8_t *leafPVS(int leaf, appfw::span<uint8_t> buf) const noexcept;

    inline const std::vector<BSPPlane> &getPlanes() const { return m_Planes; }
    inline const std::vector<BSPMipTex> &getTextures() const { return m_Textures; }
    inline const std::vector<glm::vec3> &getVertices() const { return m_Vertices; }
    inline const std::vector<uint8_t> &getVisData() const { return m_VisData; }
    inline const std::vector<BSPNode> &getNodes() const { return m_Nodes; }
    inline const std::vector<BSPTextureInfo> &getTexInfo() const { return m_TexInfo; }
    inline const std::vector<BSPFace> &getFaces() const { return m_Faces; }
    inline const std::vector<uint8_t> &getLightMaps() const { return m_Lightmaps; }
    inline const std::vector<BSPLeaf> &getLeaves() const { return m_Leaves; }
    inline const std::vector<BSPMarkSurface> &getMarkSurfaces() const { return m_MarkSurfaces; }
    inline const std::vector<BSPEdge> &getEdges() const { return m_Edges; }
    inline const std::vector<BSPSurfEdge> &getSurfEdges() const { return m_SurfEdges; }
    inline const std::vector<BSPModel> &getModels() const { return m_Models; }
    inline const EntityList &getEntities() const { return m_Entities; }
    inline const std::vector<uint8_t> &getRawTextures() const { return m_RawTextureLump; }

private:
    std::vector<BSPPlane> m_Planes;
    std::vector<BSPMipTex> m_Textures;
    std::vector<glm::vec3> m_Vertices;
    std::vector<uint8_t> m_VisData;
    std::vector<BSPNode> m_Nodes;
    std::vector<BSPTextureInfo> m_TexInfo;
    std::vector<BSPFace> m_Faces;
    std::vector<uint8_t> m_Lightmaps;
    std::vector<BSPLeaf> m_Leaves;
    std::vector<BSPMarkSurface> m_MarkSurfaces;
    std::vector<BSPEdge> m_Edges;
    std::vector<BSPSurfEdge> m_SurfEdges;
    std::vector<BSPModel> m_Models;
    EntityList m_Entities;
    std::vector<uint8_t> m_RawTextureLump;

    int recursiveTraceLine(int node, const glm::vec3 &from, const glm::vec3 &to) const;
};

} // namespace bsp

#endif
