# galay-mail Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build the first `galay-mail` release with SMTP/POP3 protocol parsing, URL-based connection setup, and async mail clients supporting plain TCP, implicit TLS, and STARTTLS/STLS.

**Architecture:** Follow the `galay-redis` repository shape while keeping the first release focused. Implement deterministic protocol and URL components first, then wire async clients through a shared transport abstraction over `galay-kernel` TCP and `galay-ssl` TLS sockets.

**Tech Stack:** C++23, CMake 3.20+, `galay-kernel`, `galay-ssl`, OpenSSL, `std::expected`, `galay::kernel::Task`

---

### Task 1: Scaffold Package Layout

**Files:**
- Create: `CMakeLists.txt`
- Create: `cmake/option.cmake`
- Create: `galay-mail/CMakeLists.txt`
- Create: `galay-mail-config.cmake.in`
- Create: `README.md`
- Create: `docs/README.md`
- Create: `examples/CMakeLists.txt`
- Create: `test/CMakeLists.txt`

**Steps:**
1. Write the package CMake files using `galay-redis` as the structure reference.
2. Configure package dependencies: `galay-kernel 4.0.0`, `galay-ssl 2.0.0`, OpenSSL, and spdlog if needed.
3. Install public headers from `base`, `protocol`, `async`, and `module`.
4. Add empty test/example directories to keep CMake options stable.
5. Run `cmake -S . -B build -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON`.

Expected: CMake configures once source files exist; before source files it may fail because target sources are not yet present.

### Task 2: Add Base Types and URL Parser

**Files:**
- Create: `galay-mail/base/mail_error.h`
- Create: `galay-mail/base/mail_error.cc`
- Create: `galay-mail/base/mail_config.h`
- Create: `galay-mail/base/mail_url.h`
- Create: `galay-mail/base/mail_url.cc`
- Create: `test/t1_url.cc`

**Steps:**
1. Write failing URL tests for `smtp`, `smtp+starttls`, `smtps`, `pop3`, `pop3+stls`, and `pop3s`.
2. Add tests for default ports, explicit ports, percent-decoded credentials, IPv6 literals, and query TLS options.
3. Implement `MailError`, `MailTlsConfig`, `MailProtocol`, `MailSecurity`, `MailEndpoint`, and `parseMailUrl`.
4. Run `cmake --build build --target t1_url` and `ctest --test-dir build -R t1_url --output-on-failure`.

Expected: URL tests pass without network access.

### Task 3: Implement Auth Helpers

**Files:**
- Create: `galay-mail/protocol/auth.h`
- Create: `galay-mail/protocol/auth.cc`
- Create: `test/t2_auth.cc`

**Steps:**
1. Write failing tests for Base64 known vectors.
2. Add tests for AUTH PLAIN payload: `base64("\\0user\\0pass")`.
3. Add tests for AUTH LOGIN username/password payloads.
4. Implement Base64 and AUTH helper functions.
5. Run `ctest --test-dir build -R t2_auth --output-on-failure`.

Expected: Auth helper tests pass.

### Task 4: Implement SMTP Protocol

**Files:**
- Create: `galay-mail/protocol/smtp_protocol.h`
- Create: `galay-mail/protocol/smtp_protocol.cc`
- Create: `test/t3_smtp_protocol.cc`

**Steps:**
1. Write parser tests for single-line reply: `220 mail.example.com ready\r\n`.
2. Write parser tests for multiline reply: `250-mail\r\n250-STARTTLS\r\n250 AUTH PLAIN LOGIN\r\n`.
3. Write incomplete and invalid format tests.
4. Write encoder tests for `EHLO`, `STARTTLS`, `AUTH`, `MAIL FROM`, `RCPT TO`, `DATA`, and `QUIT`.
5. Write DATA dot-stuffing tests.
6. Implement `SmtpReply`, `SmtpParser`, and `SmtpEncoder`.
7. Run `ctest --test-dir build -R t3_smtp_protocol --output-on-failure`.

Expected: SMTP protocol tests pass without network access.

### Task 5: Implement POP3 Protocol

**Files:**
- Create: `galay-mail/protocol/pop3_protocol.h`
- Create: `galay-mail/protocol/pop3_protocol.cc`
- Create: `test/t4_pop3_protocol.cc`

**Steps:**
1. Write parser tests for `+OK` and `-ERR` single-line replies.
2. Write multiline tests for dot-terminated `CAPA`, `LIST`, and `RETR` style replies.
3. Add dot-unstuffing and incomplete reply tests.
4. Write encoder tests for `CAPA`, `STLS`, `USER`, `PASS`, `AUTH PLAIN`, `STAT`, `LIST`, `RETR`, `DELE`, `NOOP`, `RSET`, and `QUIT`.
5. Implement `Pop3Reply`, `Pop3Parser`, and `Pop3Encoder`.
6. Run `ctest --test-dir build -R t4_pop3_protocol --output-on-failure`.

Expected: POP3 protocol tests pass without network access.

### Task 6: Implement Async Transport and Clients

**Files:**
- Create: `galay-mail/async/mail_transport.h`
- Create: `galay-mail/async/mail_transport.cc`
- Create: `galay-mail/async/smtp_client.h`
- Create: `galay-mail/async/smtp_client.cc`
- Create: `galay-mail/async/pop3_client.h`
- Create: `galay-mail/async/pop3_client.cc`
- Create: `test/t5_surface.cc`

**Steps:**
1. Write compile-time surface tests for builders and public client methods.
2. Implement a shared transport that sends/receives over TCP or TLS.
3. Implement connect-by-host and connect-by-URL for SMTP and POP3.
4. Implement implicit TLS handshake.
5. Implement STARTTLS/STLS upgrade command flow and TLS handshake over the existing handle.
6. Implement SMTP commands: `ehlo`, `helo`, `startTls`, `authPlain`, `authLogin`, `mailFrom`, `rcptTo`, `data`, `sendMail`, `quit`.
7. Implement POP3 commands: `capa`, `stls`, `userPass`, `authPlain`, `stat`, `list`, `retr`, `dele`, `noop`, `rset`, `quit`.
8. Run `ctest --test-dir build -R t5_surface --output-on-failure`.

Expected: Client API compiles and link tests pass. Real network tests remain optional.

### Task 7: Add Module Interface and Examples

**Files:**
- Create: `galay-mail/module/module_prelude.hpp`
- Create: `galay-mail/module/galay_mail.cppm`
- Create: `examples/common/config.h`
- Create: `examples/include/e1_smtp_send.cc`
- Create: `examples/include/e2_pop3_retr.cc`
- Create: `examples/import/e1_smtp_send.cc`
- Create: `examples/import/e2_pop3_retr.cc`

**Steps:**
1. Export base, protocol, and async public headers in the module interface.
2. Add guarded examples that read URLs and credentials from environment variables.
3. Ensure examples do not run during unit tests.
4. Build examples with `cmake --build build --target e1_smtp_send e2_pop3_retr`.

Expected: Examples compile when dependencies are available.

### Task 8: Final Documentation and Verification

**Files:**
- Modify: `README.md`
- Modify: `docs/README.md`
- Create: `CHANGELOG.md`
- Create: `docs/release_note.md`

**Steps:**
1. Document build requirements and CMake consumption.
2. Document URL schemes and defaults.
3. Document minimal SMTP and POP3 usage.
4. Add first changelog entry for the initial implementation.
5. Run:
   - `cmake -S . -B build -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON`
   - `cmake --build build --parallel`
   - `ctest --test-dir build --output-on-failure`
6. Review `git status --short` and commit the implementation.

Expected: All unit tests pass locally; network integration is not required for first release verification.

