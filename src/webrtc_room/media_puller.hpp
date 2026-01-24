#ifndef MEDIA_PULLER_HPP
#define MEDIA_PULLER_HPP
#include "utils/logger.hpp"
#include "utils/av/av.hpp"
#include "net/rtprtcp/rtp_packet.hpp"
#include "net/rtprtcp/rtprtcp_pub.hpp"
#include "net/rtprtcp/rtcp_pspli.hpp"
#include "net/rtprtcp/rtcp_rr.hpp"
#include "udp_transport.hpp"
#include "rtp_send_session.hpp"
#include "rtc_info.hpp"
#include <memory>
#include <string>

namespace cpp_streamer {

class MediaPuller
{
public:
    MediaPuller(const RtpSessionParam& param, 
        const std::string& room_id, 
        const std::string& puller_user_id, 
        const std::string& pusher_user_id,
        const std::string& pusher_id,
        const std::string& session_id,
        TransportSendCallbackI* cb,
        uv_loop_t* loop,
        Logger* logger);
    virtual ~MediaPuller();

public:
    std::string GetPullerId() { return puller_id_; }
    std::string GetPusherId() { return pusher_id_; }
    std::string GetPulllerUserId() { return puller_user_id_; }
    std::string GetPusherUserId() { return pusher_user_id_; }
    MEDIA_PKT_TYPE GetMediaType() { return param_.av_type_; }
    RtpSessionParam& GetRtpSessionParam() { return param_; }

public:
    void CreateRtpSendSession();
    int HandleRtcpRrBlock(RtcpRrBlockInfo& rr_block);
    int HandleRtcpFbNack(RtcpFbNack* nack_pkt);

public:
    void OnTransportSendRtp(RtpPacket* rtp_pkt);

public:
    void OnTimer(int64_t now_ms);

private:
    RtpSessionParam param_;
    uv_loop_t* loop_ = nullptr;
    Logger* logger_ = nullptr;
    std::string room_id_;
    std::string puller_user_id_;
    std::string pusher_user_id_;
    std::string session_id_;
    std::string puller_id_;
    std::string pusher_id_;
    TransportSendCallbackI* cb_ = nullptr;

private:
    std::unique_ptr<RtpSendSession> rtp_send_session_ = nullptr;

private:
    int64_t last_statics_ms_ = -1;
};

} // namespace cpp_streamer

#endif // MEDIA_PULLER_HPP