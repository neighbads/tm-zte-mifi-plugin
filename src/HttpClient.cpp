#include "HttpClient.h"
#pragma comment(lib, "winhttp.lib")

HttpClient::HttpClient()
    : m_hSession(NULL)
{
    m_hSession = WinHttpOpen(L"ZteMifiPlugin/1.0",
                             WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                             WINHTTP_NO_PROXY_NAME,
                             WINHTTP_NO_PROXY_BYPASS,
                             0);
}

HttpClient::~HttpClient()
{
    if (m_hSession)
        WinHttpCloseHandle(m_hSession);
}

std::string HttpClient::Get(const std::wstring& host, int port, const std::wstring& path,
                            int timeoutMs, int& errorCode)
{
    errorCode = 1;
    std::string result;

    if (!m_hSession)
        return result;

    WinHttpSetTimeouts(m_hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    HINTERNET hConnect = WinHttpConnect(m_hSession, host.c_str(), (INTERNET_PORT)port, 0);
    if (!hConnect)
        return result;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            0);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        return result;
    }

    BOOL bResult = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                      WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!bResult)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return result;
    }

    bResult = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResult)
    {
        errorCode = 3;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return result;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);

    if (statusCode != 200)
    {
        errorCode = 2;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return result;
    }

    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    do
    {
        dwSize = 0;
        WinHttpQueryDataAvailable(hRequest, &dwSize);
        if (dwSize > 0)
        {
            char* buffer = new char[dwSize + 1];
            ZeroMemory(buffer, dwSize + 1);
            WinHttpReadData(hRequest, buffer, dwSize, &dwDownloaded);
            result.append(buffer, dwDownloaded);
            delete[] buffer;
        }
    } while (dwSize > 0);

    errorCode = 0;
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    return result;
}
