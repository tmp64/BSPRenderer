#include <rad/rad_sim.h>
#include "rad_sim_impl.h"

rad::RadSim::RadSim(int threadCount) {
    m_Impl = std::make_unique<RadSimImpl>(threadCount);
}

rad::RadSim::~RadSim() {}

void rad::RadSim::setAppConfig(AppConfig *appcfg) {
    m_Impl->setAppConfig(appcfg);
}

void rad::RadSim::setProgressCallback(const ProgressCallback &cb) {
    m_Impl->setProgressCallback(cb);
}

const bsp::Level *rad::RadSim::getLevel() {
    return m_Impl->getLevel();
}

void rad::RadSim::setLevel(const bsp::Level *pLevel, const std::string &name,
                           const std::string &profileName) {
    m_Impl->setLevel(pLevel, name, profileName);
}

const rad::BuildProfile &rad::RadSim::getBuildProfile() {
    return m_Impl->getBuildProfile();
}

void rad::RadSim::setBounceCount(int count) {
    m_Impl->setBounceCount(count);
}

appfw::SHA256::Digest rad::RadSim::getPatchHash() {
    return m_Impl->getPatchHash();
}

bool rad::RadSim::isVisMatValid() {
    return m_Impl->isVisMatValid();
}

bool rad::RadSim::loadVisMat() {
    return m_Impl->loadVisMat();
}

void rad::RadSim::calcVisMat() {
    m_Impl->calcVisMat();
}

bool rad::RadSim::isVFListValid() {
    return m_Impl->isVFListValid();
}

void rad::RadSim::calcViewFactors() {
    m_Impl->calcViewFactors();
}

void rad::RadSim::bounceLight() {
    m_Impl->bounceLight();
}

void rad::RadSim::writeLightmaps() {
    m_Impl->writeLightmaps();
}
