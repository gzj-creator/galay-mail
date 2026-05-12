#include "examples/common/config.h"
#include "galay-mail/async/smtp_client.h"
#include <galay-kernel/kernel/runtime.h>
#include <iostream>

using namespace galay::mail::async;

galay::kernel::Task<void> run()
{
    const auto url = envOrEmpty("GALAY_MAIL_SMTP_URL");
    const auto from = envOrEmpty("GALAY_MAIL_SMTP_FROM");
    const auto to = envOrEmpty("GALAY_MAIL_SMTP_TO");
    if (url.empty() || from.empty() || to.empty()) {
        std::cerr << "set GALAY_MAIL_SMTP_URL, GALAY_MAIL_SMTP_FROM and GALAY_MAIL_SMTP_TO\n";
        co_return;
    }

    auto client = SmtpClientBuilder().build();
    auto connected = co_await client.connect(url);
    if (!connected) {
        std::cerr << connected.error().message() << '\n';
        co_return;
    }

    auto sent = co_await client.sendMail(from, {to}, "Subject: galay-mail demo\r\n\r\nhello\r\n");
    if (!sent) {
        std::cerr << sent.error().message() << '\n';
    }
    (void)co_await client.quit();
    (void)co_await client.close();
}

int main()
{
    galay::kernel::Runtime runtime;
    runtime.blockOn(run());
    return 0;
}
