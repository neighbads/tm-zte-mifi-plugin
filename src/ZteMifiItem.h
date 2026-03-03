#pragma once
#include "PluginInterface.h"
#include <string>

enum class ItemType
{
    Operator,
    SignalStrength,
    OperatorSignal,
    UpDownSpeed,
    UploadSpeed,
    DownloadSpeed
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
