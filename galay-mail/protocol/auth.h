#ifndef GALAY_MAIL_PROTOCOL_AUTH_H
#define GALAY_MAIL_PROTOCOL_AUTH_H

#include <string>
#include <string_view>

namespace galay::mail::protocol
{
    std::string base64Encode(std::string_view value);
    std::string authPlainPayload(std::string_view username, std::string_view password);
    std::string authLoginUsername(std::string_view username);
    std::string authLoginPassword(std::string_view password);
} // namespace galay::mail::protocol

#endif
