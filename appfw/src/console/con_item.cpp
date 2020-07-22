#include <appfw/console/con_item.h>
#include <appfw/dbg.h>
#include <appfw/utils.h>

//----------------------------------------------------------------
// ConItemBase
//----------------------------------------------------------------
appfw::console::ConItemBase::ConItemBase(const char *name, const char *descr) {
    AFW_ASSERT(name);
    m_pName = name;
    m_pDescr = descr ? descr : "";

    getUnregItems().push_front(this);
}

std::forward_list<appfw::console::ConItemBase *> &appfw::console::ConItemBase::getUnregItems() {
    static std::forward_list<appfw::console::ConItemBase *> list;
    return list;
}

//----------------------------------------------------------------
// ConVarBase
//----------------------------------------------------------------
bool appfw::console::ConVarBase::isLocked() { return m_bIsLocked; }

void appfw::console::ConVarBase::setLocked(bool state) { m_bIsLocked = state; }

appfw::console::ConItemType appfw::console::ConVarBase::getType() { return ConItemType::ConVar; }

//----------------------------------------------------------------
// ConVar
//----------------------------------------------------------------
template <typename T>
appfw::console::ConVar<T>::ConVar(const char *name, const T &defValue, const char *descr)
    : ConVarBase(name, descr) {
    setValue(defValue);
}

template <typename T>
const T &appfw::console::ConVar<T>::getValue() {
    return m_Value;
}

template <typename T>
appfw::console::VarSetResult appfw::console::ConVar<T>::setValue(const T &newVal) {
    if (isLocked()) {
        return VarSetResult::Locked;
    }

    if (m_Callback) {
        if (!m_Callback(m_Value, newVal)) {
            return VarSetResult::CallbackRejected;
        }
    }

    m_Value = newVal;
    return VarSetResult::Success;
}

template <typename T>
inline std::string appfw::console::ConVar<T>::getValueAsString() {
    return utils::valToString(m_Value);
}

template <typename T>
appfw::console::VarSetResult appfw::console::ConVar<T>::setValueString(const std::string &val) {
    T newVal;
    if (!utils::stringToVal(val, newVal)) {
        return VarSetResult::InvalidString;
    }

    return setValue(newVal);
}

template <typename T>
const char *appfw::console::ConVar<T>::getVarTypeAsString() {
    return utils::typeNameToString<T>();
}

template class appfw::console::ConVar<bool>;
template class appfw::console::ConVar<int>;
template class appfw::console::ConVar<float>;
template class appfw::console::ConVar<std::string>;

appfw::console::ConCommand::ConCommand(const char *name, const char *descr, const Callback &callback)
    : ConItemBase(name, descr) {
    AFW_ASSERT_MSG(callback, "ConCommand callback is invalid");
    m_Callback = callback;
}

void appfw::console::ConCommand::execute(const ParsedCommand &args) { m_Callback(args); }

appfw::console::ConItemType appfw::console::ConCommand::getType() { return ConItemType::ConCommand; }
