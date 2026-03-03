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

    virtual IPluginItem* GetItem(int index) override;
    virtual void DataRequired() override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual OptionReturn ShowOptionsDialog(void* hParent) override;
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;
    virtual const wchar_t* GetTooltipInfo() override;

    void RebuildActiveItems();

private:
    void UpdateItemValues(const std::map<std::string, std::string>& params);
    void SetAllItemsText(const std::wstring& text);
    std::wstring FormatSignal(int signalBar, bool connected);
    // Format speed with auto unit (KB/s, MB/s, GB/s), 1 decimal place
    // rawValue is KB/s * 100 from API
    std::wstring FormatSpeed(const std::string& rawValue);

    static ZteMifiPlugin m_instance;

    ZteMifiItem m_operator_item;
    ZteMifiItem m_signal_item;
    ZteMifiItem m_operator_signal_item;
    ZteMifiItem m_up_down_speed_item;
    ZteMifiItem m_upload_speed_item;
    ZteMifiItem m_download_speed_item;

    std::vector<ZteMifiItem*> m_active_items;

    HttpClient m_http_client;
    int m_fail_count;
    std::wstring m_tooltip_info;
    std::wstring m_network_type;  // "5G", "4G", etc.
    ITrafficMonitor* m_app;
};

#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();
#ifdef __cplusplus
}
#endif
