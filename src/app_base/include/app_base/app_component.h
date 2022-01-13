#ifndef APP_COMPONENT_H
#define APP_COMPONENT_H
#include <appfw/appfw.h>

//! A component of an application.
//! It is always a singleton.
class AppComponent : appfw::NoMove {
public:
    AppComponent();
    virtual ~AppComponent();

    //! Returns whether ticking is enabled for this component.
    inline bool isTickEnabled() { return m_iFlags & FLAG_TICK; }

    //! Called after all components have been constructed and are available.
    virtual void lateInit();

    //! Called from the main loop every tick in the initialization order.
    virtual void tick();

    //! Called from the main loop every tick but after all tick()s have been called.
    virtual void lateTick();

protected:
    //! Enables/disables ticking for this component.
    //! A call to this method will cause tick list to be rebuilt next tick (which is O(n), n - total
    //! number of components).
    void setTickEnabled(bool state);

private:
    static constexpr unsigned FLAG_TICK = 1 << 0;
    unsigned m_iFlags = 0;
};

//! Base class for any components. Provides a way to get an instance of the component.
template <typename T>
class AppComponentBase : public AppComponent {
public:
    //! Returns the singleton instance of the component.
    //! If isInitialized() == false, behavior is undefined.
    static inline T &get() { return static_cast<T &>(*m_spInstance); }

    //! Returns whether the component is initialized.
    static bool isInitialized() { return m_spInstance != nullptr; }

    inline AppComponentBase() {
        AFW_ASSERT(!m_spInstance);
        m_spInstance = this;
    }

    inline ~AppComponentBase() {
        AFW_ASSERT(m_spInstance == this);
        m_spInstance = nullptr;
    }

private:
    static inline AppComponent *m_spInstance = nullptr;
};

#endif
