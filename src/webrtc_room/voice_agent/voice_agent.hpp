#ifndef VOICE_AGENT_HPP
#define VOICE_AGENT_HPP
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "utils/data_buffer.hpp"
#include "webrtc_room/fast_jitterbuffer.hpp"
#include "ws_message/ws_protoo_client.hpp"
#include "voice_agent_pub.hpp"

#include <uv.h>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

namespace cpp_streamer
{

class VoiceAgent : public JitterBufferCallbackI
                , public TimerInterface
                , public WsProtooClientCallbackI
{
public:
    VoiceAgent(const std::string& room_id, const std::string& user_id,
        int clock_rate, 
        VoiceAgentCallbackI* cb,
        uv_loop_t* uv_loop, Logger* logger);
    virtual ~VoiceAgent();

public:
    void PushRtpPacket(RtpPacket* rtp_packet);
    void Close();

protected:// JitterBufferCallbackI implementation
    virtual void OnJitterBufferRtpPacket(std::vector<RtpPacket*> rtp_packets, bool discard) override;

protected:// TimerInterface implementation
    virtual bool OnTimer() override;

protected:// WsProtooClientCallbackI implementation
    virtual void OnConnected() override;
    virtual void OnResponse(const std::string& text) override;
    virtual void OnNotification(const std::string& text) override;
    virtual void OnClosed(int code, const std::string& reason) override;

private:
    void ConnectWsProtooServer();
    void SendAudio2ProtooServer(uint8_t* data, size_t data_len, const std::string& codec);
    void SendHeartbeatToProtooServer(int64_t now_ms);

private:
    Logger* logger_ = nullptr;
    int clock_rate_ = 0;
    std::string room_id_;
    std::string user_id_;
    std::unique_ptr<FastJitterBuffer> jitter_buffer_ptr_;
    VoiceAgentCallbackI* voice_agent_cb_ = nullptr;

private:
    std::unique_ptr<WsProtooClient> ws_protoo_client_ptr_;
    bool protoo_connected_ = false;
    int64_t last_try_connect_ms_ = 0;
    int64_t last_heartbeat_ms_ = 0;
    int64_t voice_data_index_ = 0;
    int64_t dbg_count_ = 0;

private:
    bool closed_ = false;

private://tts
    int64_t tts_opus_pts_ms_ = 0;
};

}

#endif //VOICE_AGENT_HPP