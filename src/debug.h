#pragma once

// Toggles global debug
// #define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define debug(x) (x)
#else
#define debug(x)
#endif
