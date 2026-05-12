#include "mail_transport.h"

#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <utility>

namespace galay::mail::async
{
    namespace
    {
        MailError mapIoError(const galay::kernel::IOError& error, MailErrorType fallback)
        {
            if (galay::kernel::IOError::contains(error.code(), galay::kernel::kTimeout)) {
                return MailError(MailErrorType::Timeout, error.message());
            }
            if (galay::kernel::IOError::contains(error.code(), galay::kernel::kDisconnectError)) {
                return MailError(MailErrorType::ConnectionClosed, error.message());
            }
            return MailError(fallback, error.message());
        }

        MailError mapSslError(const galay::ssl::SslError& error, MailErrorType fallback)
        {
            using galay::ssl::SslErrorCode;
            switch (error.code()) {
            case SslErrorCode::kTimeout:
            case SslErrorCode::kHandshakeTimeout:
                return MailError(MailErrorType::Timeout, error.message());
            case SslErrorCode::kPeerClosed:
                return MailError(MailErrorType::ConnectionClosed, error.message());
            case SslErrorCode::kVerificationFailed:
            case SslErrorCode::kHandshakeFailed:
            case SslErrorCode::kSNISetFailed:
                return MailError(MailErrorType::TlsError, error.message());
            default:
                return MailError(fallback, error.message());
            }
        }
    } // namespace

    MailTransport::MailTransport(AsyncMailConfig config)
        : m_config(std::move(config))
    {
    }

    MailTransport::MailTransport(MailTransport&&) noexcept = default;
    MailTransport& MailTransport::operator=(MailTransport&&) noexcept = default;
    MailTransport::~MailTransport() = default;

    MailResult<MailTransport::ResolvedHost>
    MailTransport::resolveHost(const std::string& host, uint16_t port)
    {
        addrinfo hints{};
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_family = AF_UNSPEC;

        addrinfo* result = nullptr;
        const std::string service = std::to_string(port);
        const int rc = ::getaddrinfo(host.c_str(), service.c_str(), &hints, &result);
        if (rc != 0 || result == nullptr) {
            return std::unexpected(MailError(MailErrorType::InvalidHost, ::gai_strerror(rc)));
        }

        std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> holder(result, &::freeaddrinfo);
        for (addrinfo* it = result; it != nullptr; it = it->ai_next) {
            if (it->ai_family == AF_INET) {
                auto* addr = reinterpret_cast<sockaddr_in*>(it->ai_addr);
                return ResolvedHost{galay::kernel::Host(*addr), galay::kernel::IPType::IPV4};
            }
            if (it->ai_family == AF_INET6) {
                auto* addr = reinterpret_cast<sockaddr_in6*>(it->ai_addr);
                return ResolvedHost{galay::kernel::Host(*addr), galay::kernel::IPType::IPV6};
            }
        }

        return std::unexpected(MailError(MailErrorType::InvalidHost, host));
    }

    MailVoidResult MailTransport::configureContext(galay::ssl::SslContext& context,
                                                   const MailTlsConfig& tls_config)
    {
        if (!context.isValid()) {
            return std::unexpected(MailError(MailErrorType::TlsError, context.error().message()));
        }

        if (!tls_config.ca_path.empty()) {
            auto loaded = context.loadCACertificate(tls_config.ca_path);
            if (!loaded) {
                return std::unexpected(mapSslError(loaded.error(), MailErrorType::TlsError));
            }
        } else if (tls_config.verify_peer) {
            auto loaded = context.useDefaultCA();
            if (!loaded) {
                return std::unexpected(mapSslError(loaded.error(), MailErrorType::TlsError));
            }
        }

        if (tls_config.verify_peer) {
            context.setVerifyMode(galay::ssl::SslVerifyMode::Peer);
            context.setVerifyDepth(tls_config.verify_depth);
        } else {
            context.setVerifyMode(galay::ssl::SslVerifyMode::None);
        }
        return {};
    }

    MailVoidResult MailTransport::resetTlsSocket(galay::kernel::IPType ip_type,
                                                 GHandle handle,
                                                 const MailTlsConfig& tls_config,
                                                 std::string_view server_name)
    {
        m_ssl.reset();
        m_ssl_context.reset();
        m_ssl_context.emplace(galay::ssl::SslMethod::TLS_Client);

        auto configured = configureContext(*m_ssl_context, tls_config);
        if (!configured) {
            return configured;
        }

        try {
            if (handle == GHandle::invalid()) {
                m_ssl.emplace(&*m_ssl_context, ip_type);
            } else {
                m_ssl.emplace(&*m_ssl_context, handle);
            }
        } catch (const std::exception& e) {
            return std::unexpected(MailError(MailErrorType::TlsError, e.what()));
        }

        auto non_block = m_ssl->option().handleNonBlock();
        if (!non_block) {
            return std::unexpected(mapIoError(non_block.error(), MailErrorType::ConnectionError));
        }

        if (!server_name.empty()) {
            auto sni = m_ssl->setHostname(std::string(server_name));
            if (!sni) {
                return std::unexpected(mapSslError(sni.error(), MailErrorType::TlsError));
            }
        }

        m_ip_type = ip_type;
        m_server_name = std::string(server_name);
        return {};
    }

    galay::kernel::Task<MailVoidResult> MailTransport::connect(const MailEndpoint& endpoint)
    {
        auto resolved = resolveHost(endpoint.host, endpoint.port);
        if (!resolved) {
            co_return std::unexpected(resolved.error());
        }

        (void)co_await close();
        m_ip_type = resolved->ip_type;
        m_server_name = endpoint.tls.server_name.empty() ? endpoint.host : endpoint.tls.server_name;

        if (endpoint.security == MailSecurity::Tls) {
            auto reset = resetTlsSocket(resolved->ip_type,
                                        GHandle::invalid(),
                                        endpoint.tls,
                                        m_server_name);
            if (!reset) {
                co_return std::unexpected(reset.error());
            }

            auto connected = co_await m_ssl->connect(resolved->host).timeout(m_config.connect_timeout);
            if (!connected) {
                co_return std::unexpected(mapIoError(connected.error(), MailErrorType::ConnectionError));
            }

            auto handshake = co_await m_ssl->handshake().timeout(m_config.connect_timeout);
            if (!handshake) {
                co_return std::unexpected(mapSslError(handshake.error(), MailErrorType::TlsError));
            }

            m_state = State::Tls;
            co_return MailVoidResult{};
        }

        try {
            m_tcp.emplace(resolved->ip_type);
        } catch (const std::exception& e) {
            co_return std::unexpected(MailError(MailErrorType::ConnectionError, e.what()));
        }

        auto non_block = m_tcp->option().handleNonBlock();
        if (!non_block) {
            co_return std::unexpected(mapIoError(non_block.error(), MailErrorType::ConnectionError));
        }

        auto connected = co_await m_tcp->connect(resolved->host).timeout(m_config.connect_timeout);
        if (!connected) {
            co_return std::unexpected(mapIoError(connected.error(), MailErrorType::ConnectionError));
        }

        m_state = State::Plain;
        co_return MailVoidResult{};
    }

    galay::kernel::Task<MailVoidResult>
    MailTransport::startTls(const MailTlsConfig& tls_config, std::string server_name)
    {
        if (m_state != State::Plain || !m_tcp.has_value()) {
            co_return std::unexpected(MailError(MailErrorType::InvalidState, "plain connection required"));
        }

        auto handle = m_tcp->handle();
        auto reset = resetTlsSocket(m_ip_type, handle, tls_config, server_name);
        if (!reset) {
            co_return std::unexpected(reset.error());
        }

        auto handshake = co_await m_ssl->handshake().timeout(m_config.connect_timeout);
        if (!handshake) {
            co_return std::unexpected(mapSslError(handshake.error(), MailErrorType::TlsError));
        }

        m_tcp.reset();
        m_state = State::Tls;
        co_return MailVoidResult{};
    }

    galay::kernel::Task<MailVoidResult> MailTransport::sendAll(std::string data)
    {
        size_t sent = 0;
        while (sent < data.size()) {
            if (m_state == State::Plain && m_tcp.has_value()) {
                auto result = co_await m_tcp->send(data.data() + sent, data.size() - sent)
                                  .timeout(m_config.send_timeout);
                if (!result) {
                    co_return std::unexpected(mapIoError(result.error(), MailErrorType::SendError));
                }
                if (result.value() == 0) {
                    co_return std::unexpected(MailError(MailErrorType::ConnectionClosed));
                }
                sent += result.value();
            } else if (m_state == State::Tls && m_ssl.has_value()) {
                auto result = co_await m_ssl->send(data.data() + sent, data.size() - sent);
                if (!result) {
                    co_return std::unexpected(mapSslError(result.error(), MailErrorType::SendError));
                }
                if (result.value() == 0) {
                    co_return std::unexpected(MailError(MailErrorType::ConnectionClosed));
                }
                sent += result.value();
            } else {
                co_return std::unexpected(MailError(MailErrorType::InvalidState, "not connected"));
            }
        }
        co_return MailVoidResult{};
    }

    galay::kernel::Task<MailResult<std::string>> MailTransport::recvChunk()
    {
        std::string buffer(m_config.buffer_size, '\0');
        if (m_state == State::Plain && m_tcp.has_value()) {
            auto result = co_await m_tcp->recv(buffer.data(), buffer.size())
                              .timeout(m_config.recv_timeout);
            if (!result) {
                co_return std::unexpected(mapIoError(result.error(), MailErrorType::RecvError));
            }
            if (result.value() == 0) {
                co_return std::unexpected(MailError(MailErrorType::ConnectionClosed));
            }
            buffer.resize(result.value());
            co_return buffer;
        }

        if (m_state == State::Tls && m_ssl.has_value()) {
            auto result = co_await m_ssl->recv(buffer.data(), buffer.size());
            if (!result) {
                co_return std::unexpected(mapSslError(result.error(), MailErrorType::RecvError));
            }
            if (result->empty()) {
                co_return std::unexpected(MailError(MailErrorType::ConnectionClosed));
            }
            co_return result->toString();
        }

        co_return std::unexpected(MailError(MailErrorType::InvalidState, "not connected"));
    }

    galay::kernel::Task<MailVoidResult> MailTransport::close()
    {
        if (m_ssl.has_value()) {
            (void)co_await m_ssl->shutdown();
            (void)co_await m_ssl->close();
            m_ssl.reset();
        }
        if (m_tcp.has_value()) {
            (void)co_await m_tcp->close();
            m_tcp.reset();
        }
        m_state = State::Closed;
        co_return MailVoidResult{};
    }
} // namespace galay::mail::async
