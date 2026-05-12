#include "galay-mail/async/pop3_client.h"
#include "galay-mail/async/smtp_client.h"
#include <concepts>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

using namespace galay::mail;
using namespace galay::mail::async;

static_assert(std::same_as<decltype(SmtpClientBuilder().build()), SmtpClient>);
static_assert(std::same_as<decltype(Pop3ClientBuilder().build()), Pop3Client>);

static_assert(requires(SmtpClient client, MailEndpoint endpoint) {
    { client.connect(endpoint) } -> std::same_as<galay::kernel::Task<MailVoidResult>>;
    { client.connect(std::string{}) } -> std::same_as<galay::kernel::Task<MailVoidResult>>;
    { client.ehlo(std::string{}) } -> std::same_as<galay::kernel::Task<SmtpReplyResult>>;
    { client.helo(std::string{}) } -> std::same_as<galay::kernel::Task<SmtpReplyResult>>;
    { client.startTls() } -> std::same_as<galay::kernel::Task<SmtpReplyResult>>;
    { client.authPlain(std::string{}, std::string{}) } -> std::same_as<galay::kernel::Task<SmtpReplyResult>>;
    { client.authLogin(std::string{}, std::string{}) } -> std::same_as<galay::kernel::Task<SmtpReplyResult>>;
    { client.mailFrom(std::string{}) } -> std::same_as<galay::kernel::Task<SmtpReplyResult>>;
    { client.rcptTo(std::string{}) } -> std::same_as<galay::kernel::Task<SmtpReplyResult>>;
    { client.data(std::string{}) } -> std::same_as<galay::kernel::Task<SmtpReplyResult>>;
    { client.sendMail(std::string{}, std::vector<std::string>{}, std::string{}) } -> std::same_as<galay::kernel::Task<SmtpReplyResult>>;
    { client.quit() } -> std::same_as<galay::kernel::Task<SmtpReplyResult>>;
    { client.close() } -> std::same_as<galay::kernel::Task<MailVoidResult>>;
});

static_assert(requires(Pop3Client client, MailEndpoint endpoint) {
    { client.connect(endpoint) } -> std::same_as<galay::kernel::Task<MailVoidResult>>;
    { client.connect(std::string{}) } -> std::same_as<galay::kernel::Task<MailVoidResult>>;
    { client.capa() } -> std::same_as<galay::kernel::Task<Pop3ReplyResult>>;
    { client.stls() } -> std::same_as<galay::kernel::Task<Pop3ReplyResult>>;
    { client.userPass(std::string{}, std::string{}) } -> std::same_as<galay::kernel::Task<Pop3ReplyResult>>;
    { client.authPlain(std::string{}, std::string{}) } -> std::same_as<galay::kernel::Task<Pop3ReplyResult>>;
    { client.stat() } -> std::same_as<galay::kernel::Task<Pop3ReplyResult>>;
    { client.list(std::optional<int>{}) } -> std::same_as<galay::kernel::Task<Pop3ReplyResult>>;
    { client.retr(1) } -> std::same_as<galay::kernel::Task<Pop3ReplyResult>>;
    { client.dele(1) } -> std::same_as<galay::kernel::Task<Pop3ReplyResult>>;
    { client.noop() } -> std::same_as<galay::kernel::Task<Pop3ReplyResult>>;
    { client.rset() } -> std::same_as<galay::kernel::Task<Pop3ReplyResult>>;
    { client.quit() } -> std::same_as<galay::kernel::Task<Pop3ReplyResult>>;
    { client.close() } -> std::same_as<galay::kernel::Task<MailVoidResult>>;
});

int main()
{
    return 0;
}
