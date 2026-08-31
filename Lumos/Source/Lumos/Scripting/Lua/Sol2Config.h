#pragma once


#define SOL_ALL_SAFETIES_ON 0

#define SOL_EXCEPTIONS_ALWAYS_UNSAFE 0
#define SOL_USING_CXX_LUA 0

// Optimization: disable some features we don't use
#define SOL_PRINT_ERRORS 0  // We'll handle errors manually
#define SOL_DEFAULT_PASS_ON_ERROR 0

// Keep stack trace for debugging
#define SOL_SAFE_STACK_CHECK 1
