#ifndef HLVIEWER_LEVEL_VIEW_RENDERER_IFACE_H
#define HLVIEWER_LEVEL_VIEW_RENDERER_IFACE_H

class ILevelViewRenderer {
public:
    virtual ~ILevelViewRenderer() = default;

    //! Sets the intensity scale of a lightstyle.
    virtual void setLightstyleScale(int style, float scale) = 0;
};

#endif
