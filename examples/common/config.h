#ifndef GALAY_MAIL_EXAMPLES_CONFIG_H
#define GALAY_MAIL_EXAMPLES_CONFIG_H

#include <cstdlib>
#include <string>

inline std::string envOrEmpty(const char* name)
{
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string{};
}

#endif
