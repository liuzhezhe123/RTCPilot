#include "voice_agent.hpp"
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
    
    StartTimer();
    LogInfof(logger_, "VoiceAgent constructed for room_id:%s, user_id:%s, clock_rate:%d, tts_enabled:%s", 
        room_id_.c_str(), user_id_.c_str(), clock_rate_, BOOL2STRING(Config::Instance().voice_agent_cfg_.tts_config_.tts_enable_));
}
VoiceAgent::~VoiceAgent()
{
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
        // send rtp packet to voice agent server
        SendAudio2ProtooServer((uint8_t*)pkt->GetPayload(), pkt->GetPayloadLength(), "opus");
        delete pkt;
    }
}

bool VoiceAgent::OnTimer() {
    if (closed_) {
        return false;
    }
    int64_t now_ms = now_millisec();
    ConnectWsProtooServer();
    SendHeartbeatToProtooServer(now_ms);
    
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

void VoiceAgent::SendAudio2ProtooServer(uint8_t* data, size_t data_len, const std::string& codec) {
    if (!protoo_connected_) {
        return;
    }
    std::string base64_data = Base64Encode(data, data_len);
    json voice_json;

    LogDebugf(logger_, "audio data len:%zu, base64 len:%zu, codec:%s", data_len, base64_data.size(), codec.c_str());
    voice_json["index"] = voice_data_index_++;
    voice_json["type"] = "input_audio_buffer.append";
    voice_json["roomId"] = room_id_;
    voice_json["userId"] = user_id_;
    voice_json["audio"] = base64_data;
    voice_json["codec"] = codec;

    ws_protoo_client_ptr_->SendNotification("input_audio_buffer.append", voice_json.dump());
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
            std::string conversation_id = data_json["conversationId"];
            if (voice_agent_cb_) {
                const std::string ai_user_id = "ai";
                voice_agent_cb_->OnVoiceAgentResponseText(room_id_, ai_user_id, response_text, now_ms);
            }
            // TtsMessage msg(conversation_id, response_text, ++tts_msg_index_, now_ms);
            // InsertTtsMessage(msg);
        } else if (notification_type == "conversation.start") {
            LogInfof(logger_, "VoiceAgent OnNotification conversation.start:%s", data_json.dump().c_str());
            if (voice_agent_cb_) {
                std::string conversation_id = data_json["conversationId"];
                voice_agent_cb_->OnVoiceAgentConversationStart(room_id_, conversation_id, now_ms);
            }
        } else if (notification_type == "conversation.end") {
            LogInfof(logger_, "VoiceAgent OnNotification conversation.end:%s", data_json.dump().c_str());
            if (voice_agent_cb_) {
                std::string conversation_id = data_json["conversationId"];
                voice_agent_cb_->OnVoiceAgentConversationEnd(room_id_, conversation_id, now_ms);
            }
        } else if (notification_type == "tts_opus_data") {
            LogInfof(logger_, "VoiceAgent OnNotification tts_opus_data:%s", data_json.dump().c_str());
            /*
            {
                "conversationId": "3",
                "ms": 1771722564618,
                "msg": "fIgB4vzzcxd2TMlGoA/zY/dxUWbMVoVUe0aA/Mlntsvs3mZTr1JKrZYEhlLaVFT9PsRrLO24h0Bu7dh8AA==",
                "roomId": "a2omtymw",
                "type": "tts_opus_data",
                "userId": "7861"
            }
            */
            std::string conversation_id = data_json["conversationId"];
            std::string base64_data = data_json["msg"];
            std::string opus_data_str = Base64Decode(base64_data);
            if (voice_agent_cb_) {
                std::vector<uint8_t> opus_data(opus_data_str.begin(), opus_data_str.end());
                int task_index = atoi(conversation_id.c_str());
                int64_t pts = tts_opus_pts_ms_ * 48000 / 1000;
                voice_agent_cb_->OnVoiceAgentAiOpusData(opus_data, 48000, 2, pts, task_index);
                tts_opus_pts_ms_ += 20;//20ms
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

} // namespace cpp_streamer