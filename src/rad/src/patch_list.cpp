#include "patch_list.h"

void rad::PatchList::allocate(PatchIndex size) {
    AFW_ASSERT(m_iSize == 0);
    m_iSize = size;
    m_flSize.resize(size);
    m_vFaceOrigin.resize(size);
    m_vOrigin.resize(size);
    m_vNormal.resize(size);
    m_pPlane.resize(size);
    m_Reflectivity.resize(size);
    m_FinalColor.resize(size);
}

void rad::PatchList::clear() {
    m_iSize = 0;
    m_flSize.clear();
    m_vOrigin.clear();
    m_vFaceOrigin.clear();
    m_vNormal.clear();
    m_pPlane.clear();
    m_Reflectivity.clear();
    m_FinalColor.clear();
}

rad::PatchRef rad::PatchList::Iterator::operator*() { return PatchRef(*m_pList, m_iIndex); }
