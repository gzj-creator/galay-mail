#include "pop3_client.h"

#include "galay-mail/protocol/auth.h"

#include <utility>

namespace galay::mail::async
{
    Pop3ClientBuilder& Pop3ClientBuilder::scheduler(galay::kernel::IOScheduler* scheduler)
    {
        m_scheduler = scheduler;
        return *this;
    }

    Pop3ClientBuilder& Pop3ClientBuilder::config(AsyncMailConfig config)
    {
        m_config = config;
        return *this;
    }

    Pop3ClientBuilder& Pop3ClientBuilder::tlsConfig(MailTlsConfig config)
    {
        m_tls_config = std::move(config);
        return *this;
    }

    Pop3Client Pop3ClientBuilder::build() const
    {
        return Pop3Client(m_scheduler, m_config, m_tls_config);
    }

    Pop3Client::Pop3Client(galay::kernel::IOScheduler* scheduler,
                           AsyncMailConfig config,
                           MailTlsConfig tls_config)
        : m_scheduler(scheduler)
        , m_config(config)
        , m_tls_config(std::move(tls_config))
        , m_transport(config)
    {
    }

    galay::kernel::Task<MailVoidResult> Pop3Client::connect(const MailEndpoint& endpoint)
    {
        if (endpoint.protocol != MailProtocol::Pop3) {
            co_return std::unexpected(MailError(MailErrorType::InvalidScheme, "expected pop3 url"));
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

        auto greeting = co_await readReply(false);
        if (!greeting) {
            co_return std::unexpected(greeting.error());
        }
        if (!greeting->ok()) {
            co_return std::unexpected(MailError(MailErrorType::ServerError, greeting->message));
        }

        if (m_endpoint.security == MailSecurity::StartTls) {
            auto upgraded = co_await stls();
            if (!upgraded || !upgraded->ok()) {
                co_return upgraded ? std::unexpected(MailError(MailErrorType::ServerError, upgraded->message))
                                   : std::unexpected(upgraded.error());
            }
        }

        if (!m_endpoint.username.empty()) {
            auto auth = co_await userPass(m_endpoint.username, m_endpoint.password);
            if (!auth || !auth->ok()) {
                co_return auth ? std::unexpected(MailError(MailErrorType::ServerError, auth->message))
                               : std::unexpected(auth.error());
            }
        }
        co_return MailVoidResult{};
    }

    galay::kernel::Task<MailVoidResult> Pop3Client::connect(std::string url)
    {
        auto endpoint = parseMailUrl(url);
        if (!endpoint) {
            co_return std::unexpected(endpoint.error());
        }
        co_return co_await connect(*endpoint);
    }

    galay::kernel::Task<Pop3ReplyResult> Pop3Client::readReply(bool multiline)
    {
        std::string buffer;
        while (true) {
            auto parsed = m_parser.parse(buffer, multiline);
            if (parsed) {
                co_return std::move(parsed->second);
            }
            if (parsed.error() != protocol::Pop3ParseError::Incomplete) {
                co_return std::unexpected(MailError(MailErrorType::ParseError));
            }
            auto chunk = co_await m_transport.recvChunk();
            if (!chunk) {
                co_return std::unexpected(chunk.error());
            }
            buffer += *chunk;
        }
    }

    galay::kernel::Task<Pop3ReplyResult> Pop3Client::command(std::string encoded, bool multiline)
    {
        auto sent = co_await m_transport.sendAll(std::move(encoded));
        if (!sent) {
            co_return std::unexpected(sent.error());
        }
        co_return co_await readReply(multiline);
    }

    galay::kernel::Task<Pop3ReplyResult> Pop3Client::capa()
    {
        co_return co_await command(m_encoder.capa(), true);
    }

    galay::kernel::Task<Pop3ReplyResult> Pop3Client::stls()
    {
        auto reply = co_await command(m_encoder.stls());
        if (!reply || !reply->ok()) {
            co_return reply;
        }
        auto upgraded = co_await m_transport.startTls(m_endpoint.tls, m_endpoint.tls.server_name);
        if (!upgraded) {
            co_return std::unexpected(upgraded.error());
        }
        co_return *reply;
    }

    galay::kernel::Task<Pop3ReplyResult>
    Pop3Client::userPass(std::string username, std::string password)
    {
        auto reply = co_await command(m_encoder.user(username));
        if (!reply || !reply->ok()) {
            co_return reply;
        }
        co_return co_await command(m_encoder.pass(password));
    }

    galay::kernel::Task<Pop3ReplyResult>
    Pop3Client::authPlain(std::string username, std::string password)
    {
        co_return co_await command(m_encoder.authPlain(
            protocol::authPlainPayload(username, password)));
    }

    galay::kernel::Task<Pop3ReplyResult> Pop3Client::stat()
    {
        co_return co_await command(m_encoder.stat());
    }

    galay::kernel::Task<Pop3ReplyResult> Pop3Client::list(std::optional<int> message)
    {
        co_return co_await command(m_encoder.list(message), !message.has_value());
    }

    galay::kernel::Task<Pop3ReplyResult> Pop3Client::retr(int message)
    {
        co_return co_await command(m_encoder.retr(message), true);
    }

    galay::kernel::Task<Pop3ReplyResult> Pop3Client::dele(int message)
    {
        co_return co_await command(m_encoder.dele(message));
    }

    galay::kernel::Task<Pop3ReplyResult> Pop3Client::noop()
    {
        co_return co_await command(m_encoder.noop());
    }

    galay::kernel::Task<Pop3ReplyResult> Pop3Client::rset()
    {
        co_return co_await command(m_encoder.rset());
    }

    galay::kernel::Task<Pop3ReplyResult> Pop3Client::quit()
    {
        co_return co_await command(m_encoder.quit());
    }

    galay::kernel::Task<MailVoidResult> Pop3Client::close()
    {
        co_return co_await m_transport.close();
    }
} // namespace galay::mail::async
