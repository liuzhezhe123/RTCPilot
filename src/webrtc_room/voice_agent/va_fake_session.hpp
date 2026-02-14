#ifndef VA_FAKE_SESSION_HPP_
#define VA_FAKE_SESSION_HPP_
#include "webrtc_room/rtc_info.hpp"
#include "webrtc_room/udp_transport.hpp"

#include "voice_agent_pub.hpp"
#include "utils/logger.hpp"

namespace cpp_streamer
{

class VaFakeSession : public TransportSendCallbackI
{
public:
    VaFakeSession(const std::string& room_id, const std::string& user_id, Logger* logger);
    virtual ~VaFakeSession();

public:
    virtual bool IsConnected() override;
    virtual void OnTransportSendRtp(uint8_t* data, size_t sent_size) override;
    virtual void OnTransportSendRtcp(uint8_t* data, size_t sent_size) override;

public:
    std::string GetSessionId() { return id_; }

private:
    std::string id_;
    std::string room_id_;
    std::string user_id_;
    Logger* logger_ = nullptr;
};


} // namespace cpp_streamer

#endif //VA_FAKE_SESSION_HPP_