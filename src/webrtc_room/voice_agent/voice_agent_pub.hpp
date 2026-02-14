#ifndef VOICE_AGENT_PUB_HPP_
#define VOICE_AGENT_PUB_HPP_
#include <string>
#include <stddef.h>
#include <stdint.h>

namespace cpp_streamer {

class VoiceAgentCallbackI
{
public:
    virtual void OnVoiceAgentRecognizedText(const std::string& room_id,
        const std::string& user_id,
        const std::string& text,
        int64_t ts) = 0;
    virtual void OnVoiceAgentResponseText(const std::string& room_id,
        const std::string& user_id,
        const std::string& text,
        int64_t ts) = 0;
    virtual void OnVoiceAgentConversationStart(const std::string& room_id,
        const std::string& conversation_id,
        int64_t ts) = 0;
    virtual void OnVoiceAgentConversationEnd(const std::string& room_id,
        const std::string& conversation_id,
        int64_t ts) = 0;
    virtual void OnVoiceAgentAiOpusData(const std::vector<uint8_t>& opus_data, 
        int sample_rate, int channels, int64_t pts, int current_index) = 0;
};

} // namespace cpp_streamer
#endif // VOICE_AGENT_PUB_HPP_