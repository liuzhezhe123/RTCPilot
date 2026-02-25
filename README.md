
English: [中文](README_CN.md)

# RTCPilot

RTCPilot is an open-source WebRTC SFU (Selective Forwarding Unit) implemented in modern C++.

## Key features
- High-performance WebRTC SFU for real-time media forwarding.
- Cross-platform: Windows 11, Linux (recommended Debian), and macOS.
- Supports SFU clustering for scalable deployments.
- Written in modern C++ (requires C++17 or later).

## Repository layout (selected)
- `src/` — C++ source code for the SFU and supporting libraries.
- `pilot_center/` — Python-based cluster management service. Handles SFU registration and information forwarding.
- `3rdparty/`, `win_3rdparty/` — bundled third-party libraries and build helpers.

## WebRTC JS client
- Open-source browser demo client: [https://github.com/runner365/webrtc_js_client](https://github.com/runner365/webrtc_js_client)


For more details, see the `voice_agent` section in the configuration files: [config_guide.md](config_guide.md) (Chinese) and [config_guide_en.md](config_guide_en.md) (English).

## Signaling (WebSocket / protoo)
- RTCPilot uses a JSON-over-WebSocket signaling protocol (protoo-style). Main message types include:
	- `join`: client joins a room and receives current users and their `pushers` (media streams).
	- `push`: client pushes an SDP offer to publish media to the SFU and receives an SDP answer.
	- `pull`: client requests to pull media (audio/video) from a target user, specifying `pusher_id` per stream.
	- Notifications from server: `newUser`, `newPusher`, `userLeft` for room membership and stream updates.
- Requests typically include `request`, `id`, `method`, and `data`; responses carry `ok/response`, `id`, and `data` (code, message, sdp, users, etc.).
- See `ws_design.md` for full message examples and field descriptions.

## Supported platforms and build

### Windows 11 (Visual Studio)
- Recommended: Visual Studio Community 2022 (tested with 17.14.16).
- Open the provided Visual Studio solution `RTCPilot.sln`, choose x64 Debug/Release and build.
- Ensure required third-party libraries (OpenSSL, libuv, libsrtp, yaml-cpp) are available in `win_3rdparty` or installed on the system.

### Linux (recommended Debian)
- Requires: C++17 (or newer), `cmake`, `gcc`/`clang`, and typical build tools.
- Example build steps:
```bash
sudo apt update
sudo apt install -y build-essential cmake git libssl-dev
mkdir build && cd build
cmake ..
make -j 2
```

### macOS
- Build with CMake (Xcode or clang toolchain).
- Example:

```bash
mkdir build && cd build
cmake ..
make -j 2
```

## Cluster and `pilot_center`
- The `pilot_center` directory contains a Python-based cluster management service.
- It registers SFU nodes and forwards SFU information between services to enable clustering and discovery.
- See `pilot_center/requirements.txt` and `pilot_center/pilot_center.py` for quick startup instructions.

## Voice Agent integration (`voice_agent`)
RTCPilot SFU supports integrating a voice agent (`voice_agent`) to handle voice interactions with AI large language models.

**Voice Agent** is an advanced "real-time voice conversation AI" agent. The open-source repository for voice_agent service: [https://github.com/runner365/VoiceAgent](https://github.com/runner365/VoiceAgent)

Steps to integrate VoiceAgent:
1. Deploy the VoiceAgent service and ensure it is running properly.
2. Enable the voice agent functionality in the RTCPilot configuration file (`enable: true`).
3. Configure the VoiceAgent service IP address, port, and registration path prefix (`agent_ip`, `agent_port`, `subpath`).
```yaml
voice_agent:
  enable: true
  agent_ip: "192.168.1.221"
  agent_port: 5555
  subpath: "/voiceagent"
```

## Configuration
- The project includes YAML-based configuration files (for example `RTCPilot/config.yaml`).
- Adjust network, logging and SFU parameters in the config files before running.
- Detailed configuration guide: [config_guide.md](config_guide.md) (Chinese) and [config_guide_en.md](config_guide_en.md) (English).

## Requirements
- C++ compiler with C++17 support or newer.
- CMake 3.10+ recommended for cross-platform builds.
- Platform-specific native dependencies (OpenSSL, libsrtp, libuv, yaml-cpp). See the `3rdparty` and `win_3rdparty` directories.
