#pragma once
#include <string>
#include <windows.h>

struct ItemConfig
{
    bool operator_enabled = true;
    bool signal_enabled = true;
    bool operator_signal_enabled = true;
    bool up_down_speed_enabled = true;
    bool upload_speed_enabled = true;
    bool download_speed_enabled = true;
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

    // Log to ZteMifiPlugin.log, only when debug=1 in INI
    void Log(const char* format, ...);

private:
    ConfigManager();
    static ConfigManager m_instance;
    PluginConfig m_config;
    std::wstring m_config_path;
    std::wstring m_log_path;
    bool m_debug = false;
};
