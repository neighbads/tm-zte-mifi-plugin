#pragma once
#include <string>
#include <windows.h>
#include <winhttp.h>

class HttpClient
{
public:
    HttpClient();
    ~HttpClient();

    // Synchronous GET request, returns response body. Empty string on failure.
    // errorCode: 0=success, 1=cannot connect, 2=http error, 3=timeout
    std::string Get(const std::wstring& host, int port, const std::wstring& path, int timeoutMs, int& errorCode);

private:
    HINTERNET m_hSession;
};
