#ifndef VIS_H
#define VIS_H

void buildVisMatrix();
void buildVisLeaves();
void buildVisRow(size_t patchnum, uint8_t *pvs, size_t bitpos);
void testPatchToFace(size_t patchnum, int facenum, size_t bitpos);
void decompressVis(const uint8_t *in, uint8_t *decompressed);
const bsp::BSPLeaf *pointInLeaf(glm::vec3 point);
bool checkVisBit(size_t p1, size_t p2);

#endif
