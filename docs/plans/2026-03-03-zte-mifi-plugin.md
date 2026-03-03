# ZTE 5G MiFi Plugin Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Create a TrafficMonitor plugin DLL that monitors ZTE 5G MiFi device status via HTTP API.

**Architecture:** Pure Win32 C++ DLL (no MFC). WinHTTP for async HTTP requests, URL parameter parser for response data, Win32 dialog for options, INI file for configuration. Singleton plugin pattern following TrafficMonitor's interface.

**Tech Stack:** C++17, Win32 API, WinHTTP, cl.exe compiler

**Reference:** PluginDemo uses MFC but we use pure Win32 to keep dependencies minimal and compilation simple with cl.exe.

---

### Task 1: Project Skeleton and PluginInterface.h

**Files:**
- Create: `include/PluginInterface.h`
- Create: `src/dllmain.cpp`

**Step 1: Create include/PluginInterface.h**

Copy the exact PluginInterface.h from TrafficMonitor (API version 7) with all three interfaces: `IPluginItem`, `ITMPlugin`, `ITrafficMonitor`.

**Step 2: Create src/dllmain.cpp**

```cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
```

**Step 3: Create build.bat**

```batch
@echo off
setlocal

set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist %VCVARS% call %VCVARS%

if not exist build mkdir build

rc /fo build\OptionsDialog.res src\OptionsDialog.rc

cl /LD /EHsc /O2 /MD /utf-8 /std:c++17 ^
   /I include ^
   /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" ^
   src\dllmain.cpp ^
   src\ZteMifiPlugin.cpp ^
   src\ZteMifiItem.cpp ^
   src\HttpClient.cpp ^
   src\UrlParser.cpp ^
   src\ConfigManager.cpp ^
   src\OptionsDialog.cpp ^
   build\OptionsDialog.res ^
   /link /DLL /OUT:build\ZteMifiPlugin.dll ^
   winhttp.lib user32.lib gdi32.lib comdlg32.lib

del /Q *.obj 2>nul

echo.
if exist build\ZteMifiPlugin.dll (
    echo Build SUCCESS: build\ZteMifiPlugin.dll
) else (
    echo Build FAILED
)

endlocal
```

**Step 4: Commit**

```
feat: project skeleton with PluginInterface.h and build script
```

---

### Task 2: URL Parameter Parser

**Files:**
- Create: `src/UrlParser.h`
- Create: `src/UrlParser.cpp`

**Step 1: Create src/UrlParser.h**

```cpp
#pragma once
#include <string>
#include <map>

class UrlParser
{
public:
    // Parse "key1=value1&key2=value2" format into key-value map
    static std::map<std::string, std::string> Parse(const std::string& data);

    // Get a value by key, returns defaultVal if not found
    static std::string GetValue(const std::map<std::string, std::string>& params,
                                const std::string& key,
                                const std::string& defaultVal = "");
};
```

**Step 2: Create src/UrlParser.cpp**

```cpp
#include "UrlParser.h"

std::map<std::string, std::string> UrlParser::Parse(const std::string& data)
{
    std::map<std::string, std::string> result;
    size_t pos = 0;
    while (pos < data.size())
    {
        size_t ampPos = data.find('&', pos);
        if (ampPos == std::string::npos)
            ampPos = data.size();

        std::string pair = data.substr(pos, ampPos - pos);
        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos)
        {
            std::string key = pair.substr(0, eqPos);
            std::string value = pair.substr(eqPos + 1);
            result[key] = value;
        }
        pos = ampPos + 1;
    }
    return result;
}

std::string UrlParser::GetValue(const std::map<std::string, std::string>& params,
                                const std::string& key,
                                const std::string& defaultVal)
{
    auto it = params.find(key);
    if (it != params.end())
        return it->second;
    return defaultVal;
}
```

**Step 3: Commit**

```
feat: add URL parameter parser for ZTE API response
```

---

### Task 3: HTTP Client with WinHTTP

**Files:**
- Create: `src/HttpClient.h`
- Create: `src/HttpClient.cpp`

**Step 1: Create src/HttpClient.h**

```cpp
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
```

**Step 2: Create src/HttpClient.cpp**

```cpp
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
    errorCode = 1; // default: cannot connect
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
                                            0); // HTTP not HTTPS
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
        errorCode = 3; // timeout or receive error
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return result;
    }

    // Check status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);

    if (statusCode != 200)
    {
        errorCode = 2; // HTTP error (may need login)
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return result;
    }

    // Read response body
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

    errorCode = 0; // success
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    return result;
}
```

**Step 3: Commit**

```
feat: add WinHTTP client for ZTE device API requests
```

---

### Task 4: Configuration Manager

**Files:**
- Create: `src/ConfigManager.h`
- Create: `src/ConfigManager.cpp`
- Create: `ZteMifiPlugin.ini`

**Step 1: Create src/ConfigManager.h**

```cpp
#pragma once
#include <string>
#include <windows.h>

struct ItemConfig
{
    bool operator_enabled = true;
    bool signal_enabled = true;
    bool operator_signal_enabled = false;
    bool up_down_speed_enabled = true;
    bool upload_speed_enabled = false;
    bool download_speed_enabled = false;
};

struct PluginConfig
{
    std::wstring device_ip = L"192.168.0.1";
    int update_interval = 1000;
    ItemConfig items;
};

class ConfigManager
{
public:
    static ConfigManager& Instance();

    void Load(const std::wstring& config_dir);
    void Save() const;

    PluginConfig& Config() { return m_config; }
    const PluginConfig& Config() const { return m_config; }

private:
    ConfigManager();
    static ConfigManager m_instance;
    PluginConfig m_config;
    std::wstring m_config_path;
};
```

**Step 2: Create src/ConfigManager.cpp**

```cpp
#include "ConfigManager.h"

ConfigManager ConfigManager::m_instance;

ConfigManager::ConfigManager() {}

ConfigManager& ConfigManager::Instance()
{
    return m_instance;
}

static void WritePrivateProfileIntW(const wchar_t* section, const wchar_t* key, int value, const wchar_t* path)
{
    wchar_t buf[16];
    swprintf_s(buf, L"%d", value);
    WritePrivateProfileStringW(section, key, buf, path);
}

void ConfigManager::Load(const std::wstring& config_dir)
{
    // Build config path: same name as DLL but with .ini extension
    HMODULE hModule = NULL;
    wchar_t modulePath[MAX_PATH] = {};
    // Get module handle of this DLL
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCWSTR)&ConfigManager::Instance, &hModule);
    GetModuleFileNameW(hModule, modulePath, MAX_PATH);

    std::wstring dllPath(modulePath);
    // If config_dir provided, use it; otherwise use DLL's directory
    if (!config_dir.empty())
    {
        size_t lastSlash = dllPath.find_last_of(L"\\/");
        std::wstring fileName = dllPath.substr(lastSlash + 1);
        m_config_path = config_dir + fileName;
    }
    else
    {
        m_config_path = dllPath;
    }
    // Replace .dll with .ini
    size_t dotPos = m_config_path.rfind(L'.');
    if (dotPos != std::wstring::npos)
        m_config_path = m_config_path.substr(0, dotPos);
    m_config_path += L".ini";

    // Read General section
    wchar_t buf[256] = {};
    GetPrivateProfileStringW(L"General", L"DeviceIP", L"192.168.0.1", buf, 256, m_config_path.c_str());
    m_config.device_ip = buf;
    m_config.update_interval = GetPrivateProfileIntW(L"General", L"UpdateInterval", 1000, m_config_path.c_str());

    // Read Items section
    m_config.items.operator_enabled = GetPrivateProfileIntW(L"Items", L"Operator", 1, m_config_path.c_str()) != 0;
    m_config.items.signal_enabled = GetPrivateProfileIntW(L"Items", L"SignalStrength", 1, m_config_path.c_str()) != 0;
    m_config.items.operator_signal_enabled = GetPrivateProfileIntW(L"Items", L"OperatorWithSignal", 0, m_config_path.c_str()) != 0;
    m_config.items.up_down_speed_enabled = GetPrivateProfileIntW(L"Items", L"UpDownSpeed", 1, m_config_path.c_str()) != 0;
    m_config.items.upload_speed_enabled = GetPrivateProfileIntW(L"Items", L"UploadSpeed", 0, m_config_path.c_str()) != 0;
    m_config.items.download_speed_enabled = GetPrivateProfileIntW(L"Items", L"DownloadSpeed", 0, m_config_path.c_str()) != 0;
}

void ConfigManager::Save() const
{
    if (m_config_path.empty())
        return;

    WritePrivateProfileStringW(L"General", L"DeviceIP", m_config.device_ip.c_str(), m_config_path.c_str());
    WritePrivateProfileIntW(L"General", L"UpdateInterval", m_config.update_interval, m_config_path.c_str());

    WritePrivateProfileIntW(L"Items", L"Operator", m_config.items.operator_enabled ? 1 : 0, m_config_path.c_str());
    WritePrivateProfileIntW(L"Items", L"SignalStrength", m_config.items.signal_enabled ? 1 : 0, m_config_path.c_str());
    WritePrivateProfileIntW(L"Items", L"OperatorWithSignal", m_config.items.operator_signal_enabled ? 1 : 0, m_config_path.c_str());
    WritePrivateProfileIntW(L"Items", L"UpDownSpeed", m_config.items.up_down_speed_enabled ? 1 : 0, m_config_path.c_str());
    WritePrivateProfileIntW(L"Items", L"UploadSpeed", m_config.items.upload_speed_enabled ? 1 : 0, m_config_path.c_str());
    WritePrivateProfileIntW(L"Items", L"DownloadSpeed", m_config.items.download_speed_enabled ? 1 : 0, m_config_path.c_str());
}
```

**Step 3: Create ZteMifiPlugin.ini**

```ini
[General]
DeviceIP=192.168.0.1
UpdateInterval=1000

[Items]
Operator=1
SignalStrength=1
OperatorWithSignal=0
UpDownSpeed=1
UploadSpeed=0
DownloadSpeed=0
```

**Step 4: Commit**

```
feat: add configuration manager with INI file support
```

---

### Task 5: Data Item Classes

**Files:**
- Create: `src/ZteMifiItem.h`
- Create: `src/ZteMifiItem.cpp`

Each of the 6 data items is an instance of `ZteMifiItem` configured with different parameters.

**Step 1: Create src/ZteMifiItem.h**

```cpp
#pragma once
#include "PluginInterface.h"
#include <string>

enum class ItemType
{
    Operator,           // 运营商 (中国移动√ / 中国移动×)
    SignalStrength,     // 信号强度 (▂▄▆█ / ××××)
    OperatorSignal,     // 运营商+信号
    UpDownSpeed,        // 上传下载速度 (↑xx.xxKB/s ↓xx.xxKB/s)
    UploadSpeed,        // 上传速度
    DownloadSpeed       // 下载速度
};

class ZteMifiItem : public IPluginItem
{
public:
    ZteMifiItem(ItemType type);

    virtual const wchar_t* GetItemName() const override;
    virtual const wchar_t* GetItemId() const override;
    virtual const wchar_t* GetItemLableText() const override;
    virtual const wchar_t* GetItemValueText() const override;
    virtual const wchar_t* GetItemValueSampleText() const override;

    void SetValueText(const std::wstring& text);
    ItemType GetType() const { return m_type; }

private:
    ItemType m_type;
    std::wstring m_value_text;
};
```

**Step 2: Create src/ZteMifiItem.cpp**

```cpp
#include "ZteMifiItem.h"

ZteMifiItem::ZteMifiItem(ItemType type)
    : m_type(type)
    , m_value_text(L"\x6B63\x5728\x8FDE\x63A5...") // 正在连接...
{
}

const wchar_t* ZteMifiItem::GetItemName() const
{
    switch (m_type)
    {
    case ItemType::Operator:        return L"\x8FD0\x8425\x5546";          // 运营商
    case ItemType::SignalStrength:   return L"\x4FE1\x53F7\x5F3A\x5EA6";  // 信号强度
    case ItemType::OperatorSignal:   return L"\x8FD0\x8425\x5546+\x4FE1\x53F7"; // 运营商+信号
    case ItemType::UpDownSpeed:      return L"\x4E0A\x4E0B\x8F7D\x901F\x5EA6";  // 上下载速度
    case ItemType::UploadSpeed:      return L"\x4E0A\x4F20\x901F\x5EA6";  // 上传速度
    case ItemType::DownloadSpeed:    return L"\x4E0B\x8F7D\x901F\x5EA6";  // 下载速度
    }
    return L"";
}

const wchar_t* ZteMifiItem::GetItemId() const
{
    switch (m_type)
    {
    case ItemType::Operator:        return L"zteMifiOp";
    case ItemType::SignalStrength:   return L"zteMifiSig";
    case ItemType::OperatorSignal:   return L"zteMifiOpSig";
    case ItemType::UpDownSpeed:      return L"zteMifiUDSpd";
    case ItemType::UploadSpeed:      return L"zteMifiUpSpd";
    case ItemType::DownloadSpeed:    return L"zteMifiDnSpd";
    }
    return L"";
}

const wchar_t* ZteMifiItem::GetItemLableText() const
{
    switch (m_type)
    {
    case ItemType::Operator:        return L"\x8FD0\x8425\x5546: ";      // 运营商:
    case ItemType::SignalStrength:   return L"\x4FE1\x53F7: ";            // 信号:
    case ItemType::OperatorSignal:   return L"\x7F51\x7EDC: ";            // 网络:
    case ItemType::UpDownSpeed:      return L"\x901F\x5EA6: ";            // 速度:
    case ItemType::UploadSpeed:      return L"\x4E0A\x4F20: ";            // 上传:
    case ItemType::DownloadSpeed:    return L"\x4E0B\x8F7D: ";            // 下载:
    }
    return L"";
}

const wchar_t* ZteMifiItem::GetItemValueText() const
{
    return m_value_text.c_str();
}

const wchar_t* ZteMifiItem::GetItemValueSampleText() const
{
    switch (m_type)
    {
    case ItemType::Operator:        return L"\x4E2D\x56FD\x79FB\x52A8\x2714"; // 中国移动√
    case ItemType::SignalStrength:   return L"\x2582\x2584\x2586\x2588";       // ▂▄▆█
    case ItemType::OperatorSignal:   return L"\x4E2D\x56FD\x79FB\x52A8 \x2582\x2584\x2586\x2588"; // 中国移动 ▂▄▆█
    case ItemType::UpDownSpeed:      return L"\x2191""999.99KB/s \x2193""999.99KB/s"; // ↑999.99KB/s ↓999.99KB/s
    case ItemType::UploadSpeed:      return L"999.99 KB/s";
    case ItemType::DownloadSpeed:    return L"999.99 KB/s";
    }
    return L"";
}

void ZteMifiItem::SetValueText(const std::wstring& text)
{
    m_value_text = text;
}
```

**Step 3: Commit**

```
feat: add ZteMifiItem class for 6 display item types
```

---

### Task 6: Main Plugin Class

**Files:**
- Create: `src/ZteMifiPlugin.h`
- Create: `src/ZteMifiPlugin.cpp`

**Step 1: Create src/ZteMifiPlugin.h**

```cpp
#pragma once
#include "PluginInterface.h"
#include "ZteMifiItem.h"
#include "HttpClient.h"
#include <vector>
#include <string>
#include <map>

class ZteMifiPlugin : public ITMPlugin
{
private:
    ZteMifiPlugin();

public:
    static ZteMifiPlugin& Instance();

    // ITMPlugin interface
    virtual IPluginItem* GetItem(int index) override;
    virtual void DataRequired() override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual OptionReturn ShowOptionsDialog(void* hParent) override;
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;
    virtual const wchar_t* GetTooltipInfo() override;

    // Rebuild active items list based on config
    void RebuildActiveItems();

private:
    void UpdateItemValues(const std::map<std::string, std::string>& params);
    void SetAllItemsText(const std::wstring& text);
    std::wstring FormatSignal(int signalBar, bool connected);
    std::wstring FormatSpeed(const std::string& rawValue);

    static ZteMifiPlugin m_instance;

    // All 6 items
    ZteMifiItem m_operator_item;
    ZteMifiItem m_signal_item;
    ZteMifiItem m_operator_signal_item;
    ZteMifiItem m_up_down_speed_item;
    ZteMifiItem m_upload_speed_item;
    ZteMifiItem m_download_speed_item;

    // Active (enabled) items for GetItem()
    std::vector<ZteMifiItem*> m_active_items;

    HttpClient m_http_client;
    int m_fail_count;
    std::wstring m_tooltip_info;
    ITrafficMonitor* m_app;
};

// DLL export
#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();
#ifdef __cplusplus
}
#endif
```

**Step 2: Create src/ZteMifiPlugin.cpp**

```cpp
#include "ZteMifiPlugin.h"
#include "UrlParser.h"
#include "ConfigManager.h"
#include "OptionsDialog.h"
#include <cstdio>

ZteMifiPlugin ZteMifiPlugin::m_instance;

ZteMifiPlugin::ZteMifiPlugin()
    : m_operator_item(ItemType::Operator)
    , m_signal_item(ItemType::SignalStrength)
    , m_operator_signal_item(ItemType::OperatorSignal)
    , m_up_down_speed_item(ItemType::UpDownSpeed)
    , m_upload_speed_item(ItemType::UploadSpeed)
    , m_download_speed_item(ItemType::DownloadSpeed)
    , m_fail_count(0)
    , m_app(nullptr)
{
}

ZteMifiPlugin& ZteMifiPlugin::Instance()
{
    return m_instance;
}

void ZteMifiPlugin::RebuildActiveItems()
{
    m_active_items.clear();
    const auto& items = ConfigManager::Instance().Config().items;
    if (items.operator_enabled)         m_active_items.push_back(&m_operator_item);
    if (items.signal_enabled)           m_active_items.push_back(&m_signal_item);
    if (items.operator_signal_enabled)  m_active_items.push_back(&m_operator_signal_item);
    if (items.up_down_speed_enabled)    m_active_items.push_back(&m_up_down_speed_item);
    if (items.upload_speed_enabled)     m_active_items.push_back(&m_upload_speed_item);
    if (items.download_speed_enabled)   m_active_items.push_back(&m_download_speed_item);
}

IPluginItem* ZteMifiPlugin::GetItem(int index)
{
    if (index >= 0 && index < (int)m_active_items.size())
        return m_active_items[index];
    return nullptr;
}

void ZteMifiPlugin::DataRequired()
{
    const auto& config = ConfigManager::Instance().Config();

    // Build request path
    std::wstring path = L"/goform/goform_get_cmd_process?multi_data=1&isTest=false"
        L"&cmd=ppp_status%2Cnetwork_provider_fullname%2Cnetwork_signalbar"
        L"%2Cflux_realtime_tx_thrpt%2Cflux_realtime_rx_thrpt%2Cnetwork_type";

    int errorCode = 0;
    int timeout = 3000;
    std::string response = m_http_client.Get(config.device_ip, 80, path, timeout, errorCode);

    if (errorCode != 0)
    {
        m_fail_count++;
        if (errorCode == 2)
            SetAllItemsText(L"\x9700\x8981\x767B\x5F55"); // 需要登录
        else
            SetAllItemsText(L"\x8BBE\x5907\x79BB\x7EBF"); // 设备离线
        return;
    }

    m_fail_count = 0;

    auto params = UrlParser::Parse(response);
    if (params.empty())
    {
        SetAllItemsText(L"\x6570\x636E\x5F02\x5E38"); // 数据异常
        return;
    }

    UpdateItemValues(params);
}

void ZteMifiPlugin::UpdateItemValues(const std::map<std::string, std::string>& params)
{
    std::string pppStatus = UrlParser::GetValue(params, "ppp_status");
    std::string providerName = UrlParser::GetValue(params, "network_provider_fullname");
    std::string signalBar = UrlParser::GetValue(params, "network_signalbar", "0");
    std::string txThrpt = UrlParser::GetValue(params, "flux_realtime_tx_thrpt", "0");
    std::string rxThrpt = UrlParser::GetValue(params, "flux_realtime_rx_thrpt", "0");

    bool connected = (pppStatus == "ppp_connected");
    int signalVal = 0;
    try { signalVal = std::stoi(signalBar); } catch (...) {}

    // Convert provider name from UTF-8 to wide string
    std::wstring wProvider;
    if (!providerName.empty())
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, providerName.c_str(), -1, NULL, 0);
        if (len > 0)
        {
            wProvider.resize(len - 1);
            MultiByteToWideChar(CP_UTF8, 0, providerName.c_str(), -1, &wProvider[0], len);
        }
    }
    if (wProvider.empty())
        wProvider = L"N/A";

    // Operator: 中国移动√ or 中国移动×
    std::wstring opText = wProvider + (connected ? L"\x2714" : L"\x00D7");
    m_operator_item.SetValueText(opText);

    // Signal: ▂▄▆█ or ××××
    std::wstring sigText = FormatSignal(signalVal, connected);
    m_signal_item.SetValueText(sigText);

    // Operator + Signal
    m_operator_signal_item.SetValueText(wProvider + L" " + sigText);

    // Speed formatting
    std::wstring upSpeed = FormatSpeed(txThrpt);
    std::wstring downSpeed = FormatSpeed(rxThrpt);

    // Up+Down: ↑xx.xxKB/s ↓xx.xxKB/s
    m_up_down_speed_item.SetValueText(L"\x2191" + upSpeed + L"KB/s \x2193" + downSpeed + L"KB/s");

    // Upload: xx.xx KB/s
    m_upload_speed_item.SetValueText(upSpeed + L" KB/s");

    // Download: xx.xx KB/s
    m_download_speed_item.SetValueText(downSpeed + L" KB/s");

    // Tooltip
    m_tooltip_info = wProvider + L" " + sigText + L"\n"
        L"\x4E0A\x4F20: " + upSpeed + L" KB/s\n"
        L"\x4E0B\x8F7D: " + downSpeed + L" KB/s";
}

void ZteMifiPlugin::SetAllItemsText(const std::wstring& text)
{
    m_operator_item.SetValueText(text);
    m_signal_item.SetValueText(text);
    m_operator_signal_item.SetValueText(text);
    m_up_down_speed_item.SetValueText(text);
    m_upload_speed_item.SetValueText(text);
    m_download_speed_item.SetValueText(text);
}

std::wstring ZteMifiPlugin::FormatSignal(int signalBar, bool connected)
{
    if (!connected)
        return L"\x00D7\x00D7\x00D7\x00D7"; // ××××

    // ▂=0x2582 ▄=0x2584 ▆=0x2586 █=0x2588
    const wchar_t bars[] = { L'\x2582', L'\x2584', L'\x2586', L'\x2588' };
    std::wstring result;

    int level = signalBar;
    if (level < 0) level = 0;
    if (level > 5) level = 5;

    // Map 0-5 to number of bars: 0->1, 1->1, 2->2, 3->3, 4->4, 5->4
    int count = level;
    if (count <= 1) count = 1;
    if (count > 4) count = 4;

    for (int i = 0; i < count; i++)
        result += bars[i];

    return result;
}

std::wstring ZteMifiPlugin::FormatSpeed(const std::string& rawValue)
{
    double val = 0;
    try { val = std::stod(rawValue); } catch (...) {}
    val /= 100.0; // API returns KB/s * 100

    wchar_t buf[32];
    swprintf_s(buf, L"%.2f", val);
    return buf;
}

const wchar_t* ZteMifiPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:          return L"ZTE MiFi Monitor";
    case TMI_DESCRIPTION:   return L"ZTE 5G \x968F\x8EAB WiFi \x76D1\x63A7\x63D2\x4EF6"; // ZTE 5G 随身 WiFi 监控插件
    case TMI_AUTHOR:        return L"ZteMifiPlugin";
    case TMI_COPYRIGHT:     return L"Copyright (C) 2026";
    case TMI_VERSION:       return L"1.0";
    case TMI_URL:           return L"";
    default: break;
    }
    return L"";
}

ITMPlugin::OptionReturn ZteMifiPlugin::ShowOptionsDialog(void* hParent)
{
    return ShowPluginOptionsDialog((HWND)hParent);
}

void ZteMifiPlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
    switch (index)
    {
    case EI_CONFIG_DIR:
        ConfigManager::Instance().Load(data ? std::wstring(data) : L"");
        RebuildActiveItems();
        break;
    default:
        break;
    }
}

const wchar_t* ZteMifiPlugin::GetTooltipInfo()
{
    return m_tooltip_info.c_str();
}

// DLL export
ITMPlugin* TMPluginGetInstance()
{
    return &ZteMifiPlugin::Instance();
}
```

**Step 3: Commit**

```
feat: add main plugin class with data fetching and formatting
```

---

### Task 7: Options Dialog (Win32)

**Files:**
- Create: `src/OptionsDialog.h`
- Create: `src/OptionsDialog.cpp`
- Create: `src/resource.h`
- Create: `src/OptionsDialog.rc`

**Step 1: Create src/resource.h**

```cpp
#pragma once

#define IDD_OPTIONS_DIALOG      101

#define IDC_STATIC_IP           1001
#define IDC_EDIT_IP             1002
#define IDC_STATIC_INTERVAL     1003
#define IDC_EDIT_INTERVAL       1004

#define IDC_CHECK_OPERATOR      1010
#define IDC_CHECK_SIGNAL        1011
#define IDC_CHECK_OP_SIGNAL     1012
#define IDC_CHECK_UPDOWN_SPEED  1013
#define IDC_CHECK_UP_SPEED      1014
#define IDC_CHECK_DOWN_SPEED    1015

#define IDOK                    1
#define IDCANCEL                2
```

**Step 2: Create src/OptionsDialog.rc**

```rc
#include "resource.h"
#include <windows.h>

IDD_OPTIONS_DIALOG DIALOGEX 0, 0, 240, 220
STYLE DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "ZTE MiFi 插件设置"
FONT 9, "MS Shell Dlg"
BEGIN
    LTEXT           "设备 IP 地址:", IDC_STATIC_IP, 10, 12, 60, 10
    EDITTEXT        IDC_EDIT_IP, 75, 10, 155, 14, ES_AUTOHSCROLL

    LTEXT           "更新间隔(ms):", IDC_STATIC_INTERVAL, 10, 32, 60, 10
    EDITTEXT        IDC_EDIT_INTERVAL, 75, 30, 155, 14, ES_AUTOHSCROLL | ES_NUMBER

    GROUPBOX        "显示项目", -1, 10, 52, 220, 120

    AUTOCHECKBOX    "运营商 (中国移动√/×)", IDC_CHECK_OPERATOR, 20, 68, 200, 10
    AUTOCHECKBOX    "信号强度 (▂▄▆█)", IDC_CHECK_SIGNAL, 20, 84, 200, 10
    AUTOCHECKBOX    "运营商+信号 (中国移动 ▂▄▆█)", IDC_CHECK_OP_SIGNAL, 20, 100, 200, 10
    AUTOCHECKBOX    "上传下载速度 (↑xx ↓xx)", IDC_CHECK_UPDOWN_SPEED, 20, 116, 200, 10
    AUTOCHECKBOX    "上传速度", IDC_CHECK_UP_SPEED, 20, 132, 200, 10
    AUTOCHECKBOX    "下载速度", IDC_CHECK_DOWN_SPEED, 20, 148, 200, 10

    DEFPUSHBUTTON   "确定", IDOK, 120, 190, 50, 14
    PUSHBUTTON      "取消", IDCANCEL, 180, 190, 50, 14
END
```

**Step 3: Create src/OptionsDialog.h**

```cpp
#pragma once
#include <windows.h>
#include "PluginInterface.h"

ITMPlugin::OptionReturn ShowPluginOptionsDialog(HWND hParent);
```

**Step 4: Create src/OptionsDialog.cpp**

```cpp
#include "OptionsDialog.h"
#include "ConfigManager.h"
#include "ZteMifiPlugin.h"
#include "resource.h"
#include <string>

// Get HINSTANCE of this DLL
static HINSTANCE GetDllInstance()
{
    HMODULE hModule = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCWSTR)&GetDllInstance, &hModule);
    return (HINSTANCE)hModule;
}

static INT_PTR CALLBACK OptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        const auto& config = ConfigManager::Instance().Config();
        SetDlgItemTextW(hDlg, IDC_EDIT_IP, config.device_ip.c_str());
        SetDlgItemInt(hDlg, IDC_EDIT_INTERVAL, config.update_interval, FALSE);
        CheckDlgButton(hDlg, IDC_CHECK_OPERATOR, config.items.operator_enabled ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_SIGNAL, config.items.signal_enabled ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_OP_SIGNAL, config.items.operator_signal_enabled ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_UPDOWN_SPEED, config.items.up_down_speed_enabled ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_UP_SPEED, config.items.upload_speed_enabled ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_DOWN_SPEED, config.items.download_speed_enabled ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            auto& config = ConfigManager::Instance().Config();
            wchar_t buf[256] = {};
            GetDlgItemTextW(hDlg, IDC_EDIT_IP, buf, 256);
            config.device_ip = buf;
            config.update_interval = GetDlgItemInt(hDlg, IDC_EDIT_INTERVAL, NULL, FALSE);
            if (config.update_interval < 500) config.update_interval = 500;

            config.items.operator_enabled = IsDlgButtonChecked(hDlg, IDC_CHECK_OPERATOR) == BST_CHECKED;
            config.items.signal_enabled = IsDlgButtonChecked(hDlg, IDC_CHECK_SIGNAL) == BST_CHECKED;
            config.items.operator_signal_enabled = IsDlgButtonChecked(hDlg, IDC_CHECK_OP_SIGNAL) == BST_CHECKED;
            config.items.up_down_speed_enabled = IsDlgButtonChecked(hDlg, IDC_CHECK_UPDOWN_SPEED) == BST_CHECKED;
            config.items.upload_speed_enabled = IsDlgButtonChecked(hDlg, IDC_CHECK_UP_SPEED) == BST_CHECKED;
            config.items.download_speed_enabled = IsDlgButtonChecked(hDlg, IDC_CHECK_DOWN_SPEED) == BST_CHECKED;

            ConfigManager::Instance().Save();
            ZteMifiPlugin::Instance().RebuildActiveItems();

            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

ITMPlugin::OptionReturn ShowPluginOptionsDialog(HWND hParent)
{
    INT_PTR result = DialogBoxW(GetDllInstance(), MAKEINTRESOURCEW(IDD_OPTIONS_DIALOG), hParent, OptionsDlgProc);
    if (result == IDOK)
        return ITMPlugin::OR_OPTION_CHANGED;
    return ITMPlugin::OR_OPTION_UNCHANGED;
}
```

**Step 5: Commit**

```
feat: add Win32 options dialog for plugin configuration
```

---

### Task 8: Final Integration and Build Verification

**Files:**
- Verify: all files compile and link correctly

**Step 1: Update dllmain.cpp to include OnExtenedInfo config load as fallback**

No changes needed - `OnExtenedInfo` in `ZteMifiPlugin.cpp` already handles `EI_CONFIG_DIR`.

**Step 2: Verify resource.h does not conflict with windows.h IDOK/IDCANCEL**

Remove `IDOK` and `IDCANCEL` defines from `resource.h` since they're already defined in `windows.h`.

Updated `src/resource.h`:
```cpp
#pragma once

#define IDD_OPTIONS_DIALOG      101

#define IDC_STATIC_IP           1001
#define IDC_EDIT_IP             1002
#define IDC_STATIC_INTERVAL     1003
#define IDC_EDIT_INTERVAL       1004

#define IDC_CHECK_OPERATOR      1010
#define IDC_CHECK_SIGNAL        1011
#define IDC_CHECK_OP_SIGNAL     1012
#define IDC_CHECK_UPDOWN_SPEED  1013
#define IDC_CHECK_UP_SPEED      1014
#define IDC_CHECK_DOWN_SPEED    1015
```

**Step 3: Copy build output for deployment**

After successful build, the user should:
1. Copy `build\ZteMifiPlugin.dll` to TrafficMonitor's `plugins` directory
2. Copy `ZteMifiPlugin.ini` to TrafficMonitor's `plugins` directory (or the config dir)
3. Restart TrafficMonitor and enable the plugin

**Step 4: Final commit**

```
feat: finalize ZTE MiFi plugin - ready for build
```

---

### Task Summary

| Task | Description | Key Files |
|------|-------------|-----------|
| 1 | Project skeleton | `PluginInterface.h`, `dllmain.cpp`, `build.bat` |
| 2 | URL parser | `UrlParser.h/cpp` |
| 3 | HTTP client | `HttpClient.h/cpp` |
| 4 | Config manager | `ConfigManager.h/cpp`, `.ini` |
| 5 | Data item classes | `ZteMifiItem.h/cpp` |
| 6 | Main plugin class | `ZteMifiPlugin.h/cpp` |
| 7 | Options dialog | `OptionsDialog.h/cpp`, `resource.h`, `.rc` |
| 8 | Integration & build | Verify all compiles |
