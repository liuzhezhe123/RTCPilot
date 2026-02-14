#ifndef VOICE_AGENT_HPP
#define VOICE_AGENT_HPP
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "utils/data_buffer.hpp"
#include "webrtc_room/fast_jitterbuffer.hpp"
#include "transcode/ffmpeg_include.h"
#include "transcode/decoder/decoder.h"
#include "transcode/filter/media_filter.h"
#include "ws_message/ws_protoo_client.hpp"
#include "voice_agent_pub.hpp"
#include "tts.hpp"
#include "pcm2opus.hpp"

#include <uv.h>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

namespace cpp_streamer
{

class TtsMessage
{
public:
    TtsMessage() {};
    TtsMessage(const std::string& c_id, const std::string& t, int64_t idx, int64_t ts)
        : conversation_id(c_id), text(t), index(idx), ts_ms(ts) {};
    ~TtsMessage() = default;
public:
    std::string conversation_id;
    std::string text;
    int64_t index = 0;
    int64_t ts_ms = 0;
};

class VoiceAgent : public JitterBufferCallbackI
                , public SinkCallbackI
                , public TimerInterface
                , public WsProtooClientCallbackI
                , public Pcm2OpusCallbackI
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

protected:// SinkCallbackI implementation
    virtual void OnData(std::shared_ptr<FFmpegMediaPacket> pkt) override;

protected:// TimerInterface implementation
    virtual bool OnTimer() override;

protected:// WsProtooClientCallbackI implementation
    virtual void OnConnected() override;
    virtual void OnResponse(const std::string& text) override;
    virtual void OnNotification(const std::string& text) override;
    virtual void OnClosed(int code, const std::string& reason) override;

protected:// Pcm2OpusCallbackI implementation
    virtual void OnOpusData(const std::vector<uint8_t>& opus_data, int sample_rate, int channels, int64_t pts, int task_index) override;
    
private:
    void ConnectWsProtooServer();
    void SendAudio2ProtooServer(uint8_t* data, size_t data_len);
    void SendHeartbeatToProtooServer(int64_t now_ms);

private:
    void DecodeAvPacket(AVPacket* av_pkt);

private:
    void InsertAudioBufferToQueue(std::shared_ptr<DataBuffer> audio_buffer);
    std::shared_ptr<DataBuffer> GetAudioBufferFromQueue();
    size_t GetAudioBufferQueueSize();

private:
    void StartTtsThread();
    void StopTtsThread();
    void TtsThreadFunc();
    void InsertTtsMessage(const TtsMessage& msg);
    TtsMessage GetTtsMessage();
    size_t GetTtsMessageQueueSize();

private:
    Logger* logger_ = nullptr;
    int clock_rate_ = 0;
    std::string room_id_;
    std::string user_id_;
    std::unique_ptr<FastJitterBuffer> jitter_buffer_ptr_;
    VoiceAgentCallbackI* voice_agent_cb_ = nullptr;

private:
    std::unique_ptr<Decoder> audio_decoder_ptr_;
    std::unique_ptr<MediaFilter> audio_filter_ptr_;

private:
    std::queue<std::shared_ptr<DataBuffer>> audio_buffer_queue_;
    std::mutex audio_buffer_queue_mutex_;

private:
    std::unique_ptr<WsProtooClient> ws_protoo_client_ptr_;
    bool protoo_connected_ = false;
    int64_t last_try_connect_ms_ = 0;
    int64_t last_heartbeat_ms_ = 0;
    int64_t voice_data_index_ = 0;

private:
    bool closed_ = false;

private://tts
    bool tts_running_ = false;
    int64_t tts_msg_index_ = 0;
    std::unique_ptr<std::thread> tts_thread_ptr_;
    std::queue<TtsMessage> tts_message_queue_;
    std::mutex tts_message_queue_mutex_;
    std::condition_variable tts_message_queue_cv_;
    std::unique_ptr<TTSImpl> tts_impl_ptr_;
    std::unique_ptr<Pcm2Opus> pcm2opus_ptr_;
};

}

#endif //VOICE_AGENT_HPP