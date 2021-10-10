#include <renderer/utils.h>
#include "vis.h"

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

Vis::Vis() {
    AFW_ASSERT(!m_spInstance);
    m_spInstance = this;
    std::fill(m_VisBuf.begin(), m_VisBuf.end(), (uint8_t)0);
}

Vis::~Vis() {
    AFW_ASSERT(m_spInstance == this);
    m_spInstance = nullptr;
}

void Vis::setLevel(bsp::Level *pLevel) {
    m_pLevel = pLevel;
}

bool Vis::boxInPvs(const glm::vec3 &origin, const glm::vec3 &mins, const glm::vec3 &maxs) {
    int leaf = m_pLevel->pointInLeaf(origin);
    const uint8_t *vis = m_pLevel->leafPVS(leaf, m_VisBuf);

    return isBoxVisible(mins, maxs, vis);
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
