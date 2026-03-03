#include "OptionsDialog.h"
#include "ConfigManager.h"
#include "ZteMifiPlugin.h"
#include "resource.h"
#include <string>

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
