#include "mail_url.h"

#include <charconv>
#include <cctype>
#include <optional>
#include <string>
#include <utility>

namespace galay::mail
{
    namespace
    {
        bool iequals(std::string_view lhs, std::string_view rhs)
        {
            if (lhs.size() != rhs.size()) {
                return false;
            }
            for (size_t i = 0; i < lhs.size(); ++i) {
                if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
                    std::tolower(static_cast<unsigned char>(rhs[i]))) {
                    return false;
                }
            }
            return true;
        }

        int hexValue(char ch)
        {
            if (ch >= '0' && ch <= '9') {
                return ch - '0';
            }
            if (ch >= 'a' && ch <= 'f') {
                return ch - 'a' + 10;
            }
            if (ch >= 'A' && ch <= 'F') {
                return ch - 'A' + 10;
            }
            return -1;
        }

        std::expected<uint16_t, MailError> parsePort(std::string_view value)
        {
            if (value.empty()) {
                return std::unexpected(MailError(MailErrorType::InvalidPort, "empty port"));
            }

            uint32_t port = 0;
            auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), port);
            if (ec != std::errc{} || ptr != value.data() + value.size() || port == 0 || port > 65535) {
                return std::unexpected(MailError(MailErrorType::InvalidPort, std::string(value)));
            }
            return static_cast<uint16_t>(port);
        }

        std::expected<std::pair<MailProtocol, MailSecurity>, MailError>
        schemeToMode(std::string_view scheme)
        {
            if (iequals(scheme, "smtp")) {
                return std::pair{MailProtocol::Smtp, MailSecurity::Plain};
            }
            if (iequals(scheme, "smtp+starttls")) {
                return std::pair{MailProtocol::Smtp, MailSecurity::StartTls};
            }
            if (iequals(scheme, "smtps")) {
                return std::pair{MailProtocol::Smtp, MailSecurity::Tls};
            }
            if (iequals(scheme, "pop3")) {
                return std::pair{MailProtocol::Pop3, MailSecurity::Plain};
            }
            if (iequals(scheme, "pop3+stls")) {
                return std::pair{MailProtocol::Pop3, MailSecurity::StartTls};
            }
            if (iequals(scheme, "pop3s")) {
                return std::pair{MailProtocol::Pop3, MailSecurity::Tls};
            }
            return std::unexpected(MailError(MailErrorType::InvalidScheme, std::string(scheme)));
        }

        uint16_t defaultPort(MailProtocol protocol, MailSecurity security)
        {
            if (protocol == MailProtocol::Smtp) {
                if (security == MailSecurity::Tls) {
                    return 465;
                }
                if (security == MailSecurity::StartTls) {
                    return 587;
                }
                return 25;
            }
            if (security == MailSecurity::Tls) {
                return 995;
            }
            return 110;
        }

        bool parseBool(std::string_view value)
        {
            return iequals(value, "true") || value == "1" || iequals(value, "yes");
        }

        void applyQuery(MailEndpoint& endpoint, std::string_view query)
        {
            while (!query.empty()) {
                const size_t amp = query.find('&');
                const std::string_view item = amp == std::string_view::npos
                                                  ? query
                                                  : query.substr(0, amp);
                query = amp == std::string_view::npos ? std::string_view{} : query.substr(amp + 1);
                if (item.empty()) {
                    continue;
                }

                const size_t eq = item.find('=');
                const std::string_view key = eq == std::string_view::npos ? item : item.substr(0, eq);
                const std::string value = eq == std::string_view::npos
                                              ? std::string{}
                                              : percentDecode(item.substr(eq + 1));
                if (key == "verify_peer") {
                    endpoint.tls.verify_peer = parseBool(value);
                } else if (key == "ca_path") {
                    endpoint.tls.ca_path = value;
                } else if (key == "server_name") {
                    endpoint.tls.server_name = value;
                }
            }
        }
    } // namespace

    std::string percentDecode(std::string_view value)
    {
        std::string out;
        out.reserve(value.size());
        for (size_t i = 0; i < value.size(); ++i) {
            if (value[i] == '%' && i + 2 < value.size()) {
                const int hi = hexValue(value[i + 1]);
                const int lo = hexValue(value[i + 2]);
                if (hi >= 0 && lo >= 0) {
                    out.push_back(static_cast<char>((hi << 4) | lo));
                    i += 2;
                    continue;
                }
            }
            out.push_back(value[i] == '+' ? ' ' : value[i]);
        }
        return out;
    }

    std::expected<MailEndpoint, MailError> parseMailUrl(std::string_view url)
    {
        const size_t scheme_end = url.find("://");
        if (scheme_end == std::string_view::npos || scheme_end == 0) {
            return std::unexpected(MailError(MailErrorType::InvalidUrl, "missing scheme"));
        }

        auto mode = schemeToMode(url.substr(0, scheme_end));
        if (!mode) {
            return std::unexpected(mode.error());
        }

        MailEndpoint endpoint;
        endpoint.protocol = mode->first;
        endpoint.security = mode->second;
        endpoint.port = defaultPort(endpoint.protocol, endpoint.security);

        std::string_view rest = url.substr(scheme_end + 3);
        const size_t query_pos = rest.find('?');
        const std::string_view authority = query_pos == std::string_view::npos
                                               ? rest
                                               : rest.substr(0, query_pos);
        const std::string_view query = query_pos == std::string_view::npos
                                           ? std::string_view{}
                                           : rest.substr(query_pos + 1);
        if (authority.empty()) {
            return std::unexpected(MailError(MailErrorType::InvalidHost, "empty authority"));
        }

        std::string_view host_port = authority;
        const size_t at = authority.rfind('@');
        if (at != std::string_view::npos) {
            const std::string_view userinfo = authority.substr(0, at);
            host_port = authority.substr(at + 1);
            const size_t colon = userinfo.find(':');
            endpoint.username = percentDecode(colon == std::string_view::npos ? userinfo : userinfo.substr(0, colon));
            if (colon != std::string_view::npos) {
                endpoint.password = percentDecode(userinfo.substr(colon + 1));
            }
        }

        if (host_port.empty()) {
            return std::unexpected(MailError(MailErrorType::InvalidHost, "empty host"));
        }

        if (host_port.front() == '[') {
            const size_t close = host_port.find(']');
            if (close == std::string_view::npos) {
                return std::unexpected(MailError(MailErrorType::InvalidHost, "unclosed ipv6 literal"));
            }
            endpoint.host = std::string(host_port.substr(1, close - 1));
            if (close + 1 < host_port.size()) {
                if (host_port[close + 1] != ':') {
                    return std::unexpected(MailError(MailErrorType::InvalidHost, "invalid ipv6 authority"));
                }
                auto port = parsePort(host_port.substr(close + 2));
                if (!port) {
                    return std::unexpected(port.error());
                }
                endpoint.port = *port;
            }
        } else {
            const size_t colon = host_port.rfind(':');
            if (colon != std::string_view::npos && host_port.find(':') == colon) {
                endpoint.host = std::string(host_port.substr(0, colon));
                auto port = parsePort(host_port.substr(colon + 1));
                if (!port) {
                    return std::unexpected(port.error());
                }
                endpoint.port = *port;
            } else {
                endpoint.host = std::string(host_port);
            }
        }

        if (endpoint.host.empty()) {
            return std::unexpected(MailError(MailErrorType::InvalidHost, "empty host"));
        }

        endpoint.tls.server_name = endpoint.host;
        applyQuery(endpoint, query);
        return endpoint;
    }
} // namespace galay::mail
