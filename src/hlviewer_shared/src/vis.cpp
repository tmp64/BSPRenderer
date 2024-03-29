#include <renderer/utils.h>
#include <hlviewer/vis.h>
#include <hlviewer/world_state_base.h>

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int boxOnPlaneSide(const glm::vec3 &emins, const glm::vec3 &emaxs, const bsp::BSPPlane *p) {
    float dist1, dist2;
    int sides = 0;
    uint8_t signbits = signbitsForPlane(p->vNormal);

    // general case
    switch (signbits) {
    case 0:
        dist1 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emaxs[2];
        dist2 = p->vNormal[0] * emins[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emins[2];
        break;
    case 1:
        dist1 = p->vNormal[0] * emins[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emaxs[2];
        dist2 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emins[2];
        break;
    case 2:
        dist1 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emaxs[2];
        dist2 = p->vNormal[0] * emins[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emins[2];
        break;
    case 3:
        dist1 = p->vNormal[0] * emins[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emaxs[2];
        dist2 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emins[2];
        break;
    case 4:
        dist1 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emins[2];
        dist2 = p->vNormal[0] * emins[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emaxs[2];
        break;
    case 5:
        dist1 = p->vNormal[0] * emins[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emins[2];
        dist2 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emaxs[2];
        break;
    case 6:
        dist1 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emins[2];
        dist2 = p->vNormal[0] * emins[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emaxs[2];
        break;
    case 7:
        dist1 = p->vNormal[0] * emins[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emins[2];
        dist2 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emaxs[2];
        break;
    default:
        // shut up compiler
        dist1 = dist2 = 0;
        break;
    }

    if (dist1 >= p->fDist)
        sides = 1;
    if (dist2 < p->fDist)
        sides |= 2;

    return sides;
}

Vis::Vis(WorldStateBase &worldState)
    : m_WorldState(worldState) {
    std::fill(m_VisBuf.begin(), m_VisBuf.end(), (uint8_t)0);
}

void Vis::setLevel(const bsp::Level *pLevel) {
    m_pLevel = pLevel;
}

bool Vis::boxInPvs(const glm::vec3 &origin, const glm::vec3 &mins, const glm::vec3 &maxs) {
    int leaf = m_pLevel->pointInLeaf(origin);
    const uint8_t *vis = m_pLevel->leafPVS(leaf, m_VisBuf);

    return isBoxVisible(mins, maxs, vis);
}

bool Vis::rayIntersectsWithSurface(const Ray &ray, int surfIdx, SurfaceRaycastHit &hit,
                                   float maxDist) {
    const bsp::BSPFace &face = m_pLevel->getFaces()[surfIdx];
    const bsp::BSPPlane &plane = m_pLevel->getPlanes()[face.iPlane];
    glm::vec3 normal = plane.vNormal;
    float fDist = plane.fDist;

    if (face.nPlaneSide) {
        normal *= -1.0f;
        fDist *= -1;
    }

    // Check plane side
    if (glm::dot(ray.origin, normal) - fDist < 0) {
        return false;
    }

    // Find intersection with the plane
    float t = glm::dot(ray.direction, normal);

    if (abs(t) < 0.001) {
        // Ray is parallel to the plane
        return false;
    }

    float d = (fDist - glm::dot(ray.origin, normal)) / t;

    if (d > maxDist || d < 0) {
        // Too far or behind
        return false;
    }

    glm::vec3 point = ray.origin + ray.direction * d;

    // Check if the point is in the face
    std::vector<glm::vec3> vertices = m_pLevel->getFaceVertices(face);
    size_t count = vertices.size();

    for (size_t j = 0; j < count; j++) {
        glm::vec3 a = vertices[j];
        glm::vec3 b = vertices[(j + 1) % count];
        glm::vec3 edge = b - a;
        glm::vec3 p = point - a;

        if (glm::dot(normal, glm::cross(edge, p)) > 0) {
            return false;
        }
    }

    hit.surface = (int)surfIdx;
    hit.point = point;
    hit.distance = d;
    return true;
}

bool Vis::raycastToSurface(const Ray &ray, SurfaceRaycastHit &hit, bool ignoreTriggers,
                           float maxDist) {
    hit = SurfaceRaycastHit();
    hit.distance = maxDist;

    SurfaceRaycastHit worldHit, entHit;
    if (raycastToWorldSurface(ray, worldHit, maxDist)) {
        hit = worldHit;
    }

    // Entities are not rendered behind the world so raycast can't go further than the world
    if (raycastToEntitySurface(ray, entHit, ignoreTriggers, worldHit.distance)) {
        hit = entHit;
    }

    return hit.surface != -1;

#if 0
    // Bruteforce is bad, mkay
    auto &faces = m_pLevel->getFaces();

    for (size_t surfIdx = 0; surfIdx < faces.size(); surfIdx++) {
        SurfaceRaycastHit testHit;
        if (rayIntersectsWithSurface(ray, (int)surfIdx, testHit, hit.distance) &&
            testHit.distance < hit.distance) {
            hit = testHit;
        }
    }

    return hit.surface != -1;
#endif
}

bool Vis::raycastToWorldSurface(const Ray &ray, SurfaceRaycastHit &hit, float maxDist) {
    hit = SurfaceRaycastHit();
    hit.distance = maxDist;
    raycastRecursiveWorldNodes(0, ray, hit);
    return hit.surface != -1;
}

bool Vis::raycastToEntitySurface(const Ray &inputRay, SurfaceRaycastHit &hit, bool ignoreTriggers,
                                 float maxDist) {
    hit = SurfaceRaycastHit();
    hit.distance = maxDist;
    auto &ents = m_WorldState.getEntList();

    for (size_t i = 0; i < ents.size(); i++) {
        BaseEntity *pEnt = ents[i].get();

        if (ignoreTriggers && pEnt->isTrigger()) {
            continue;
        }

        if (pEnt->getModel() && pEnt->getModel()->type == ModelType::Brush) {
            BrushModel &model = static_cast<BrushModel &>(*pEnt->getModel());

            SurfaceRaycastHit testHit;
            testHit.distance = hit.distance; // limit the distance

            Ray ray = inputRay;
            ray.origin -= pEnt->getOrigin();

            raycastRecursiveWorldNodes(model.iHeadnodes[0], ray, testHit);

            if (testHit.surface != -1) {
                hit = testHit;
                hit.point += pEnt->getOrigin();
                hit.entIndex = (int)i;
            }
        }
    }

    return hit.entIndex != -1;
}

bool Vis::raycastToEntity(const Ray &inputRay, EntityRaycastHit &hit, bool ignoreTriggers,
                          float maxDist) {
    hit = EntityRaycastHit();
    hit.distance = maxDist;
    auto &ents = m_WorldState.getEntList();

    for (size_t i = 0; i < ents.size(); i++) {
        BaseEntity *pEnt = ents[i].get();

        if (ignoreTriggers && pEnt->isTrigger()) {
            continue;
        }

        if (pEnt->useAABB()) {
            glm::vec3 mins = pEnt->getOrigin() + pEnt->getAABBPos() - pEnt->getAABBHalfExtents();
            glm::vec3 maxs = pEnt->getOrigin() + pEnt->getAABBPos() + pEnt->getAABBHalfExtents();
            glm::vec3 hitpoint = glm::vec3(0, 0, 0);

            if (raycastToAABB(inputRay, mins, maxs, hitpoint)) {
                float dist = glm::length(hitpoint - inputRay.origin);

                if (dist < hit.distance) {
                    hit.entity = (int)i;
                    hit.point = hitpoint;
                    hit.distance = dist;
                }
            }
        } else if (pEnt->getModel()) {
            Model &baseModel = *pEnt->getModel();

            if (baseModel.type == ModelType::Brush) {
                BrushModel &model = static_cast<BrushModel &>(*pEnt->getModel());

                SurfaceRaycastHit testHit;
                testHit.distance = hit.distance; // limit the distance

                Ray ray = inputRay;
                ray.origin -= pEnt->getOrigin();

                raycastRecursiveWorldNodes(model.iHeadnodes[0], ray, testHit);

                if (testHit.surface != -1) {
                    hit.point = testHit.point + pEnt->getOrigin();
                    hit.distance = testHit.distance;
                    hit.entity = (int)i;
                }
            } else {
                AFW_ASSERT_MSG(false, "Unknown model type");
            }
        }
    }

    return hit.entity != -1;
}

//! Fast Ray-Box Intersection
//! by Andrew Woo
//! from "Graphics Gems", Academic Press, 1990
//! https://github.com/erich666/GraphicsGems/blob/master/gems/RayBox.c
bool Vis::raycastToAABB(const Ray &ray, glm::vec3 minB, glm::vec3 maxB, glm::vec3 &coord) {
    constexpr int NUMDIM = 3;
    constexpr int RIGHT = 0;
    constexpr int LEFT = 1;
    constexpr int MIDDLE = 2;

    bool inside = true;
    char quadrant[NUMDIM];
    int i;
    int whichPlane;
    float maxT[NUMDIM];
    float candidatePlane[NUMDIM];

    /* Find candidate planes; this loop can be avoided if
    rays cast all from the eye(assume perpsective view) */
    for (i = 0; i < NUMDIM; i++)
        if (ray.origin[i] < minB[i]) {
            quadrant[i] = LEFT;
            candidatePlane[i] = minB[i];
            inside = false;
        } else if (ray.origin[i] > maxB[i]) {
            quadrant[i] = RIGHT;
            candidatePlane[i] = maxB[i];
            inside = false;
        } else {
            quadrant[i] = MIDDLE;
        }

    /* Ray origin inside bounding box */
    if (inside) {
        coord = ray.origin;
        return true;
    }

    /* Calculate T distances to candidate planes */
    for (i = 0; i < NUMDIM; i++)
        if (quadrant[i] != MIDDLE && ray.direction[i] != 0.)
            maxT[i] = (candidatePlane[i] - ray.origin[i]) / ray.direction[i];
        else
            maxT[i] = -1.;

    /* Get largest of the maxT's for final choice of intersection */
    whichPlane = 0;
    for (i = 1; i < NUMDIM; i++)
        if (maxT[whichPlane] < maxT[i])
            whichPlane = i;

    /* Check final candidate actually inside box */
    if (maxT[whichPlane] < 0.)
        return false;
    for (i = 0; i < NUMDIM; i++)
        if (whichPlane != i) {
            coord[i] = ray.origin[i] + maxT[whichPlane] * ray.direction[i];
            if (coord[i] < minB[i] || coord[i] > maxB[i])
                return false;
        } else {
            coord[i] = candidatePlane[i];
        }
    return true; /* ray hits box */
}

bool Vis::isBoxVisible(const glm::vec3 &mins, const glm::vec3 &maxs, const uint8_t *visbits) {
    short leafList[MAX_BOX_LEAFS];

    if (!visbits) {
        return true;
    }

    int count = boxLeafNums(mins, maxs, appfw::span(leafList, MAX_BOX_LEAFS));

    for (int i = 0; i < count; i++) {
        int leafnum = leafList[i];

        if (leafnum != -1 && visbits[leafnum >> 3] & (1 << (leafnum & 7)))
            return true;
    }
    return false;
}

int Vis::boxLeafNums(const glm::vec3 &mins, const glm::vec3 &maxs, appfw::span<short> list) {
    LeafList ll;
    ll.mins = mins;
    ll.maxs = maxs;
    ll.count = 0;
    ll.maxcount = (int)list.size();
    ll.list = list.data();
    ll.topnode = -1;
    ll.overflowed = false;

    boxLeafNums_r(ll, 0);

    return ll.count;
}

void Vis::boxLeafNums_r(LeafList &ll, int node) {
    for (;;) {
        if (node < 0) {
            int leafidx = ~node;
            const bsp::BSPLeaf *pLeaf = &m_pLevel->getLeaves()[leafidx];

            if (pLeaf->nContents == bsp::CONTENTS_SOLID)
                return;

            // it's a leaf!
            if (ll.count >= ll.maxcount) {
                ll.overflowed = true;
                return;
            }

            ll.list[ll.count] = (short)(leafidx - 1);
            ll.count++;
            return;
        }

        const bsp::BSPNode *pNode = &m_pLevel->getNodes()[node];
        const bsp::BSPPlane &plane = m_pLevel->getPlanes()[pNode->iPlane];
        int s = boxOnPlaneSide(ll.mins, ll.maxs, &plane);

        if (s == 1) {
            node = pNode->iChildren[0];
        } else if (s == 2) {
            node = pNode->iChildren[1];
        } else {
            // go down both
            if (ll.topnode == -1)
                ll.topnode = node;
            boxLeafNums_r(ll, pNode->iChildren[0]);
            node = pNode->iChildren[1];
        }
    }
}

void Vis::raycastRecursiveWorldNodes(int nodeIdx, const Ray &ray, SurfaceRaycastHit &hit) {
    if (nodeIdx < 0) {
        // Leaf
        return;
    }

    const bsp::BSPNode &node = m_pLevel->getNodes()[nodeIdx];
    const bsp::BSPPlane &plane = m_pLevel->getPlanes()[node.iPlane];

    float dot = planeDiff(ray.origin, plane);
    int side = (dot >= 0.0f) ? 0 : 1;

    // Front side
    raycastRecursiveWorldNodes(node.iChildren[side], ray, hit);

    // Check surfaces of current node
    for (unsigned i = 0; i < node.nFaces; i++) {
        unsigned surfIdx = node.firstFace + i;
        SurfaceRaycastHit testHit;

        if (rayIntersectsWithSurface(ray, (int)surfIdx, testHit, hit.distance) &&
            testHit.distance < hit.distance) {
            hit = testHit;
        }
    }

    // Back side
    raycastRecursiveWorldNodes(node.iChildren[!side], ray, hit);
}
