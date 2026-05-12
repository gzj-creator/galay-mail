#include "galay-mail/base/mail_url.h"
#include <cstdlib>
#include <iostream>
#include <string>

using namespace galay::mail;

static void require(bool value, const char* message)
{
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

int main()
{
    {
        auto result = parseMailUrl("smtp://user:p%40ss@mail.example.com");
        require(result.has_value(), "smtp url parses");
        require(result->protocol == MailProtocol::Smtp, "smtp protocol");
        require(result->security == MailSecurity::Plain, "smtp plain");
        require(result->port == 25, "smtp default port");
        require(result->username == "user", "smtp username");
        require(result->password == "p@ss", "smtp password percent decode");
        require(result->host == "mail.example.com", "smtp host");
    }
    {
        auto result = parseMailUrl("smtp+starttls://mail.example.com");
        require(result.has_value(), "smtp starttls url parses");
        require(result->security == MailSecurity::StartTls, "smtp starttls");
        require(result->port == 587, "smtp starttls default port");
    }
    {
        auto result = parseMailUrl("smtps://mail.example.com:2465?verify_peer=true&ca_path=%2Ftmp%2Fca.pem&server_name=sni.example.com");
        require(result.has_value(), "smtps url parses");
        require(result->security == MailSecurity::Tls, "smtps tls");
        require(result->port == 2465, "smtps explicit port");
        require(result->tls.verify_peer, "query verify peer");
        require(result->tls.ca_path == "/tmp/ca.pem", "query ca path");
        require(result->tls.server_name == "sni.example.com", "query server name");
    }
    {
        auto result = parseMailUrl("pop3://u:p@[2001:db8::1]");
        require(result.has_value(), "pop3 ipv6 url parses");
        require(result->protocol == MailProtocol::Pop3, "pop3 protocol");
        require(result->host == "2001:db8::1", "ipv6 host strips brackets");
        require(result->port == 110, "pop3 default port");
    }
    {
        auto result = parseMailUrl("pop3+stls://mail.example.com");
        require(result.has_value(), "pop3 stls url parses");
        require(result->security == MailSecurity::StartTls, "pop3 stls");
    }
    {
        auto result = parseMailUrl("pop3s://mail.example.com");
        require(result.has_value(), "pop3s url parses");
        require(result->security == MailSecurity::Tls, "pop3s tls");
        require(result->port == 995, "pop3s default port");
    }
    {
        auto result = parseMailUrl("imap://mail.example.com");
        require(!result.has_value(), "unsupported scheme rejected");
    }
    std::cout << "url tests passed\n";
    return 0;
}
