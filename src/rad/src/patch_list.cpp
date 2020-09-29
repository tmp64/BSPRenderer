#include "patch_list.h"

PatchList g_Patches;

void PatchList::allocate(size_t size) {
    AFW_ASSERT(m_iSize == 0);
    m_iSize = size;
    m_flSize.resize(size);
    m_vOrigin.resize(size);
    m_vNormal.resize(size);
    m_pPlane.resize(size);
    m_FinalColor.resize(size);
    m_pLMPixel.resize(size);
    m_ViewFactors.resize(size);
}

void PatchList::clear() {
    m_iSize = 0;
    m_flSize.clear();
    m_vOrigin.clear();
    m_vNormal.clear();
    m_pPlane.clear();
    m_FinalColor.clear();
    m_pLMPixel.clear();
    m_ViewFactors.clear();
}

PatchRef PatchList::Iterator::operator*() { return PatchRef(m_iIndex); }
