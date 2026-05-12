# galay-mail SMTP/POP3 Client Design

## Context

`galay-mail` starts as an empty standalone repository under the `galay-sdk`
workspace. It should follow the `galay-redis` package shape: top-level CMake
entry, `cmake/`, `docs/`, `examples/`, `test/`, and an inner `galay-mail/`
source tree with `base/`, `protocol/`, `async/`, and `module/`.

The first release must provide SMTP and POP3 protocol parsing, command encoding,
and async client libraries. It must support plain TCP, implicit TLS, and
STARTTLS/STLS upgrades through `galay-ssl`.

## Goals

- Provide a standalone CMake package named `galay-mail`.
- Export public headers under `include/galay-mail/...`.
- Provide `SmtpClient` and `Pop3Client` async APIs.
- Parse SMTP multiline replies and POP3 single-line/multiline replies.
- Encode common SMTP and POP3 commands.
- Support URL based connection setup:
  - `smtp://user:pass@mail.example.com:25`
  - `smtp+starttls://user:pass@mail.example.com:587`
  - `smtps://user:pass@mail.example.com:465`
  - `pop3://user:pass@mail.example.com:110`
  - `pop3+stls://user:pass@mail.example.com:110`
  - `pop3s://user:pass@mail.example.com:995`
- Support manual authentication after connection, and automatic credential
  extraction from URL.

## Non-Goals

- Full MIME composition, attachment streaming, or RFC 5322 message building.
- IMAP support.
- SMTP server or POP3 server implementation.
- SASL mechanisms beyond AUTH PLAIN and AUTH LOGIN in the first release.
- Connection pooling.

## Architecture

`galay-mail` is split into three layers:

1. `base/`
   - `MailError`, `MailConfig`, `MailTlsConfig`, `MailEndpoint`,
     `MailUrlParser`, and common result aliases.
   - URL parsing maps schemes to protocol and security mode.
   - TLS config keeps CA path, SNI/server name, peer verification, and verify
     depth.

2. `protocol/`
   - SMTP:
     - `SmtpReply` stores status code, enhanced status text, raw lines, and
       normalized message text.
     - `SmtpParser` supports incremental parsing of multiline replies where
       `250-...` continues and `250 ...` terminates.
     - `SmtpEncoder` encodes `EHLO`, `HELO`, `STARTTLS`, `AUTH`, `MAIL FROM`,
       `RCPT TO`, `DATA`, message data dot-stuffing, `RSET`, `NOOP`, and `QUIT`.
   - POP3:
     - `Pop3Reply` stores status, first-line message, and multiline lines.
     - `Pop3Parser` supports single-line replies and dot-terminated multiline
       bodies with dot unstuffing.
     - `Pop3Encoder` encodes `CAPA`, `STLS`, `USER`, `PASS`, `AUTH PLAIN`,
       `STAT`, `LIST`, `RETR`, `DELE`, `NOOP`, `RSET`, and `QUIT`.
   - `auth.h/.cc` provides Base64 plus AUTH PLAIN and AUTH LOGIN payload helpers.

3. `async/`
   - `SmtpClient` and `Pop3Client` own a transport session.
   - The transport starts as `TcpSocket` for plain and STARTTLS/STLS modes.
   - For implicit TLS, the client connects with `SslSocket` and handshakes
     immediately.
   - For STARTTLS/STLS, the client sends the upgrade command, validates a
     success reply, wraps the existing socket handle with `SslSocket`, performs
     TLS handshake, and continues encrypted.
   - Public methods return `galay::kernel::Task<std::expected<...>>`-style
     awaitables matching the existing ecosystem.

## URL Behavior

The URL parser accepts:

```text
<scheme>://[username[:password]@]host[:port][?key=value&...]
```

Supported schemes:

- SMTP:
  - `smtp`: plain, default port `25`
  - `smtp+starttls`: STARTTLS, default port `587`
  - `smtps`: implicit TLS, default port `465`
- POP3:
  - `pop3`: plain, default port `110`
  - `pop3+stls`: STLS, default port `110`
  - `pop3s`: implicit TLS, default port `995`

The parser percent-decodes username and password, supports bracketed IPv6
hosts, and sets SNI to the URL host unless a builder override is supplied.

Initial query keys:

- `verify_peer=true|false`
- `ca_path=<path>`
- `server_name=<sni>`

Unknown query keys are ignored in the first release.

## Error Handling

All layers return `std::expected<T, MailError>`.

`MailErrorType` includes:

- invalid URL, scheme, host, port, or credential encoding
- connection, send, receive, timeout, and closed connection
- TLS handshake or verification failure
- protocol parse failure or incomplete input
- server negative reply
- unsupported auth mechanism or invalid state

Transport errors from `galay-kernel` and `galay-ssl` are mapped into
`MailError`.

## Testing

Unit tests should not require a real mail server.

- URL parser tests cover all supported schemes, default ports, credentials,
  IPv6 literals, and TLS query options.
- SMTP parser tests cover single-line, multiline, incomplete, invalid, and
  reply classification cases.
- SMTP encoder tests cover command formatting and DATA dot-stuffing.
- POP3 parser tests cover `+OK`, `-ERR`, multiline dot termination, dot
  unstuffing, incomplete replies, and invalid first lines.
- POP3 encoder tests cover command formatting.
- Auth tests cover Base64, AUTH PLAIN, and AUTH LOGIN payload helpers.
- Surface tests validate that `SmtpClient` and `Pop3Client` expose the expected
  builder and method signatures.

Network integration tests can be added later behind environment variables.

## Documentation

Add a concise README with build requirements, CMake consumption, URL examples,
and short SMTP/POP3 snippets. Keep deeper docs minimal for the first release.

