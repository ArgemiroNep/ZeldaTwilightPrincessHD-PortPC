// setenv / unsetenv for the unit tests on Windows (MinGW has neither; _putenv_s sets the C runtime's
// environment, which getenv reads, and an empty value removes the variable)
#pragma once
#include <cstdlib>
#ifdef _WIN32
inline int setenv(const char* name, const char* value, int overwrite) {
    if (!overwrite && std::getenv(name)) return 0;
    return _putenv_s(name, value);
}
inline int unsetenv(const char* name) { return _putenv_s(name, ""); }
#endif
