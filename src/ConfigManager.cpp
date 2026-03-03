#include "ConfigManager.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>

ConfigManager ConfigManager::m_instance;

ConfigManager::ConfigManager() {}

ConfigManager& ConfigManager::Instance()
{
    return m_instance;
}

static void WritePrivateProfileIntHelper(const wchar_t* section, const wchar_t* key, int value, const wchar_t* path)
{
    wchar_t buf[16];
    swprintf_s(buf, L"%d", value);
    WritePrivateProfileStringW(section, key, buf, path);
}

void ConfigManager::Load(const std::wstring& config_dir)
{
    HMODULE hModule = NULL;
    wchar_t modulePath[MAX_PATH] = {};
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCWSTR)&ConfigManager::Instance, &hModule);
    GetModuleFileNameW(hModule, modulePath, MAX_PATH);

    std::wstring dllPath(modulePath);
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
    size_t dotPos = m_config_path.rfind(L'.');
    if (dotPos != std::wstring::npos)
        m_config_path = m_config_path.substr(0, dotPos);
    m_config_path += L".ini";

    // Setup log path: same directory, same name but .log extension
    m_log_path = m_config_path.substr(0, m_config_path.size() - 4) + L".log";

    // Read debug flag first (default 0 = off)
    m_debug = GetPrivateProfileIntW(L"General", L"Debug", 0, m_config_path.c_str()) != 0;

    Log("=== Plugin loaded ===");

    // INI file may not exist - GetPrivateProfileXxx returns defaults in that case
    wchar_t buf[256] = {};
    GetPrivateProfileStringW(L"General", L"DeviceIP", L"192.168.0.1", buf, 256, m_config_path.c_str());
    m_config.device_ip = buf;
    m_config.update_interval = GetPrivateProfileIntW(L"General", L"UpdateInterval", 1000, m_config_path.c_str());

    m_config.items.operator_enabled = GetPrivateProfileIntW(L"Items", L"Operator", 1, m_config_path.c_str()) != 0;
    m_config.items.signal_enabled = GetPrivateProfileIntW(L"Items", L"SignalStrength", 1, m_config_path.c_str()) != 0;
    m_config.items.operator_signal_enabled = GetPrivateProfileIntW(L"Items", L"OperatorWithSignal", 1, m_config_path.c_str()) != 0;
    m_config.items.up_down_speed_enabled = GetPrivateProfileIntW(L"Items", L"UpDownSpeed", 1, m_config_path.c_str()) != 0;
    m_config.items.upload_speed_enabled = GetPrivateProfileIntW(L"Items", L"UploadSpeed", 1, m_config_path.c_str()) != 0;
    m_config.items.download_speed_enabled = GetPrivateProfileIntW(L"Items", L"DownloadSpeed", 1, m_config_path.c_str()) != 0;
}

void ConfigManager::Save() const
{
    if (m_config_path.empty())
        return;

    WritePrivateProfileStringW(L"General", L"DeviceIP", m_config.device_ip.c_str(), m_config_path.c_str());
    WritePrivateProfileIntHelper(L"General", L"UpdateInterval", m_config.update_interval, m_config_path.c_str());

    WritePrivateProfileIntHelper(L"Items", L"Operator", m_config.items.operator_enabled ? 1 : 0, m_config_path.c_str());
    WritePrivateProfileIntHelper(L"Items", L"SignalStrength", m_config.items.signal_enabled ? 1 : 0, m_config_path.c_str());
    WritePrivateProfileIntHelper(L"Items", L"OperatorWithSignal", m_config.items.operator_signal_enabled ? 1 : 0, m_config_path.c_str());
    WritePrivateProfileIntHelper(L"Items", L"UpDownSpeed", m_config.items.up_down_speed_enabled ? 1 : 0, m_config_path.c_str());
    WritePrivateProfileIntHelper(L"Items", L"UploadSpeed", m_config.items.upload_speed_enabled ? 1 : 0, m_config_path.c_str());
    WritePrivateProfileIntHelper(L"Items", L"DownloadSpeed", m_config.items.download_speed_enabled ? 1 : 0, m_config_path.c_str());
}

void ConfigManager::Log(const char* format, ...)
{
    if (!m_debug || m_log_path.empty())
        return;

    FILE* f = nullptr;
    _wfopen_s(&f, m_log_path.c_str(), L"a");
    if (!f) return;

    time_t now = time(nullptr);
    struct tm t = {};
    localtime_s(&t, &now);
    fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
            t.tm_hour, t.tm_min, t.tm_sec);

    va_list args;
    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);

    fprintf(f, "\n");
    fclose(f);
}
