#include "voice_agent.hpp"
#include "transcode/ffmpeg_include.h"
#include "config/config.hpp"
#include "utils/timeex.hpp"
#include "utils/json.hpp"
#include "utils/base64.hpp"

#include <fstream>

using json = nlohmann::json;

namespace cpp_streamer
{
VoiceAgent::VoiceAgent(const std::string& room_id, const std::string& user_id,
    int clock_rate, 
    VoiceAgentCallbackI* cb,
    uv_loop_t* uv_loop, Logger* logger)
    : TimerInterface(20)
    , logger_(logger)
{
    voice_agent_cb_ = cb;
    room_id_ = room_id;
    user_id_ = user_id;
    clock_rate_ = clock_rate;
    jitter_buffer_ptr_.reset(new FastJitterBuffer(this, uv_loop, logger_, 600, clock_rate));

    ws_protoo_client_ptr_.reset(new WsProtooClient(uv_loop,
        Config::Instance().voice_agent_cfg_.agent_ip_,
        Config::Instance().voice_agent_cfg_.agent_port_,
        Config::Instance().voice_agent_cfg_.agent_subpath_,
        false, 
        logger_, 
        this));
    
    if (Config::Instance().voice_agent_cfg_.tts_config_.tts_enable_) {
        tts_impl_ptr_.reset(new TTSImpl(logger_));
        StartTtsThread();
    }
    
    StartTimer();
    LogInfof(logger_, "VoiceAgent constructed for room_id:%s, user_id:%s, clock_rate:%d, tts_enabled:%s", 
        room_id_.c_str(), user_id_.c_str(), clock_rate_, BOOL2STRING(Config::Instance().voice_agent_cfg_.tts_config_.tts_enable_));
}
VoiceAgent::~VoiceAgent()
{
    StopTtsThread();
    StopTimer();
    Close();
    LogInfof(logger_, "VoiceAgent destructed for room_id:%s, user_id:%s, clock_rate:%d", 
        room_id_.c_str(), user_id_.c_str(), clock_rate_);
}

void VoiceAgent::Close() {
    if (closed_) {
        return;
    }
    closed_ = true;
    if (jitter_buffer_ptr_) {
        jitter_buffer_ptr_->Close();
        jitter_buffer_ptr_.reset();
    }
    if (audio_decoder_ptr_) {
        audio_decoder_ptr_->CloseDecoder();
        audio_decoder_ptr_.reset();
    }
    if (audio_filter_ptr_) {
        audio_filter_ptr_.reset();
    }
}
void VoiceAgent::PushRtpPacket(RtpPacket* rtp_packet) {
    if (closed_) {
        return;
    }
    if (jitter_buffer_ptr_) {
        jitter_buffer_ptr_->PushRtpPacket(rtp_packet);
    }
}

void VoiceAgent::OnJitterBufferRtpPacket(std::vector<RtpPacket*> rtp_packets, bool discard) {
    if (closed_) {
        for (RtpPacket* pkt : rtp_packets) {
            delete pkt;
        }
        return;
    }
    
    for (RtpPacket* pkt : rtp_packets) {
        int64_t dts = pkt->GetTimestamp();
        int stream_index = AV_PACKET_TYPE_DEF_AUDIO;
        AVPacket* av_pkt = GenerateAVPacket(pkt->GetPayload(),
            (int)pkt->GetPayloadLength(), dts, dts, stream_index, {1, clock_rate_});
        DecodeAvPacket(av_pkt);
    }
}

void VoiceAgent::DecodeAvPacket(AVPacket* av_pkt) {
    if (closed_) {
        av_packet_free(&av_pkt);
        return;
    }
    if (!audio_decoder_ptr_) {
        audio_decoder_ptr_.reset(new Decoder(logger_));
        audio_decoder_ptr_->SetSinkCallback(this);
    }
    std::shared_ptr<FFmpegMediaPacket> media_pkt_ptr;
    media_pkt_ptr.reset(new FFmpegMediaPacket(av_pkt, MEDIA_AUDIO_TYPE));
    FFmpegMediaPacketPrivate prv;
    prv.private_type_ = PRIVATE_DATA_TYPE_DECODER_ID;
    prv.codec_id_ = AV_CODEC_ID_OPUS;
    
    media_pkt_ptr->SetPrivateData(prv);

    audio_decoder_ptr_->OnData(media_pkt_ptr);
    
}

void VoiceAgent::OnData(std::shared_ptr<FFmpegMediaPacket> pkt) {
    if (closed_) {
        return;
    }
    if (pkt->GetId() == audio_decoder_ptr_->GetId()) {
        //decode avframe
        if (!pkt || !pkt->IsAVFrame()) {
            return;
        }
        AVFrame* frame = pkt->GetAVFrame();
        enum AVSampleFormat sample_fmt = (enum AVSampleFormat)frame->format;

        if (!audio_filter_ptr_) {
            audio_filter_ptr_.reset(new MediaFilter(logger_));
            audio_filter_ptr_->SetSinkCallback(this);

            AudioFilter::Params input_param = {
                .sample_rate = frame->sample_rate,
                .ch_layout = frame->ch_layout,
                .sample_fmt = sample_fmt,
                .time_base = {1, frame->sample_rate}
            };
            //filter_desc: change to 16000, single channel, s16 format
            std::string filter_desc = "aresample=16000,asetrate=16000*1.0,aformat=sample_fmts=s16:channel_layouts=mono";
            audio_filter_ptr_->InitAudioFilter(input_param, filter_desc.c_str());
        }
        audio_filter_ptr_->OnData(pkt);
        
        return;
    }
    if (pkt->GetId() == audio_filter_ptr_->GetId()) {
        // filtered avframe
        if (!pkt || !pkt->IsAVFrame()) {
            return;
        }
        AVFrame* frame = pkt->GetAVFrame();
        enum AVSampleFormat sample_fmt = (enum AVSampleFormat)frame->format;

        size_t num_samples = frame->nb_samples;
        size_t num_channels = frame->ch_layout.nb_channels;

        LogDebugf(logger_, "VoiceAgent OnData() decoded audio frame: pts=%ld, sample_rate=%d, format=%s, channels=%d, nb_samples=%d",
            frame->pts,
            frame->sample_rate,
            av_get_sample_fmt_name(sample_fmt),
            (int)num_channels,
            (int)num_samples
        );
        std::shared_ptr<DataBuffer> audio_buffer = std::make_shared<DataBuffer>();
        size_t data_size = num_samples * num_channels * av_get_bytes_per_sample(sample_fmt);
        audio_buffer->AppendData((char*)frame->data[0], data_size);

        InsertAudioBufferToQueue(audio_buffer);

        // write to pcm16 file for testing
        #if 0
        std::ofstream pcm16_file;
        std::string filename = "va_" + pkt->GetId() + ".pcm";
        pcm16_file.open(filename, std::ios::out | std::ios::app | std::ios::binary);
        if (pcm16_file.is_open()) {
            int16_t* data_ptr = (int16_t*)frame->data[0];
            pcm16_file.write((char*)data_ptr, num_samples * num_channels * sizeof(int16_t));
            pcm16_file.close();
        }
        #endif
        return;
    }
    LogWarnf(logger_, "RtcTask OnData() warning: unknown packet id:%s", pkt->GetId().c_str());
}
bool VoiceAgent::OnTimer() {
    if (closed_) {
        return false;
    }
    int64_t now_ms = now_millisec();
    ConnectWsProtooServer();
    SendHeartbeatToProtooServer(now_ms);
    
    while (GetAudioBufferQueueSize() > 0) {
        auto audio_buffer = GetAudioBufferFromQueue();
        if (!audio_buffer) {
            break;
        }
        // process audio buffer
        if (!protoo_connected_) {
            //discard audio buffer if not connected
            continue;
        }
        SendAudio2ProtooServer((uint8_t*)audio_buffer->Data(), audio_buffer->DataLen());
    }
    return true;
}

void VoiceAgent::SendHeartbeatToProtooServer(int64_t now_ms) {
    if (!protoo_connected_) {
        return;
    }
    if (now_ms - last_heartbeat_ms_ < 5000) {
        return;
    }
    last_heartbeat_ms_ = now_ms;

    json heartbeat_json;
    heartbeat_json["index"] = voice_data_index_++;
    heartbeat_json["roomId"] = room_id_;
    heartbeat_json["userId"] = user_id_;
    heartbeat_json["type"] = "heartbeat";
    ws_protoo_client_ptr_->SendNotification("heartbeat", heartbeat_json.dump());
}

void VoiceAgent::SendAudio2ProtooServer(uint8_t* data, size_t data_len) {
    if (!protoo_connected_) {
        return;
    }
    std::string base64_data = Base64Encode(data, data_len);
    json voice_json;

    voice_json["index"] = voice_data_index_++;
    voice_json["type"] = "input_audio_buffer.append";
    voice_json["roomId"] = room_id_;
    voice_json["userId"] = user_id_;
    voice_json["audio"] = base64_data;

    ws_protoo_client_ptr_->SendNotification("input_audio_buffer.append", voice_json.dump());
}

void VoiceAgent::InsertAudioBufferToQueue(std::shared_ptr<DataBuffer> audio_buffer) {
    std::lock_guard<std::mutex> lock(audio_buffer_queue_mutex_);
    audio_buffer_queue_.push(audio_buffer);
}

std::shared_ptr<DataBuffer> VoiceAgent::GetAudioBufferFromQueue() {
    std::lock_guard<std::mutex> lock(audio_buffer_queue_mutex_);
    if (audio_buffer_queue_.empty()) {
        return nullptr;
    }
    auto buffer = audio_buffer_queue_.front();
    audio_buffer_queue_.pop();
    return buffer;
}

size_t VoiceAgent::GetAudioBufferQueueSize() {
    std::lock_guard<std::mutex> lock(audio_buffer_queue_mutex_);
    return audio_buffer_queue_.size();
}

// WsProtooClientCallbackI implementation
void VoiceAgent::OnConnected() {
    LogInfof(logger_, "VoiceAgent WsProtooClient connected");
    protoo_connected_ = true;

}

void VoiceAgent::OnResponse(const std::string& text) {
    LogInfof(logger_, "VoiceAgent WsProtooClient OnResponse: %s", text.c_str());
}

void VoiceAgent::OnNotification(const std::string& data_str) {
    LogInfof(logger_, "VoiceAgent WsProtooClient OnNotification: %s", data_str.c_str());
    try {
        json notification_json = json::parse(data_str);
        json data_json = notification_json["data"];
        std::string notification_type = data_json["type"];
        int64_t now_ms = data_json["ms"];
        if (notification_type == "input.transcript") {
            std::string recognized_text = data_json["text"];
            if (voice_agent_cb_) {
                voice_agent_cb_->OnVoiceAgentRecognizedText(room_id_, user_id_, recognized_text, now_ms);
            }
        } else if (notification_type == "response.text") {
            std::string response_text = data_json["text"];
            std::string conversation_id = data_json["conversation_id"];
            if (voice_agent_cb_) {
                const std::string ai_user_id = "ai";
                voice_agent_cb_->OnVoiceAgentResponseText(room_id_, ai_user_id, response_text, now_ms);
            }
            TtsMessage msg(conversation_id, response_text, ++tts_msg_index_, now_ms);
            InsertTtsMessage(msg);
        } else if (notification_type == "conversation.start") {
            LogInfof(logger_, "VoiceAgent OnNotification conversation.start:%s", data_json.dump().c_str());
            if (voice_agent_cb_) {
                std::string conversation_id = data_json["conversation_id"];
                voice_agent_cb_->OnVoiceAgentConversationStart(room_id_, conversation_id, now_ms);
            }
        } else if (notification_type == "conversation.end") {
            LogInfof(logger_, "VoiceAgent OnNotification conversation.end:%s", data_json.dump().c_str());
            if (voice_agent_cb_) {
                std::string conversation_id = data_json["conversation_id"];
                voice_agent_cb_->OnVoiceAgentConversationEnd(room_id_, conversation_id, now_ms);
            }
        } else {
            LogWarnf(logger_, "VoiceAgent OnNotification not handle type: %s", notification_type.c_str());
        }
    } catch (const json::parse_error& e) {
        LogErrorf(logger_, "VoiceAgent OnNotification parse error: %s", e.what());
    }
}

void VoiceAgent::OnClosed(int code, const std::string& reason) {
    LogInfof(logger_, "VoiceAgent WsProtooClient closed, code:%d, reason:%s", code, reason.c_str());
    protoo_connected_ = false;
}

void VoiceAgent::ConnectWsProtooServer() {
    if (closed_) {
        return;
    }
    if (protoo_connected_) {
        return;
    }
    int64_t now_ms = now_millisec();
    if (now_ms - last_try_connect_ms_ < 2000) {
        return;
    }
    last_try_connect_ms_ = now_ms;
    ws_protoo_client_ptr_->AsyncConnect();
}

void VoiceAgent::StartTtsThread() {
    if (tts_running_) {
        return;
    }
    tts_running_ = true;
    tts_thread_ptr_.reset(new std::thread(&VoiceAgent::TtsThreadFunc, this));
}
void VoiceAgent::StopTtsThread() {
    if (!tts_running_) {
        return;
    }
    tts_running_ = false;
    tts_message_queue_cv_.notify_one();
    tts_thread_ptr_->join();
    tts_thread_ptr_.reset();
}
void VoiceAgent::TtsThreadFunc() {
    int ret = tts_impl_ptr_->Init();
    if (ret != 0) {
        LogErrorf(logger_, "VoiceAgent TtsThreadFunc tts_impl Init failed, ret:%d", ret);
        tts_running_ = false;
        return;
    }
    while (tts_running_) {
        TtsMessage msg = GetTtsMessage();
        if (msg.text.empty()) {
            continue;
        }
        int32_t sample_rate = 0;
        std::string conversation_id = msg.conversation_id;
        std::vector<float> audio_data;
        ret = tts_impl_ptr_->SynthesizeText(msg.text, sample_rate, audio_data);
        if (ret != 0) {
            LogErrorf(logger_, "VoiceAgent TtsThreadFunc SynthesizeText failed for text:%s, ret:%d", 
                msg.text.c_str(), ret);
            continue;
        }
        if (audio_data.empty()) {
            continue;
        }
        LogInfof(logger_, "VoiceAgent TtsThreadFunc SynthesizeText success for text:%s, c_id:%s, sample_rate:%d, audio_data_size:%d", 
            msg.text.c_str(), conversation_id.c_str(), sample_rate, (int)audio_data.size());
        if (!pcm2opus_ptr_) {
            pcm2opus_ptr_.reset(new Pcm2Opus(this, logger_));
        }
        PCM_DATA_INFO pcm_data(audio_data, sample_rate, 1);
        pcm2opus_ptr_->InsertPcmData(pcm_data);
    }
}

void VoiceAgent::InsertTtsMessage(const TtsMessage& msg) {
    std::lock_guard<std::mutex> lock(tts_message_queue_mutex_);
    if (!tts_running_) {
        return;
    }
    tts_message_queue_.push(msg);
    tts_message_queue_cv_.notify_one();
}

TtsMessage VoiceAgent::GetTtsMessage() {
    std::unique_lock<std::mutex> lock(tts_message_queue_mutex_);
    while (tts_message_queue_.empty() && tts_running_) {
        tts_message_queue_cv_.wait(lock);
    }
    if (!tts_running_) {
        return TtsMessage();
    }
    TtsMessage msg;
    if (!tts_message_queue_.empty()) {
        msg = tts_message_queue_.front();
        tts_message_queue_.pop();
    }
    return msg;
}
size_t VoiceAgent::GetTtsMessageQueueSize() {
    std::lock_guard<std::mutex> lock(tts_message_queue_mutex_);
    return tts_message_queue_.size();
}

// Pcm2OpusCallbackI implementation
void VoiceAgent::OnOpusData(const std::vector<uint8_t>& opus_data, int sample_rate, int channels, int64_t pts, int task_index) {
    if (voice_agent_cb_) {
        voice_agent_cb_->OnVoiceAgentAiOpusData(opus_data, sample_rate, channels, pts, task_index);
    }
}

} // namespace cpp_streamer