# RTCPilot 配置指南

说明：本文件基于 `RTCPilot/RTCPilot/config.yaml`，逐项说明配置字段含义与推荐值。修改配置后请重启服务以生效。

## 日志（`log`）
- `log_level`: 日志等级，可选 `debug`、`info`、`warn`、`error`。开发调试使用 `debug`。
- `log_path`: 日志输出文件路径，例如 `server0.log`。

## WebSocket 服务（`websocket_server`）
- `listen_ip`: 绑定监听的 IP（例如 `0.0.0.0` 表示所有网卡）。
- `port`: WebSocket 监听端口（例如 `7443`）。

## WebRTC 候选/网络接口（`candidates`）
`candidates` 是数组，每项为一个对等网络接口配置，常用于多网卡或指定公网映射：
- `nettype`: 网络类型（如 `udp`）。
- `candidate_ip`: 对外公布的候选 IP（peer 将看到的 IP，通常是公网 IP 或映射地址）。
- `listen_ip`: 本地绑定的监听 IP（服务器实际绑定地址，通常 `0.0.0.0` 或内网地址）。
- `port`: 对应的端口号（RTP/UDP 端口）。

示例用途：当服务器在 NAT 或具有多个网卡时，`candidate_ip` 可设置为公网地址，`listen_ip` 设置为本机绑定地址。

## 证书（`cert_path` / `key_path`）
- `cert_path`: TLS/SSL 证书文件路径（用于 WebSocket wss 或 DTLS）。
- `key_path`: 私钥文件路径。请确保证书与私钥配对且权限安全。

## 丢包注入（`downlink_discard_percent` / `uplink_discard_percent`）
- `downlink_discard_percent`: 下行丢包率（%），用于测试接收端行为，默认 `0`。
- `uplink_discard_percent`: 上行丢包率（%），用于测试发送端行为，默认 `0`。

## 集群中心（`pilot_center`）
- `enable`: 是否启用与 `pilot_center` 的通信（`true`/`false`）。
- `host`: `pilot_center` 服务地址（IP 或域名）。
- `port`: `pilot_center` 端口。
- `subpath`: 注册/上报的路径前缀（例如 `/pilot/center`）。

说明：当启用时，SFU 会向 `pilot_center` 注册自身信息以实现服务发现与转发。请确保 `pilot_center` 服务可达并按 `pilot_center/requirements.txt` 的说明启动。

## RTC 中继（`rtc_relay`）
- `relay_server_ip`: 中继服务器 IP（当使用中继转发 RTP 时，指定中继地址）。
- `relay_udp_start` / `relay_udp_end`: 中继使用的 UDP 端口范围（转发时分配端口区间）。
- `send_discard_percent` / `recv_discard_percent`: 中继发送/接收的丢包注入百分比（用于测试）。

## 语音代理（`voice_agent`）
- `enable`: 是否启用语音代理功能（`true`/`false`）。
- `agent_ip`: VoiceAgent服务绑定的监听 IP。
- `agent_port`: VoiceAgent服务监听端口。
- `subpath`: VoiceAgent服务注册路径前缀（例如 `/voiceagent`）。

voice_agent服务的开源地址: [https://github.com/runner365/VoiceAgent](https://github.com/runner365/VoiceAgent)
