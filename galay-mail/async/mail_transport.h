#ifndef GALAY_MAIL_TRANSPORT_H
#define GALAY_MAIL_TRANSPORT_H

#include "galay-mail/base/mail_config.h"
#include <galay-kernel/async/tcp_socket.h>
#include <galay-kernel/kernel/task.h>
#include <galay-ssl/async/ssl_socket.h>
#include <galay-ssl/ssl/ssl_context.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace galay::mail::async
{
    class MailTransport
    {
    public:
        explicit MailTransport(AsyncMailConfig config = {});
        MailTransport(MailTransport&&) noexcept;
        MailTransport& operator=(MailTransport&&) noexcept;
        MailTransport(const MailTransport&) = delete;
        MailTransport& operator=(const MailTransport&) = delete;
        ~MailTransport();

        galay::kernel::Task<MailVoidResult> connect(const MailEndpoint& endpoint);
        galay::kernel::Task<MailVoidResult> startTls(const MailTlsConfig& tls_config,
                                                     std::string server_name);
        galay::kernel::Task<MailVoidResult> sendAll(std::string data);
        galay::kernel::Task<MailResult<std::string>> recvChunk();
        galay::kernel::Task<MailVoidResult> close();

        [[nodiscard]] bool connected() const noexcept { return m_state != State::Closed; }
        [[nodiscard]] bool encrypted() const noexcept { return m_state == State::Tls; }

    private:
        enum class State
        {
            Closed,
            Plain,
            Tls,
        };

        struct ResolvedHost
        {
            galay::kernel::Host host;
            galay::kernel::IPType ip_type = galay::kernel::IPType::IPV4;
        };

        static MailResult<ResolvedHost> resolveHost(const std::string& host, uint16_t port);
        static MailVoidResult configureContext(galay::ssl::SslContext& context,
                                               const MailTlsConfig& tls_config);
        MailVoidResult resetTlsSocket(galay::kernel::IPType ip_type,
                                      GHandle handle,
                                      const MailTlsConfig& tls_config,
                                      std::string_view server_name);

        AsyncMailConfig m_config;
        State m_state = State::Closed;
        std::optional<galay::async::TcpSocket> m_tcp;
        std::optional<galay::ssl::SslContext> m_ssl_context;
        std::optional<galay::ssl::SslSocket> m_ssl;
        galay::kernel::IPType m_ip_type = galay::kernel::IPType::IPV4;
        std::string m_server_name;
    };
} // namespace galay::mail::async

#endif
