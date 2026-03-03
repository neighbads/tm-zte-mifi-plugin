# ZTE 5G MiFi Plugin for TrafficMonitor - Design Document

## Overview

TrafficMonitor 插件，用于监控 ZTE 5G 随身 WiFi 设备状态，包括网络速度、信号强度、运营商信息等。

## Architecture

### Tech Stack
- C++ DLL (Windows)
- WinHTTP API (HTTP 请求)
- Win32 API (选项对话框)
- INI 配置文件

### Project Structure
```
tm-zte-mifi-plugin/
├── src/
│   ├── ZteMifiPlugin.cpp/h      # 主插件类 (ITMPlugin)
│   ├── ZteMifiItem.cpp/h        # 数据项类 (IPluginItem)
│   ├── HttpClient.cpp/h         # WinHTTP 封装
│   ├── UrlParser.cpp/h          # URL 参数解析 (key=value&)
│   ├── ConfigManager.cpp/h      # INI 配置管理
│   ├── OptionsDialog.cpp/h      # Win32 选项对话框
│   ├── resource.h               # 资源定义
│   └── OptionsDialog.rc         # 对话框资源
├── include/
│   └── PluginInterface.h        # TrafficMonitor 插件接口
├── build.bat                    # cl 编译脚本
├── ZteMifiPlugin.ini            # 配置文件模板
└── README.md                    # 使用说明
```

### Data Flow
1. TrafficMonitor 定期调用 `DataRequired()`
2. 插件通过 WinHTTP 异步请求 ZTE API (`http://<IP>/goform/goform_get_cmd_process`)
3. 解析 URL 参数格式响应 (`key=value&key=value`)
4. 更新各数据项的显示文本
5. TrafficMonitor 调用各 Item 的 `GetItemValueText()` 获取显示文本

## Data Items (6 items)

| ID | 显示名称 | 标签 | 示例值 | 数据源 |
|----|---------|------|--------|--------|
| `operator` | 运营商 | 运营商: | 中国移动√ / 中国移动× | `network_provider_fullname` + `ppp_status` |
| `signal_strength` | 信号强度 | 信号: | ▂▄▆█ / ×××× | `network_signalbar` + `ppp_status` |
| `operator_signal` | 运营商+信号 | 网络: | 中国移动 ▂▄▆█ | 组合 |
| `up_down_speed` | 上传下载速度 | 速度: | ↑12.34KB/s ↓56.78KB/s | 组合 |
| `upload_speed` | 上传速度 | 上传: | 12.34 KB/s | `flux_realtime_tx_thrpt` |
| `download_speed` | 下载速度 | 下载: | 56.78 KB/s | `flux_realtime_rx_thrpt` |

### Signal Strength Display
- 连接正常：根据 `network_signalbar` (0-5) 显示从低到高的方块
  - 0-1: `▂`
  - 2: `▂▄`
  - 3: `▂▄▆`
  - 4: `▂▄▆█`
  - 5: `▂▄▆█` (满格)
- 连接断开：显示 `××××`

### Speed Calculation
- API 返回值单位：KB/s × 100
- 显示公式：`value / 100.0` KB/s
- 保留2位小数

### Connection Status
- 不单独显示，合并到运营商和信号中
- `ppp_connected` → √ (运营商) / 信号图标 (信号)
- 其他状态 → × (运营商) / ×××× (信号)

## Configuration

### ZteMifiPlugin.ini
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

### Options Dialog (Win32)
- IP 地址输入框
- 更新间隔输入框
- 6个复选框（对应6个数据项）
- 确定/取消按钮

## Error Handling

| 场景 | 显示 | 行为 |
|------|------|------|
| 设备不可达 | `设备离线` | 继续重试 |
| HTTP 请求失败 (需登录) | `需要登录` | 提示用户浏览器登录 |
| 响应解析失败 | `数据异常` | 继续重试 |
| 特定字段缺失 | `N/A` | 其他字段正常显示 |
| 初始状态 | `正在连接...` | - |

### Retry Strategy
- 请求超时：3秒
- 失败3次后重试间隔从配置间隔延长到5秒

## Build
- Visual Studio 2019+
- cl 编译器
- 链接库：winhttp.lib, user32.lib
- 输出：ZteMifiPlugin.dll (64-bit)
