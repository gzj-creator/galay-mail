#ifndef GALAY_MAIL_SMTP_CLIENT_H
#define GALAY_MAIL_SMTP_CLIENT_H

#include "galay-mail/async/mail_transport.h"
#include "galay-mail/base/mail_url.h"
#include "galay-mail/protocol/smtp_protocol.h"
#include <galay-kernel/kernel/awaitable.h>
#include <galay-kernel/kernel/io_scheduler.hpp>
#include <galay-kernel/kernel/task.h>
#include <string>
#include <vector>

namespace galay::mail::async
{
    using SmtpReplyResult = MailResult<protocol::SmtpReply>;
    using SmtpReplyOperation = galay::kernel::ReadyAwaitable<SmtpReplyResult>;
    using SmtpVoidOperation = galay::kernel::ReadyAwaitable<MailVoidResult>;

    class SmtpClient;

    class SmtpClientBuilder
    {
    public:
        SmtpClientBuilder& scheduler(galay::kernel::IOScheduler* scheduler);
        SmtpClientBuilder& config(AsyncMailConfig config);
        SmtpClientBuilder& tlsConfig(MailTlsConfig config);
        SmtpClientBuilder& heloDomain(std::string domain);
        SmtpClient build() const;

        [[nodiscard]] AsyncMailConfig buildConfig() const { return m_config; }
        [[nodiscard]] MailTlsConfig buildTlsConfig() const { return m_tls_config; }

    private:
        galay::kernel::IOScheduler* m_scheduler = nullptr;
        AsyncMailConfig m_config;
        MailTlsConfig m_tls_config;
        std::string m_helo_domain = "localhost";
    };

    class SmtpClient
    {
    public:
        SmtpClient(galay::kernel::IOScheduler* scheduler,
                   AsyncMailConfig config,
                   MailTlsConfig tls_config,
                   std::string helo_domain);

        galay::kernel::Task<MailVoidResult> connect(const MailEndpoint& endpoint);
        galay::kernel::Task<MailVoidResult> connect(std::string url);
        galay::kernel::Task<SmtpReplyResult> ehlo(std::string domain = {});
        galay::kernel::Task<SmtpReplyResult> helo(std::string domain = {});
        galay::kernel::Task<SmtpReplyResult> startTls();
        galay::kernel::Task<SmtpReplyResult> authPlain(std::string username, std::string password);
        galay::kernel::Task<SmtpReplyResult> authLogin(std::string username, std::string password);
        galay::kernel::Task<SmtpReplyResult> mailFrom(std::string address);
        galay::kernel::Task<SmtpReplyResult> rcptTo(std::string address);
        galay::kernel::Task<SmtpReplyResult> data(std::string message);
        galay::kernel::Task<SmtpReplyResult> sendMail(std::string from,
                                                      std::vector<std::string> recipients,
                                                      std::string message);
        galay::kernel::Task<SmtpReplyResult> quit();
        galay::kernel::Task<MailVoidResult> close();

        [[nodiscard]] bool connected() const noexcept { return m_transport.connected(); }

    private:
        galay::kernel::Task<SmtpReplyResult> command(std::string encoded);
        galay::kernel::Task<SmtpReplyResult> readReply();

        galay::kernel::IOScheduler* m_scheduler = nullptr;
        AsyncMailConfig m_config;
        MailTlsConfig m_tls_config;
        std::string m_helo_domain;
        MailEndpoint m_endpoint;
        MailTransport m_transport;
        protocol::SmtpParser m_parser;
        protocol::SmtpEncoder m_encoder;
    };
} // namespace galay::mail::async

#endif
