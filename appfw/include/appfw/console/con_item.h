#ifndef APPFW_CONSOLE_CON_ITEM_H
#define APPFW_CONSOLE_CON_ITEM_H
#include <forward_list>
#include <functional>
#include <string>

#include <appfw/utils.h>

namespace appfw {

namespace console {

class ConsoleSystem;

/**
 * An enumeration of all posible types of a console item.
 */
enum class ConItemType {
    None,
    ConVar,
    ConCommand,
    Any
};

enum class VarSetResult {
    Success = 0,
    InvalidString,
    CallbackRejected,
    Locked,
};

//----------------------------------------------------------------

/**
 * A base class for all console items.
 * Has a name and description (as const char *) and a specific type. 
 */
class ConItemBase {
public:
    /**
     * Construct a console item.
     * @param name    Name of the item.
     * @param descr   Description of the item (can be nullptr).
     */
    ConItemBase(const char *name, const char *descr);
    ConItemBase(const ConItemBase &) = delete;
    ConItemBase &operator=(const ConItemBase &) = delete;
    virtual ~ConItemBase() = default;

    /**
     * Returns name of this console item.
     */
    inline const char *getName() { return m_pName; }

    /**
     * Returns description of this console item.
     */
    inline const char *getDescr() { return m_pDescr; }

    /**
     * Returns type of the console item.
     */
    virtual ConItemType getType() = 0;

private:
    const char *m_pName = nullptr;
    const char *m_pDescr = nullptr;

    /**
     * Returns a reference to a list of unregistered items.
     */
    static std::forward_list<ConItemBase *> &getUnregItems();

    friend class ConsoleSystem;
};

//----------------------------------------------------------------

/**
 * A base (typeless) class for console variables.
 */
class ConVarBase : public ConItemBase {
public:
    using ConItemBase::ConItemBase;

    /**
     * Returns whether cvar is locked or not.
     * When cvar is locked, it's value can't be changed.
     */
    bool isLocked();

    /**
     * Locks or unlocks the cvar.
     */
    void setLocked(bool state);

    /**
     * Returns string value of the convar.
     */
    virtual std::string getValueAsString() = 0;

    /**
     * Sets a string value.
     * @return Success or failure (invalid string or callback rejected).
     */
    virtual VarSetResult setValueString(const std::string &val) = 0;

    /**
     * Returns type of the cvar as a string (e.g. float, int).
     */
    virtual const char *getVarTypeAsString() = 0;

    virtual ConItemType getType() override;

private:
    bool m_bIsLocked = false;
};

//----------------------------------------------------------------

template <typename T>
class ConVar : public ConVarBase {
public:
    using Callback = std::function<bool(const T &oldVal, const T &newVal)>;

    ConVar(const char *name, const T &defValue, const char *descr, Callback cb = Callback());

    /**
     * Returns value of the convar.
     */
    const T &getValue();

    /**
     * Sets a new value of the cvar.
     * @return Whether or nos new value was applied.
     */
    VarSetResult setValue(const T &newVal);

    virtual std::string getValueAsString() override;
    virtual VarSetResult setValueString(const std::string &val) override;
    virtual const char *getVarTypeAsString() override;

private:
    T m_Value;
    Callback m_Callback;
};

//----------------------------------------------------------------

class ConCommand : public ConItemBase {
public:
    using Callback = std::function<void(const ParsedCommand &args)>;

    ConCommand(const char *name, const char *descr, const Callback &callback);

    /**
     * Runs the command.
     */
    void execute(const ParsedCommand &args);

    virtual ConItemType getType() override;

private:
    Callback m_Callback;
};

} // namespace console

} // namespace appfw

#endif
