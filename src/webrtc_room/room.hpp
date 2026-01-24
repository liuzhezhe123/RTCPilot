#ifndef ROOM_HPP
#define ROOM_HPP
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "utils/timeex.hpp"
#include "net/http/http_common.hpp"
#include "webrtc_session.hpp"
#include "udp_transport.hpp"
#include "rtc_info.hpp"
#include <map>
#include <memory>
#include <vector>
#include <uv.h>

namespace cpp_streamer {

class RtcUser;
class WebRtcServer;
class ProtooResponseI;
class RtcRecvRelay;
class RtcSendRelay;

class Room : public TimerInterface, 
    public PacketFromRtcPusherCallbackI, 
    public MediaPushPullEventI,
    public AsyncRequestCallbackI
{
public:
    Room(const std::string& room_id, 
        PilotClientI* pilot_client,
        uv_loop_t* loop, 
        Logger* logger);
    virtual ~Room();

public:
    RTC_USER_TYPE GetUserType(const std::string& user_id);
    std::string GetRoomId() { return room_id_;}
    void Close();
    int UserJoin(const std::string& user_id, 
        const std::string& user_name,
        bool audience,
        int id,
        ProtooResponseI* resp_cb);
    int WhipUserJoin(const std::string& user_id, 
        const std::string& user_name);
	int UserLeave(const std::string& user_id);
    int DisconnectUser(const std::string& user_id);

    int HandlePushSdp(const std::string& user_id, 
        const std::string& sdp_type, 
        const std::string& sdp_str, 
        int id,
        ProtooResponseI* resp_cb);
    int HandleWhipPushSdp(const std::string& user_id, 
        const std::string& sdp_type, 
        const std::string& sdp_str, 
        std::string& anwser_sdp);
    int HandlePullSdp(const PullRequestInfo& pull_info, 
        const std::string& sdp_type, 
        const std::string& sdp_str, 
        int id,
        ProtooResponseI* resp_cb);
    int HandleRemotePullSdp(const std::string& pusher_user_id,
        const PullRequestInfo& pull_info, 
        const std::string& sdp_type, 
        const std::string& sdp_str, 
        int id,
        ProtooResponseI* resp_cb);
    int HandleWsHeartbeat(const std::string& user_id);
    bool IsAlive();
    
public:
    void NotifyTextMessage2LocalUsers(const std::string& from_user_id, const std::string& from_user_name, const std::string& message);
    void NotifyTextMessage2PilotCenter(const std::string& from_user_id, const std::string& from_user_name, const std::string& message);
    
public:
    void HandleNewUserNotificationFromCenter(json& data_json);
    void HandleNewPusherNotificationFromCenter(json& data_json);
    void HandlePullRemoteStreamNotificationFromCenter(json& data_json);
    void HandleUserDisconnectNotificationFromCenter(json& data_json);
    void HandleUserLeaveNotificationFromCenter(json& data_json);
    void HandleNotifyTextMessageFromCenter(json& data_json);

public://implement PacketFromRtcPusherCallbackI
    virtual void OnRtpPacketFromRtcPusher(const std::string& user_id, const std::string& session_id,
        const std::string& pusher_id, RtpPacket* rtp_packet) override;
    virtual void OnRtpPacketFromRemoteRtcPusher(const std::string& pusher_user_id,
        const std::string& pusher_id, 
        RtpPacket* rtp_packet) override;
public:
    virtual void OnPushClose(const std::string& pusher_id) override;
    virtual void OnPullClose(const std::string& puller_id) override;
    virtual void OnKeyFrameRequest(const std::string& pusher_id, 
        const std::string& puller_user_id, 
        const std::string& pusher_user_id,
        uint32_t ssrc) override;

public://implement AsyncRequestCallbackI
    virtual void OnAsyncRequestResponse(int id, const std::string& method, json& resp_json) override;
    
protected:
    virtual bool OnTimer() override;

private:
    int UpdateRtcSdpByPullers(std::vector<std::shared_ptr<MediaPuller>>& media_pullers, std::shared_ptr<RtcSdp> answer_sdp);
    void NotifyNewUser(const std::string& user_id, const std::string& user_name);
    void NotifyNewPusher(const std::string& pusher_user_id, 
        const std::string& pusher_user_name,
        const std::vector<PushInfo>& push_infos);
    int PullRemotePusher(const std::string& pusher_user_id, const PushInfo& push_info);
    int SendPullRequestToPilotCenter(const std::string& pusher_user_id, 
        const PushInfo& push_info,
        std::shared_ptr<RtcRecvRelay> relay_ptr);
    
private:
    int ReConnect(std::shared_ptr<RtcUser> new_user, int id, ProtooResponseI* resp_cb);

private:
    void Join2PilotCenter(std::shared_ptr<RtcUser> user_ptr);
    void JoinResponseFromPilotCenter(const std::string& method, json& data_json);
    void NewPusher2PilotCenter(const std::string& pusher_user_id, const std::vector<PushInfo>& push_infos);
    void LeaveFromPilotCenter(std::shared_ptr<RtcUser> user_ptr);
    void UserDisconnect2PilotCenter(const std::string& user_id);
    void UserLeave2PilotCenter(const std::string& user_id);

private:
    std::shared_ptr<RtcRecvRelay> CreateOrGetRecvRtcRelay(const std::string& pusher_user_id, const PushInfo& push_info);
    void ReleaseUserResources(const std::string& user_id);

private:
    std::string room_id_;
    PilotClientI* pilot_client_ = nullptr;
	uv_loop_t* loop_ = nullptr;
    Logger* logger_ = nullptr;
    int64_t last_alive_ms_ = -1;

private:
    bool closed_ = false;
    std::map<std::string, std::shared_ptr<RtcUser>> users_;
    std::map<std::string, std::shared_ptr<MediaPusher>> pusherId2pusher_;
    std::map<std::string, std::map<std::string, std::shared_ptr<MediaPuller>>> pusher2pullers_;// pusher_id -> (puller_id -> MediaPuller)
    // pusher_id -> RtcRecvRelay
    std::map<std::string, std::shared_ptr<RtcRecvRelay>> pusherId2recvRelay_;
    // user_id -> RtcRecvRelay
    std::map<std::string, std::shared_ptr<RtcRecvRelay>> pusher_user_id2recvRelay_;
    // pusher_user_id -> RtcSendRelay
    std::map<std::string, std::shared_ptr<RtcSendRelay>> pusher_user_id2sendRelay_;
};

} // namespace cpp_streamer

#endif // ROOM_HPP