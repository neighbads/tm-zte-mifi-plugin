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
    case ItemType::Operator:        return L"ZTE \x8FD0\x8425\x5546";
    case ItemType::SignalStrength:   return L"ZTE \x4FE1\x53F7\x5F3A\x5EA6";
    case ItemType::OperatorSignal:   return L"ZTE \x8FD0\x8425\x5546+\x4FE1\x53F7";
    case ItemType::UpDownSpeed:      return L"ZTE \x4E0A\x4E0B\x8F7D\x901F\x5EA6";
    case ItemType::UploadSpeed:      return L"ZTE \x4E0A\x4F20\x901F\x5EA6";
    case ItemType::DownloadSpeed:    return L"ZTE \x4E0B\x8F7D\x901F\x5EA6";
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
    return L" ";
}

const wchar_t* ZteMifiItem::GetItemValueText() const
{
    return m_value_text.c_str();
}

const wchar_t* ZteMifiItem::GetItemValueSampleText() const
{
    switch (m_type)
    {
    case ItemType::Operator:        return L"\x4E2D\x56FD\x79FB\x52A8\x2714";
    case ItemType::SignalStrength:   return L"\x2582\x2584\x2586\x2588";
    case ItemType::OperatorSignal:   return L"\x4E2D\x56FD\x79FB\x52A8 \x2582\x2584\x2586\x2588";
    case ItemType::UpDownSpeed:      return L"5G \x2191: 999.9 MB/s \x2193: 999.9 MB/s";
    case ItemType::UploadSpeed:      return L"5G \x2191: 999.9 MB/s";
    case ItemType::DownloadSpeed:    return L"5G \x2193: 999.9 MB/s";
    }
    return L"";
}

void ZteMifiItem::SetValueText(const std::wstring& text)
{
    m_value_text = text;
}
