#ifndef GALAY_MAIL_CONFIG_H
#define GALAY_MAIL_CONFIG_H

#include <chrono>
#include <cstdint>
#include <expected>
#include <string>
#include "mail_error.h"

namespace galay::mail
{
    enum class MailProtocol
    {
        Smtp,
        Pop3,
    };

    enum class MailSecurity
    {
        Plain,
        Tls,
        StartTls,
    };

    struct MailTlsConfig
    {
        std::string ca_path;
        std::string server_name;
        bool verify_peer = false;
        int verify_depth = 4;
    };

    struct MailEndpoint
    {
        MailProtocol protocol = MailProtocol::Smtp;
        MailSecurity security = MailSecurity::Plain;
        std::string host;
        uint16_t port = 0;
        std::string username;
        std::string password;
        MailTlsConfig tls;
    };

    struct AsyncMailConfig
    {
        std::chrono::milliseconds connect_timeout{5000};
        std::chrono::milliseconds send_timeout{5000};
        std::chrono::milliseconds recv_timeout{5000};
        size_t buffer_size = 8192;

        static AsyncMailConfig noTimeout()
        {
            AsyncMailConfig config;
            config.connect_timeout = std::chrono::milliseconds::zero();
            config.send_timeout = std::chrono::milliseconds::zero();
            config.recv_timeout = std::chrono::milliseconds::zero();
            return config;
        }
    };

    template <typename T>
    using MailResult = std::expected<T, MailError>;

    using MailVoidResult = std::expected<void, MailError>;
} // namespace galay::mail

#endif
