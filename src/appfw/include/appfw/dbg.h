#ifndef APPFW_DBG_H
#define APPFW_DBG_H
#include <cassert>

namespace appfw {
namespace dbg {

// TODO: Replace with custom
#define AFW_ASSERT(cond) assert(cond)
#define AFW_ASSERT_MSG(cond, msg) assert(cond)

} // namespace dbg
} // namespace appfw

#endif
