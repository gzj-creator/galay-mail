#include "examples/common/config.h"
#include "galay-mail/async/pop3_client.h"
#include <galay-kernel/kernel/runtime.h>
#include <iostream>

using namespace galay::mail::async;

galay::kernel::Task<void> run()
{
    const auto url = envOrEmpty("GALAY_MAIL_POP3_URL");
    if (url.empty()) {
        std::cerr << "set GALAY_MAIL_POP3_URL\n";
        co_return;
    }

    auto client = Pop3ClientBuilder().build();
    auto connected = co_await client.connect(url);
    if (!connected) {
        std::cerr << connected.error().message() << '\n';
        co_return;
    }

    auto stat = co_await client.stat();
    if (stat) {
        std::cout << stat->message << '\n';
    }
    (void)co_await client.quit();
    (void)co_await client.close();
}

int main()
{
    auto runtime = galay::kernel::RuntimeBuilder()
                       .ioSchedulerCount(1)
                       .computeSchedulerCount(0)
                       .build();
    runtime.blockOn(run());
    return 0;
}
