#ifndef BSP_BSP_TYPE_H
#define BSP_BSP_TYPE_H
#include <array>
#include <cstdint>
#include <glm/glm.hpp>

static_assert(sizeof(glm::vec3) == 3 * sizeof(float), "glm::vec3 is not packed");

/**
 * Based on HLBSP project
 * See http://hlbsp.sourceforge.net/index.php?content=bspdef
 */

namespace bsp {

constexpr int HL_BSP_VERSION = 30;

constexpr int LUMP_ENTITIES = 0;
constexpr int LUMP_PLANES = 1;
constexpr int LUMP_TEXTURES = 2;
constexpr int LUMP_VERTICES = 3;
constexpr int LUMP_VISIBILITY = 4;
constexpr int LUMP_NODES = 5;
constexpr int LUMP_TEXINFO = 6;
constexpr int LUMP_FACES = 7;
constexpr int LUMP_LIGHTING = 8;
constexpr int LUMP_CLIPNODES = 9;
constexpr int LUMP_LEAVES = 10;
constexpr int LUMP_MARKSURFACES = 11;
constexpr int LUMP_EDGES = 12;
constexpr int LUMP_SURFEDGES = 13;
constexpr int LUMP_MODELS = 14;
constexpr int MAX_BSP_LUMPS = 15;

constexpr int MAX_MAP_HULLS = 4;
constexpr int MAX_MAP_MODELS = 400;
constexpr int MAX_MAP_BRUSHES = 4096;
constexpr int MAX_MAP_ENTITIES = 1024;
constexpr int MAX_MAP_ENTSTRING = (128 * 1024);

constexpr int MAX_MAP_PLANES = 32767;
constexpr int MAX_MAP_NODES = 32767;
constexpr int MAX_MAP_CLIPNODES = 32767;
constexpr int MAX_MAP_LEAFS = 8192;
constexpr int MAX_MAP_VERTS = 65535;
constexpr int MAX_MAP_FACES = 65535;
constexpr int MAX_MAP_MARKSURFACES = 65535;
constexpr int MAX_MAP_TEXINFO = 8192;
constexpr int MAX_MAP_EDGES = 256000;
constexpr int MAX_MAP_SURFEDGES = 512000;
constexpr int MAX_MAP_TEXTURES = 512;
constexpr int MAX_MAP_MIPTEX = 0x200000;
constexpr int MAX_MAP_LIGHTING = 0x200000;
constexpr int MAX_MAP_VISIBILITY = 0x200000;

constexpr int MAX_MAP_PORTALS = 65536;

constexpr std::array<const char *, MAX_BSP_LUMPS> LUMP_NAME{
    "LUMP_ENTITIES",
    "LUMP_PLANES",
    "LUMP_TEXTURES",
    "LUMP_VERTICES",
    "LUMP_VISIBILITY",
    "LUMP_NODES",
    "LUMP_TEXINFO",
    "LUMP_FACES",
    "LUMP_LIGHTING",
    "LUMP_CLIPNODES",
    "LUMP_LEAVES",
    "LUMP_MARKSURFACES",
    "LUMP_EDGES",
    "LUMP_SURFEDGES",
    "LUMP_MODELS",
};

//----------------------------------------------------------------
// BSP Header
//----------------------------------------------------------------
struct BSPLump {
    /**
     * File offset to data
     */
    int32_t nOffset;

    /** 
     * Length of data
     */
    int32_t nLength;
};

struct BSPHeader {
    /**
     * Must be HL_BSP_VERSION (30) for a valid HL BSP file
     */
    int32_t nVersion;

    /**
     * Stores the directory of lumps
     */
    BSPLump lump[MAX_BSP_LUMPS];
};

//----------------------------------------------------------------
// Entity Lump
//----------------------------------------------------------------
constexpr int MAX_ENTITY_KEY = 32;
constexpr int MAX_ENTITY_VALUE = 1024;

//----------------------------------------------------------------
// Planes Lump
//----------------------------------------------------------------
enum class PlaneType : int32_t {
    PlaneX = 0,
    PlaneY,
    PlaneZ,
    PlaneAnyX,
    PlaneAnyY,
    PlaneAnyZ
};

struct BSPPlane {
    /**
     * The planes normal vector
     */
    glm::vec3 vNormal; 

    /**
     * Plane equation is: vNormal * X = fDist
     */
    float fDist;

    /**
     * Plane type
     */
    PlaneType nType;
};

//----------------------------------------------------------------
// Texture Lump
//----------------------------------------------------------------
constexpr int MAX_TEXTURE_NAME = 16;
constexpr int MIP_LEVELS = 4;

using BSPMipTexOffset = int32_t;

struct BSPTextureHeader {
    /** 
     * Number of BSPMIPTEX structures
     */
    uint32_t nMipTextures;
};

struct BSPMipTex {
    /**
     * Name of texture
     */
    char szName[MAX_TEXTURE_NAME];

    /**
     * Extends of the texture
     */
    uint32_t nWidth, nHeight;

    /**
     * Offsets to texture mipmaps BSPMIPTEX
     */
    uint32_t nOffsets[MIP_LEVELS];
};

//----------------------------------------------------------------
// Vertices Lump
//----------------------------------------------------------------
using BSPVertex = glm::vec3;

//----------------------------------------------------------------
// VIS Lump
//----------------------------------------------------------------

//----------------------------------------------------------------
// Nodes Lump
//----------------------------------------------------------------
struct BSPNode {
    /**
     * Index into Planes lump
     */
    uint32_t iPlane;

    /**
     * If > 0, then indices into Nodes // otherwise bitwise inverse indices into Leafs
     */
    int16_t iChildren[2];

    /**
     * Defines bounding box
     */
    int16_t nMins[3], nMaxs[3];

    /**
     * Index and count into Faces
     */
    uint16_t firstFace, nFaces;
};

//----------------------------------------------------------------
// Texinfo Lump
//----------------------------------------------------------------
struct BSPTextureInfo {
    glm::vec3 vS;

    /**
     * Texture shift in s direction
     */
    float fSShift;
    glm::vec3 vT;

    /**
     * Texture shift in t direction
     */
    float fTShift;

    /**
     * Index into textures array
     */
    uint32_t iMiptex;

    /**
     * Texture flags, seem to always be 0
     */
    uint32_t nFlags;
};

//----------------------------------------------------------------
// Faces Lump
//----------------------------------------------------------------
struct BSPFace {
    /**
     * Plane the face is parallel to
     */
    uint16_t iPlane;

    /**
     * Set if different normals orientation
     */
    uint16_t nPlaneSide;

    /**
     * Index of the first surfedge
     */
    uint32_t iFirstEdge;

    /**
     * Number of consecutive surfedges
     */
    uint16_t nEdges;

    /**
     * Index of the texture info structure
     */
    uint16_t iTextureInfo;

    /**
     * Specify lighting styles
     */
    uint8_t nStyles[4];

    /**
     * Offsets into the raw lightmap data
     */
    uint32_t nLightmapOffset;
};

//----------------------------------------------------------------
// Lightmap Lump
//----------------------------------------------------------------

//----------------------------------------------------------------
// Clipnodes Lump
//----------------------------------------------------------------
struct BSPClipNode {
    /**
     * Index into planes
     */
    int32_t iPlane;

    /**
     * negative numbers are contents
     */
    int16_t iChildren[2];
};

//----------------------------------------------------------------
// Leaves Lump
//----------------------------------------------------------------
constexpr int CONTENTS_EMPTY = -1;
constexpr int CONTENTS_SOLID = -2;
constexpr int CONTENTS_WATER = -3;
constexpr int CONTENTS_SLIME = -4;
constexpr int CONTENTS_LAVA = -5;
constexpr int CONTENTS_SKY = -6;
constexpr int CONTENTS_ORIGIN = -7;
constexpr int CONTENTS_CLIP = -8;
constexpr int CONTENTS_CURRENT_0 = -9;
constexpr int CONTENTS_CURRENT_90 = -10;
constexpr int CONTENTS_CURRENT_180 = -11;
constexpr int CONTENTS_CURRENT_270 = -12;
constexpr int CONTENTS_CURRENT_UP = -13;
constexpr int CONTENTS_CURRENT_DOWN = -14;
constexpr int CONTENTS_TRANSLUCENT = -15;

struct BSPLeaf {
    /**
     * Contents enumeration
     */
    int32_t nContents;

    /**
     * Offset into the visibility lump
     */
    int32_t nVisOffset;

    /**
     * Defines bounding box
     */
    int16_t nMins[3], nMaxs[3];

    /**
     * Index and count into marksurfaces array
     */
    uint16_t iFirstMarkSurface, nMarkSurfaces;

    /**
     * Ambient sound levels
     */
    uint8_t nAmbientLevels[4];
};

//----------------------------------------------------------------
// Marksurfaces Lump
//----------------------------------------------------------------
using BSPMarkSurface = uint16_t;

//----------------------------------------------------------------
// Edges Lump
//----------------------------------------------------------------
struct BSPEdge {
    /**
     * Indices into vertex array
     */
    uint16_t iVertex[2];
};

//----------------------------------------------------------------
// Surfedges Lump
//----------------------------------------------------------------
using BSPSurfEdge = int32_t;

//----------------------------------------------------------------
// Models Lump
//----------------------------------------------------------------
struct BSPModel {
    /**
     * Defines bounding box
     */
    float nMins[3], nMaxs[3];

    /**
     * Coordinates to move the // coordinate system
     */
    glm::vec3 vOrigin;

    /**
     * Index into nodes array
     */
    int32_t iHeadnodes[MAX_MAP_HULLS];

    /**
     * ???
     */
    int32_t nVisLeafs;

    /**
     * Index and count into faces
     */
    int32_t iFirstFace, nFaces;
};

} // namespace bsp

#endif
