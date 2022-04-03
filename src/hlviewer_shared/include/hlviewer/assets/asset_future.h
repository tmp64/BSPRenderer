#ifndef ASSET_FUTURE_H
#define ASSET_FUTURE_H
#include <atomic>
#include <future>
#include <memory>
#include <appfw/utils.h>

//! Default shared state class for AssetFuture
struct AssetFutureSharedState {};

template <class T, class State = AssetFutureSharedState>
class AssetFuture : public std::future<T> {
public:
    AssetFuture() = default;
    AssetFuture(AssetFuture &&other) noexcept
        : std::future<T>(std::move(other)) {
        m_SharedState = std::move(other.m_SharedState);
    }

    AssetFuture &operator=(AssetFuture &&other) noexcept {
        if (&other != this) {
            std::future<T>::operator=(std::move(other));
            m_SharedState = std::move(other.m_SharedState);
        }

        return *this;
    }

    //! Returns the shared state
    std::shared_ptr<State> getState() { return m_SharedState; }

    //! Initializes the shared state.
    //! @returns the shared state
    std::shared_ptr<State> initSharedState() {
        m_SharedState = std::make_shared<State>();
        return m_SharedState;
    }

    //! Sets the future.
    void setFuture(std::future<T> &&future) {
        AFW_ASSERT(&future != this);
        AFW_ASSERT(m_SharedState);
        AFW_ASSERT(!std::future<T>::valid());
        std::future<T>::operator=(std::move(future));
    }

    //! Returns whether the future is ready.
    bool isReady() { return appfw::isFutureReady(*this); }

    //! Returns the result or crashes if no result is available.
    T get() {
        AFW_ASSERT_REL_MSG(isReady(), "Future would block");
        return std::future<T>::get();
    }

private:
    std::shared_ptr<State> m_SharedState;
};


#endif
