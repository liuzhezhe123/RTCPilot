#ifndef ROOM_MGR_HPP
#define ROOM_MGR_HPP
#include "ws_message/ws_protoo_info.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "utils/timeex.hpp"
#include "rtc_info.hpp"

#include <memory>
#include <map>
#include <uv.h>

namespace cpp_streamer {

class Room;
class WebRtcServer;
class RoomMgr : public TimerInterface, 
                public ProtooCallBackI, 
                public AsyncRequestCallbackI,
                public AsyncNotificationCallbackI
{
    virtual ~RoomMgr();

private:
    RoomMgr(uv_loop_t* loop, Logger* logger);

public:
    static RoomMgr& Instance(uv_loop_t* loop, Logger* logger) {
        static RoomMgr instance(loop, logger);
        return instance;
    }

public:
    void SetPilotClient(PilotClientI* pilot_client) {
        pilot_client_ = pilot_client;
    }
public:
    virtual void OnProtooRequest(const int id, const std::string& method, nlohmann::json& j, ProtooResponseI* resp_cb) override;
    virtual void OnProtooNotification(const std::string& method, nlohmann::json& j) override;
    virtual void OnProtooResponse(const int id, int code, const std::string& err_msg, nlohmann::json& j) override;
	virtual void OnWsSessionClose(const std::string& room_id, const std::string& user_id) override;

public://implement AsyncRequestCallbackI
    virtual void OnAsyncRequestResponse(int id, const std::string& method, nlohmann::json& resp_json) override;

public://implement AsyncNotificationCallbackI
    virtual void OnAsyncNotification(const std::string& method, nlohmann::json& data_json) override;
    
public:
    virtual bool OnTimer() override;

public:
    std::shared_ptr<Room> GetOrCreateRoom(const std::string& room_id);
    std::shared_ptr<Room> GetRoom(const std::string& room_id);
    void RemoveRoom(const std::string& room_id);
    
private:
    int HandleJoinRequest(int id, nlohmann::json& j, ProtooResponseI* resp_cb);
    int HandlePushRequest(int id, nlohmann::json& j, ProtooResponseI* resp_cb);
    int HandlePullRequest(int id, nlohmann::json& j, ProtooResponseI* resp_cb);
    int HandleHeartbeatRequest(int id, nlohmann::json& j, ProtooResponseI* resp_cb);

private:
    int HandleTextMessageNotification(nlohmann::json& data_json);
    
private:
	uv_loop_t* loop_ = nullptr;
    Logger* logger_ = nullptr;

private:
    std::map<std::string, std::shared_ptr<Room>> rooms_;

private:
    PilotClientI* pilot_client_ = nullptr;
    int pilot_heartbeat_index_ = 1;
    int64_t last_pilot_heartbeat_ts_ = 0;

private:
    std::map<int, int64_t> id2ts_;
};

} // namespace cpp_streamer

#endif