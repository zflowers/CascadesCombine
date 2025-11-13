#include "ValidationTools.h"

// Static object ensures SafeDiv and SafeIndex are registered at program startup
struct SafeHelpersRegistrar {
    SafeHelpersRegistrar() {
        RegisterSafeHelpers();
    }
};

// Instantiate a single global object
static SafeHelpersRegistrar g_safeHelpersRegistrar;

