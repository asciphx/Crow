
#pragma once
#define DEFAULT_ENABLE_LOGGING
#define DEFAULT_LOG_LEVEL 3
#define DEFAULT_PORT 80
#if defined(_MSC_VER)
#if _MSC_VER < 1900
#define MSVC_WORKAROUND
#define constexpr const
#define noexcept throw()
#endif
#endif
