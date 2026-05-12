#include "examples/common/config.h"
#include "galay-mail/async/smtp_client.h"
#include <galay-kernel/kernel/runtime.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace galay::mail::async;

namespace
{
std::string demoMessage()
{
    const auto now = std::chrono::system_clock::now();
    const auto timestamp = std::chrono::system_clock::to_time_t(now);
    std::tm local_time{};
#if defined(_WIN32)
    localtime_s(&local_time, &timestamp);
#else
    localtime_r(&timestamp, &local_time);
#endif

    std::ostringstream message;
    message << "Subject: galay-mail live smtp test\r\n"
            << "\r\n"
            << "This message was sent by galay-mail live SMTP test at "
            << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S %z")
            << ".\r\n";
    return message.str();
}
} // namespace

galay::kernel::Task<int> run()
{
    const auto url = envOrEmpty("GALAY_MAIL_SMTP_URL");
    const auto from = envOrEmpty("GALAY_MAIL_SMTP_FROM");
    const auto to = envOrEmpty("GALAY_MAIL_SMTP_TO");
    if (url.empty() || from.empty() || to.empty()) {
        std::cerr << "set GALAY_MAIL_SMTP_URL, GALAY_MAIL_SMTP_FROM and GALAY_MAIL_SMTP_TO\n";
        co_return 2;
    }

    auto client = SmtpClientBuilder().build();
    auto connected = co_await client.connect(url);
    if (!connected) {
        std::cerr << connected.error().message() << '\n';
        co_return 1;
    }

    auto sent = co_await client.sendMail(from, {to}, demoMessage());
    if (!sent) {
        std::cerr << sent.error().message() << '\n';
        (void)co_await client.close();
        co_return 1;
    }
    if (sent->negative()) {
        std::cerr << sent->code << ' ' << sent->message << '\n';
        (void)co_await client.quit();
        (void)co_await client.close();
        co_return 1;
    }
    (void)co_await client.quit();
    (void)co_await client.close();
    std::cout << sent->code << ' ' << sent->message << '\n';
    co_return 0;
}

int main()
{
    auto runtime = galay::kernel::RuntimeBuilder()
                       .ioSchedulerCount(1)
                       .computeSchedulerCount(0)
                       .build();
    return runtime.blockOn(run());
}
