App Framework (appfw)
=====================

This is a C++ library that provides some features:
- [ ] Compiler-specific extentions
- [ ] Command line argument parsing
- [x] Console variables and commands
- [x] Command buffer
- [x] Logging

Most of the stuff is documented in header files (*/src/appfw/include/appfw/*).


Console commands
----------------
- `list` Lists all available console items.
- `help` Shows info about a command.
- `appfw_debug_lock_cvar` Locks/unlocks a convar.


Usage
-----

Before usage you need to create `this_module_info.h` with following content:
```cpp
#ifndef MODULE_INFO_H
#define MODULE_INFO_H

/**
 * A unique name of the module.
 */
#define MODULE_NAME "name_of_the_modult"

/**
 * A unique namespace of the module.
 * Must not be used anywhere else.
 * It will be `using namespace`'d in module_types.h.
 */
#define MODULE_NAMESPACE module_name_of_the_modult

#endif
```

Library needs to be initialized before usage.
```cpp
#include <appfw/init.h>

static bool s_bIsRunning;

int main(int argc, char **argv) {
	// Init takes appfw::InitOptions defined in <appfw/init.h>.
	// Default options specified there as well.
	appfw::init::init();
	
	s_bIsRunning = true;
	while (s_bIsRunning) {
		// Make sure to call that somewhere in the main loop to process console IO.
		// It needs to always be called from the main thread.
		appfw::init::mainLoopTick();
	}
	
	// When you're done call to clean up.
	appfw::init::shutdown();
}
```

Then you can access things by including *<appfw/services.h>*. See that file for more information.

Examples:
```cpp

logInfo("An information message. Printed in default color.");	// \n is not required
logError("An error message. Printed in bright red.");

ConVar<int> test_cvar("test_cvar", -1, "A test convar");
```
