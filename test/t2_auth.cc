#include "galay-mail/protocol/auth.h"
#include <cstdlib>
#include <iostream>
#include <string>

using namespace galay::mail::protocol;

static void require(bool value, const char* message)
{
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

int main()
{
    require(base64Encode("") == "", "base64 empty");
    require(base64Encode("f") == "Zg==", "base64 f");
    require(base64Encode("fo") == "Zm8=", "base64 fo");
    require(base64Encode("foo") == "Zm9v", "base64 foo");
    require(base64Encode("foobar") == "Zm9vYmFy", "base64 foobar");
    require(authPlainPayload("user", "pass") == "AHVzZXIAcGFzcw==", "auth plain");
    require(authLoginUsername("user") == "dXNlcg==", "auth login username");
    require(authLoginPassword("pass") == "cGFzcw==", "auth login password");
    std::cout << "auth tests passed\n";
    return 0;
}
