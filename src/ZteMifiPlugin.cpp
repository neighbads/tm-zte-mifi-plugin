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

    std::wstring path = L"/goform/goform_get_cmd_process?multi_data=1&isTest=false"
        L"&cmd=ppp_status%2Cnetwork_provider_fullname%2Cnetwork_signalbar"
        L"%2Cflux_realtime_tx_thrpt%2Cflux_realtime_rx_thrpt%2Cnetwork_type";

    int errorCode = 0;
    int timeout = 3000;
    std::string response = m_http_client.Get(config.device_ip, 80, path, timeout, errorCode);

    ConfigManager::Instance().Log("HTTP GET errorCode=%d, responseLen=%d", errorCode, (int)response.size());

    if (errorCode != 0)
    {
        m_fail_count++;
        ConfigManager::Instance().Log("Request failed, errorCode=%d, failCount=%d", errorCode, m_fail_count);
        if (errorCode == 2)
            SetAllItemsText(L"\x9700\x8981\x767B\x5F55");
        else
            SetAllItemsText(L"\x8BBE\x5907\x79BB\x7EBF");
        return;
    }

    m_fail_count = 0;

    // Log raw response (first 2000 chars)
    std::string logResponse = response.substr(0, 2000);
    ConfigManager::Instance().Log("Raw response: %s", logResponse.c_str());

    auto params = UrlParser::Parse(response);
    ConfigManager::Instance().Log("Parsed params count: %d", (int)params.size());

    if (params.empty())
    {
        ConfigManager::Instance().Log("Parse returned empty, showing data error");
        SetAllItemsText(L"\x6570\x636E\x5F02\x5E38");
        return;
    }

    // Log key values
    for (const auto& kv : params)
    {
        ConfigManager::Instance().Log("  %s = %s", kv.first.c_str(), kv.second.c_str());
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
    std::string netType = UrlParser::GetValue(params, "network_type");

    bool connected = (pppStatus.find("connected") != std::string::npos);
    int signalVal = 0;
    try { signalVal = std::stoi(signalBar); } catch (...) {}

    // Convert network_type to wide string
    m_network_type = L"";
    if (!netType.empty())
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, netType.c_str(), -1, NULL, 0);
        if (len > 0)
        {
            m_network_type.resize(len - 1);
            MultiByteToWideChar(CP_UTF8, 0, netType.c_str(), -1, &m_network_type[0], len);
        }
    }

    // Convert provider name to wide string
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

    // Operator: 中国电信√ or 中国电信×
    std::wstring opText = wProvider + (connected ? L"\x2714" : L"\x00D7");
    m_operator_item.SetValueText(opText);

    // Signal: ▂▄▆█ or ××××
    std::wstring sigText = FormatSignal(signalVal, connected);
    m_signal_item.SetValueText(sigText);

    // Operator + Signal
    m_operator_signal_item.SetValueText(wProvider + L" " + sigText);

    // Speed formatting: "5G ↑: 14.3 KB/s"
    std::wstring upSpeed = FormatSpeed(txThrpt);
    std::wstring downSpeed = FormatSpeed(rxThrpt);
    std::wstring prefix = m_network_type.empty() ? L"" : m_network_type + L" ";

    // Up+Down: "5G ↑: 14.3 KB/s ↓: 56.7 KB/s"
    m_up_down_speed_item.SetValueText(prefix + L"\x2191: " + upSpeed + L" \x2193: " + downSpeed);

    // Upload: "5G ↑: 14.3 KB/s"
    m_upload_speed_item.SetValueText(prefix + L"\x2191: " + upSpeed);

    // Download: "5G ↓: 56.7 KB/s"
    m_download_speed_item.SetValueText(prefix + L"\x2193: " + downSpeed);

    // Tooltip
    m_tooltip_info = wProvider + L" " + sigText + L"\n"
        + prefix + L"\x2191: " + upSpeed + L"\n"
        + prefix + L"\x2193: " + downSpeed;
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
        return L"\x00D7\x00D7\x00D7\x00D7";

    const wchar_t bars[] = { L'\x2582', L'\x2584', L'\x2586', L'\x2588' };
    std::wstring result;

    int level = signalBar;
    if (level < 0) level = 0;
    if (level > 5) level = 5;

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
    val /= 100.0; // API returns KB/s * 100, now val is KB/s

    wchar_t buf[64];
    if (val >= 1024.0 * 1024.0)
    {
        swprintf_s(buf, L"%.1f GB/s", val / (1024.0 * 1024.0));
    }
    else if (val >= 1024.0)
    {
        swprintf_s(buf, L"%.1f MB/s", val / 1024.0);
    }
    else
    {
        swprintf_s(buf, L"%.1f KB/s", val);
    }
    return buf;
}

const wchar_t* ZteMifiPlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:          return L"ZTE 5G \x76D1\x63A7";  // ZTE 5G 监控
    case TMI_DESCRIPTION:   return L"ZTE 5G \x968F\x8EAB WiFi \x76D1\x63A7\x63D2\x4EF6";
    case TMI_AUTHOR:        return L"neighbads";
    case TMI_COPYRIGHT:     return L"Copyright (C) 2026 neighbads";
    case TMI_VERSION:       return L"1.0";
    case TMI_URL:           return L"https://github.com/neighbads/tm-zte-mifi-plugin";
    default: break;
    }
    return L"";
}

ITMPlugin::OptionReturn ZteMifiPlugin::ShowOptionsDialog(void* hParent)
{
    return OR_OPTION_NOT_PROVIDED;
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

ITMPlugin* TMPluginGetInstance()
{
    return &ZteMifiPlugin::Instance();
}
