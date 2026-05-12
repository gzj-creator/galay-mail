#include "smtp_client.h"

#include "galay-mail/protocol/auth.h"

#include <utility>

namespace galay::mail::async
{
    SmtpClientBuilder& SmtpClientBuilder::scheduler(galay::kernel::IOScheduler* scheduler)
    {
        m_scheduler = scheduler;
        return *this;
    }

    SmtpClientBuilder& SmtpClientBuilder::config(AsyncMailConfig config)
    {
        m_config = config;
        return *this;
    }

    SmtpClientBuilder& SmtpClientBuilder::tlsConfig(MailTlsConfig config)
    {
        m_tls_config = std::move(config);
        return *this;
    }

    SmtpClientBuilder& SmtpClientBuilder::heloDomain(std::string domain)
    {
        m_helo_domain = std::move(domain);
        return *this;
    }

    SmtpClient SmtpClientBuilder::build() const
    {
        return SmtpClient(m_scheduler, m_config, m_tls_config, m_helo_domain);
    }

    SmtpClient::SmtpClient(galay::kernel::IOScheduler* scheduler,
                           AsyncMailConfig config,
                           MailTlsConfig tls_config,
                           std::string helo_domain)
        : m_scheduler(scheduler)
        , m_config(config)
        , m_tls_config(std::move(tls_config))
        , m_helo_domain(std::move(helo_domain))
        , m_transport(config)
    {
    }

    galay::kernel::Task<MailVoidResult> SmtpClient::connect(const MailEndpoint& endpoint)
    {
        if (endpoint.protocol != MailProtocol::Smtp) {
            co_return std::unexpected(MailError(MailErrorType::InvalidScheme, "expected smtp url"));
        }
        m_endpoint = endpoint;
        if (m_endpoint.tls.ca_path.empty()) {
            m_endpoint.tls.ca_path = m_tls_config.ca_path;
        }
        if (m_endpoint.tls.server_name.empty()) {
            m_endpoint.tls.server_name = m_tls_config.server_name.empty()
                                             ? endpoint.host
                                             : m_tls_config.server_name;
        }
        m_endpoint.tls.verify_peer = m_endpoint.tls.verify_peer || m_tls_config.verify_peer;
        if (m_tls_config.verify_depth != 4) {
            m_endpoint.tls.verify_depth = m_tls_config.verify_depth;
        }

        auto connected = co_await m_transport.connect(m_endpoint);
        if (!connected) {
            co_return std::unexpected(connected.error());
        }

        auto greeting = co_await readReply();
        if (!greeting) {
            co_return std::unexpected(greeting.error());
        }
        if (greeting->code >= 400) {
            co_return std::unexpected(MailError(MailErrorType::ServerError, greeting->message));
        }

        if (m_endpoint.security == MailSecurity::StartTls) {
            auto pre_tls_ehlo = co_await ehlo();
            if (!pre_tls_ehlo) {
                co_return std::unexpected(pre_tls_ehlo.error());
            }
            if (pre_tls_ehlo->negative()) {
                co_return std::unexpected(MailError(MailErrorType::ServerError, pre_tls_ehlo->message));
            }

            auto upgraded = co_await startTls();
            if (!upgraded) {
                co_return std::unexpected(upgraded.error());
            }
            auto post_tls_ehlo = co_await ehlo();
            if (!post_tls_ehlo) {
                co_return std::unexpected(post_tls_ehlo.error());
            }
            if (post_tls_ehlo->negative()) {
                co_return std::unexpected(MailError(MailErrorType::ServerError, post_tls_ehlo->message));
            }
        } else if (!m_endpoint.username.empty()) {
            auto hello = co_await ehlo();
            if (!hello) {
                co_return std::unexpected(hello.error());
            }
            if (hello->negative()) {
                co_return std::unexpected(MailError(MailErrorType::ServerError, hello->message));
            }
        }

        if (!m_endpoint.username.empty()) {
            auto auth = co_await authPlain(m_endpoint.username, m_endpoint.password);
            if (!auth) {
                co_return std::unexpected(auth.error());
            }
        }
        co_return MailVoidResult{};
    }

    galay::kernel::Task<MailVoidResult> SmtpClient::connect(std::string url)
    {
        auto endpoint = parseMailUrl(url);
        if (!endpoint) {
            co_return std::unexpected(endpoint.error());
        }
        co_return co_await connect(*endpoint);
    }

    galay::kernel::Task<SmtpReplyResult> SmtpClient::readReply()
    {
        std::string buffer;
        while (true) {
            auto parsed = m_parser.parse(buffer);
            if (parsed) {
                co_return std::move(parsed->second);
            }
            if (parsed.error() != protocol::SmtpParseError::Incomplete) {
                co_return std::unexpected(MailError(MailErrorType::ParseError));
            }
            auto chunk = co_await m_transport.recvChunk();
            if (!chunk) {
                co_return std::unexpected(chunk.error());
            }
            buffer += *chunk;
        }
    }

    galay::kernel::Task<SmtpReplyResult> SmtpClient::command(std::string encoded)
    {
        auto sent = co_await m_transport.sendAll(std::move(encoded));
        if (!sent) {
            co_return std::unexpected(sent.error());
        }
        co_return co_await readReply();
    }

    galay::kernel::Task<SmtpReplyResult> SmtpClient::ehlo(std::string domain)
    {
        if (domain.empty()) {
            domain = m_helo_domain;
        }
        co_return co_await command(m_encoder.ehlo(domain));
    }

    galay::kernel::Task<SmtpReplyResult> SmtpClient::helo(std::string domain)
    {
        if (domain.empty()) {
            domain = m_helo_domain;
        }
        co_return co_await command(m_encoder.helo(domain));
    }

    galay::kernel::Task<SmtpReplyResult> SmtpClient::startTls()
    {
        auto reply = co_await command(m_encoder.startTls());
        if (!reply) {
            co_return std::unexpected(reply.error());
        }
        if (reply->code != 220) {
            co_return std::unexpected(MailError(MailErrorType::ServerError, reply->message));
        }
        auto upgraded = co_await m_transport.startTls(m_endpoint.tls, m_endpoint.tls.server_name);
        if (!upgraded) {
            co_return std::unexpected(upgraded.error());
        }
        co_return *reply;
    }

    galay::kernel::Task<SmtpReplyResult>
    SmtpClient::authPlain(std::string username, std::string password)
    {
        co_return co_await command(m_encoder.authPlain(
            protocol::authPlainPayload(username, password)));
    }

    galay::kernel::Task<SmtpReplyResult>
    SmtpClient::authLogin(std::string username, std::string password)
    {
        auto reply = co_await command(m_encoder.authLogin());
        if (!reply) {
            co_return std::unexpected(reply.error());
        }
        if (reply->code != 334) {
            co_return *reply;
        }

        reply = co_await command(m_encoder.authLoginResponse(protocol::authLoginUsername(username)));
        if (!reply || reply->code != 334) {
            co_return reply;
        }
        co_return co_await command(m_encoder.authLoginResponse(protocol::authLoginPassword(password)));
    }

    galay::kernel::Task<SmtpReplyResult> SmtpClient::mailFrom(std::string address)
    {
        co_return co_await command(m_encoder.mailFrom(address));
    }

    galay::kernel::Task<SmtpReplyResult> SmtpClient::rcptTo(std::string address)
    {
        co_return co_await command(m_encoder.rcptTo(address));
    }

    galay::kernel::Task<SmtpReplyResult> SmtpClient::data(std::string message)
    {
        auto reply = co_await command(m_encoder.dataCommand());
        if (!reply) {
            co_return std::unexpected(reply.error());
        }
        if (reply->code != 354) {
            co_return *reply;
        }
        co_return co_await command(m_encoder.dataBody(message));
    }

    galay::kernel::Task<SmtpReplyResult>
    SmtpClient::sendMail(std::string from, std::vector<std::string> recipients, std::string message)
    {
        auto reply = co_await mailFrom(std::move(from));
        if (!reply || reply->negative()) {
            co_return reply;
        }
        for (auto& recipient : recipients) {
            reply = co_await rcptTo(std::move(recipient));
            if (!reply || reply->negative()) {
                co_return reply;
            }
        }
        co_return co_await data(std::move(message));
    }

    galay::kernel::Task<SmtpReplyResult> SmtpClient::quit()
    {
        co_return co_await command(m_encoder.quit());
    }

    galay::kernel::Task<MailVoidResult> SmtpClient::close()
    {
        co_return co_await m_transport.close();
    }
} // namespace galay::mail::async
