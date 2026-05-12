#include "auth.h"

namespace galay::mail::protocol
{
    std::string base64Encode(std::string_view value)
    {
        static constexpr char kTable[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string out;
        out.reserve(((value.size() + 2) / 3) * 4);

        size_t i = 0;
        while (i + 3 <= value.size()) {
            const auto a = static_cast<unsigned char>(value[i++]);
            const auto b = static_cast<unsigned char>(value[i++]);
            const auto c = static_cast<unsigned char>(value[i++]);
            out.push_back(kTable[a >> 2]);
            out.push_back(kTable[((a & 0x03) << 4) | (b >> 4)]);
            out.push_back(kTable[((b & 0x0F) << 2) | (c >> 6)]);
            out.push_back(kTable[c & 0x3F]);
        }

        const size_t rem = value.size() - i;
        if (rem == 1) {
            const auto a = static_cast<unsigned char>(value[i]);
            out.push_back(kTable[a >> 2]);
            out.push_back(kTable[(a & 0x03) << 4]);
            out.push_back('=');
            out.push_back('=');
        } else if (rem == 2) {
            const auto a = static_cast<unsigned char>(value[i]);
            const auto b = static_cast<unsigned char>(value[i + 1]);
            out.push_back(kTable[a >> 2]);
            out.push_back(kTable[((a & 0x03) << 4) | (b >> 4)]);
            out.push_back(kTable[(b & 0x0F) << 2]);
            out.push_back('=');
        }

        return out;
    }

    std::string authPlainPayload(std::string_view username, std::string_view password)
    {
        std::string raw;
        raw.reserve(username.size() + password.size() + 2);
        raw.push_back('\0');
        raw.append(username.data(), username.size());
        raw.push_back('\0');
        raw.append(password.data(), password.size());
        return base64Encode(raw);
    }

    std::string authLoginUsername(std::string_view username)
    {
        return base64Encode(username);
    }

    std::string authLoginPassword(std::string_view password)
    {
        return base64Encode(password);
    }
} // namespace galay::mail::protocol
