#pragma once

// Sol2 Lua binding library configuration
// This file must be included BEFORE sol.hpp in any file that uses Sol2

// Disable Sol2's safe mode for faster compilation and smaller binaries
// This removes automatic type checking and safety assertions
// Trade-off: ~15-25s faster builds, but Lua errors may crash instead of returning errors
#define SOL_ALL_SAFETIES_ON 0

// Enable exceptions for better error handling
// We'll use try-catch blocks around Lua calls to prevent crashes
#define SOL_EXCEPTIONS_ALWAYS_UNSAFE 0
#define SOL_USING_CXX_LUA 0

// Optimization: disable some features we don't use
#define SOL_PRINT_ERRORS 0  // We'll handle errors manually
#define SOL_DEFAULT_PASS_ON_ERROR 0

// Keep stack trace for debugging
#define SOL_SAFE_STACK_CHECK 1
