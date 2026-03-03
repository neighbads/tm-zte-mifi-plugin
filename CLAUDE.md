# CLAUDE.md

## Project Overview

TrafficMonitor plugin DLL for monitoring ZTE 5G MiFi devices. Written in C++17, targets Windows x64.

## Build

```bat
build.bat
```

Requires Visual Studio 2022 with C++ desktop workload. Output: `build\ZteMifiPlugin.dll`.

## Architecture

- **Plugin interface**: `include/PluginInterface.h` (API v7, do not modify)
- **Main plugin**: `src/ZteMifiPlugin.h/cpp` — singleton, implements `ITMPlugin`
- **Display items**: `src/ZteMifiItem.h/cpp` — 6 item types via `ItemType` enum
- **HTTP client**: `src/HttpClient.h/cpp` — WinHTTP synchronous GET
- **Response parser**: `src/UrlParser.h/cpp` — auto-detects JSON or URL params
- **Config**: `src/ConfigManager.h/cpp` — INI file read/write + debug logging
- **Dialog**: `src/OptionsDialog.h/cpp/rc` — reserved, currently returns `OR_OPTION_NOT_PROVIDED`

## Key Patterns

- All classes use singleton pattern with static `m_instance`
- Wide strings (`std::wstring`, `L"..."`) throughout for Windows Unicode API
- Chinese text encoded as `\xNNNN` hex escapes in source (compiled with `/utf-8`)
- `GetItemLableText()` returns `L" "` (space, not empty) — empty causes TrafficMonitor to fall back to `GetItemName()` as label
- ZTE API returns JSON (`{"key":value,...}`), parser auto-detects format
- `ppp_status` contains various forms like `"ipv4_ipv6_connected"` — use `find("connected")` not exact match
- Speed values from API are KB/s * 100, divide by 100.0 before display
- Config file is optional; Windows `GetPrivateProfileXxx` returns defaults when file absent

## ZTE Device API

Endpoint: `http://{DeviceIP}/goform/goform_get_cmd_process`

Query: `?multi_data=1&isTest=false&cmd=ppp_status,network_provider_fullname,network_signalbar,flux_realtime_tx_thrpt,flux_realtime_rx_thrpt,network_type`

Returns JSON with string values for all fields.

## No External Dependencies

Only Windows SDK libs: `winhttp.lib`, `user32.lib`, `gdi32.lib`, `comdlg32.lib`.
