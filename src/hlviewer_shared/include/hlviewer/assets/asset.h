#ifndef ASSETS_ASSET_H
#define ASSETS_ASSET_H
#include "asset_future.h"

class AssetRefBase;
class AssetManager;

//! Base class for an asset (level, model, sprite, etc)
//! Some assets may use reference counting.
class Asset {
public:
    //! @returns the path to the asset. Some paths may not be real (e.g. texture in a WAD).
    inline const std::string &getPath() const { return m_Path; }

protected:
    std::string m_Path;

private:
    unsigned m_uReferenceCount;
    friend class AssetRefBase;
    friend class AssetManager;
};

//! Base class for an asset reference.
class AssetRefBase {
public:
    AssetRefBase() = default;
    AssetRefBase(nullptr_t) {};
    explicit AssetRefBase(Asset *asset) { set(asset); }
    ~AssetRefBase() { reset(); }
    AssetRefBase(const AssetRefBase &other) noexcept { set(other.m_pAsset); }
    AssetRefBase(AssetRefBase &&other) noexcept {
        m_pAsset = other.m_pAsset;
        other.m_pAsset = nullptr;
    }

    AssetRefBase &operator=(const AssetRefBase &other) noexcept {
        if (&other != this && other.m_pAsset != m_pAsset) {
            reset();
            set(other.m_pAsset);
        }

        return *this;
    }

    AssetRefBase &operator=(AssetRefBase &&other) noexcept {
        if (&other != this) {
            reset();
            m_pAsset = other.m_pAsset;
            other.m_pAsset = nullptr;
        }

        return *this;
    }

    void set(Asset *asset) noexcept {
        if (m_pAsset == asset) {
            return;
        }

        reset();

        if (asset) {
            m_pAsset = asset;
            asset->m_uReferenceCount++;
        }
    }

    void reset() noexcept {
        if (m_pAsset) {
            m_pAsset->m_uReferenceCount--;
            m_pAsset = nullptr;
        }
    }

    operator bool() noexcept { return m_pAsset; }

protected:
    Asset *m_pAsset = nullptr;
};

//! An asset reference.
template <typename T>
class AssetRef : public AssetRefBase {
public:
    using AssetRefBase::AssetRefBase;

    T *get() { return static_cast<T *>(m_pAsset); }
    T &operator*() { return static_cast<T &>(*m_pAsset); }
    T *operator->() { return static_cast<T *>(m_pAsset); }
};

#endif
