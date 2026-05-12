#ifndef GALAY_MAIL_POP3_CLIENT_H
#define GALAY_MAIL_POP3_CLIENT_H

#include "galay-mail/async/mail_transport.h"
#include "galay-mail/base/mail_url.h"
#include "galay-mail/protocol/pop3_protocol.h"
#include <galay-kernel/kernel/awaitable.h>
#include <galay-kernel/kernel/io_scheduler.hpp>
#include <galay-kernel/kernel/task.h>
#include <optional>
#include <string>

namespace galay::mail::async
{
    using Pop3ReplyResult = MailResult<protocol::Pop3Reply>;
    using Pop3ReplyOperation = galay::kernel::ReadyAwaitable<Pop3ReplyResult>;
    using Pop3VoidOperation = galay::kernel::ReadyAwaitable<MailVoidResult>;

    class Pop3Client;

    class Pop3ClientBuilder
    {
    public:
        Pop3ClientBuilder& scheduler(galay::kernel::IOScheduler* scheduler);
        Pop3ClientBuilder& config(AsyncMailConfig config);
        Pop3ClientBuilder& tlsConfig(MailTlsConfig config);
        Pop3Client build() const;

    private:
        galay::kernel::IOScheduler* m_scheduler = nullptr;
        AsyncMailConfig m_config;
        MailTlsConfig m_tls_config;
    };

    class Pop3Client
    {
    public:
        Pop3Client(galay::kernel::IOScheduler* scheduler,
                   AsyncMailConfig config,
                   MailTlsConfig tls_config);

        galay::kernel::Task<MailVoidResult> connect(const MailEndpoint& endpoint);
        galay::kernel::Task<MailVoidResult> connect(std::string url);
        galay::kernel::Task<Pop3ReplyResult> capa();
        galay::kernel::Task<Pop3ReplyResult> stls();
        galay::kernel::Task<Pop3ReplyResult> userPass(std::string username, std::string password);
        galay::kernel::Task<Pop3ReplyResult> authPlain(std::string username, std::string password);
        galay::kernel::Task<Pop3ReplyResult> stat();
        galay::kernel::Task<Pop3ReplyResult> list(std::optional<int> message = std::nullopt);
        galay::kernel::Task<Pop3ReplyResult> retr(int message);
        galay::kernel::Task<Pop3ReplyResult> dele(int message);
        galay::kernel::Task<Pop3ReplyResult> noop();
        galay::kernel::Task<Pop3ReplyResult> rset();
        galay::kernel::Task<Pop3ReplyResult> quit();
        galay::kernel::Task<MailVoidResult> close();

    private:
        galay::kernel::Task<Pop3ReplyResult> command(std::string encoded, bool multiline = false);
        galay::kernel::Task<Pop3ReplyResult> readReply(bool multiline);

        galay::kernel::IOScheduler* m_scheduler = nullptr;
        AsyncMailConfig m_config;
        MailTlsConfig m_tls_config;
        MailEndpoint m_endpoint;
        MailTransport m_transport;
        protocol::Pop3Parser m_parser;
        protocol::Pop3Encoder m_encoder;
    };
} // namespace galay::mail::async

#endif
