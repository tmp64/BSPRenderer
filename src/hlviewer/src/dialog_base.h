#ifndef DIALOG_BASE_H
#define DIALOG_BASE_H
#include <string_view>

class DialogBase {
public:
    DialogBase();
    virtual ~DialogBase();

    //! Called every frame
    virtual void tick();

    //! Called before rendering ImGui.
    virtual void preRender();

    //! Brings the dialog to front.
    inline void bringToFront() { m_bBringToFront = true; }

    //! @returns the dialog title.
    inline const std::string &getTitle() const { return m_Title; }

    //! @returns whether the dialog is open.
    inline bool isOpen() const { return m_bIsOpen; }

    //! Sets the dialog title
    void setTitle(std::string_view title);

    //! @returns the unique index.
    inline unsigned getUniqueId() const { return m_uUniqueId; }

protected:
    ImGuiWindowFlags m_WindowFlags = 0;
    bool m_bIsOpen = true;

    //! @returns whether the contents were visible last frame. Updated during tick().
    inline bool isContentVisible() const { return m_bContentVisible; }

    //! Shows the dialog contents.
    //! ImGui::Begin is called by outside code.
    virtual void showContents() = 0;

private:
    std::string m_Title;
    std::string m_TitleID;
    unsigned m_uUniqueId = 0;
    bool m_bContentVisible = false;
    bool m_bBringToFront = false;
    glm::vec2 m_DefaultSize = glm::vec2(1600, 900);

    static inline unsigned m_suNextUniqueId = 0;
};

#endif
