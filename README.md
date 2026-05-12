# galay-mail

`galay-mail` provides SMTP and POP3 client utilities for the `galay` ecosystem.

This first release focuses on protocol parsing, command encoding, URL handling,
and async client APIs for plain TCP, implicit TLS, and STARTTLS/STLS.

## Build

```sh
cmake -S . -B build -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

If dependencies are installed in a local prefix, pass `CMAKE_PREFIX_PATH`.

## URL Schemes

- `smtp://user:pass@mail.example.com:25`
- `smtp+starttls://user:pass@mail.example.com:587`
- `smtps://user:pass@mail.example.com:465`
- `pop3://user:pass@mail.example.com:110`
- `pop3+stls://user:pass@mail.example.com:110`
- `pop3s://user:pass@mail.example.com:995`

## CMake Consumption

```cmake
find_package(galay-mail CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE galay-mail::galay-mail)
```

## SMTP

Async network calls must run on a `galay::kernel::IOScheduler`. For small
tools, build the runtime with an IO scheduler and no compute scheduler, or
schedule the task onto `runtime.getNextIOScheduler()`.

```cpp
auto runtime = galay::kernel::RuntimeBuilder()
                   .ioSchedulerCount(1)
                   .computeSchedulerCount(0)
                   .build();
auto client = galay::mail::async::SmtpClientBuilder().build();
auto connected = co_await client.connect("smtp+starttls://user:pass@mail.example.com:587");
auto sent = co_await client.sendMail(
    "sender@example.com",
    {"receiver@example.com"},
    "Subject: hello\r\n\r\nbody\r\n");
```

For 163 implicit TLS SMTP, use a URL like:

```text
smtps://galay_center%40163.com:<auth-code>@smtp.163.com:465?verify_peer=true&server_name=smtp.163.com
```

## POP3

```cpp
auto client = galay::mail::async::Pop3ClientBuilder().build();
auto connected = co_await client.connect("pop3s://user:pass@mail.example.com");
auto stat = co_await client.stat();
```
