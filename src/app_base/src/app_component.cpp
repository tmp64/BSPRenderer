#include <app_base/app_component.h>
#include <app_base/app_base.h>

AppComponent::AppComponent() {
    AppBase::ComponentSubsystem::m_spInstance->addComponent(this);
}

AppComponent::~AppComponent() = default;

void AppComponent::lateInit() {}

void AppComponent::tick() {}

void AppComponent::lateTick() {}

void AppComponent::setTickEnabled(bool state) {
    if (state == isTickEnabled()) {
        return;
    }

    if (state) {
        m_iFlags |= FLAG_TICK;
    } else {
        m_iFlags &= ~FLAG_TICK;
    }

    AppBase::ComponentSubsystem::m_spInstance->markTickListAsDirty();
}
