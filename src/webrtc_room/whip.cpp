#include "whip.hpp"
#include "room_mgr.hpp"
#include "room.hpp"
#include "webrtc_server.hpp"
#include "utils/logger.hpp"
#include "utils/json.hpp"

using json = nlohmann::json;

namespace cpp_streamer
{
bool Whip::ssl_enable_ = false;

static void GetHandle(const HttpRequest* request, std::shared_ptr<HttpResponse> response_ptr) {
    Logger* logger = request->GetLogger();

    LogInfof(logger, "get request received");

    response_ptr->SetStatusCode(200);
    response_ptr->SetStatus("OK");
    response_ptr->AddHeader("Content-Type", "application/json");
    json resp_json = json::object();
    resp_json["message"] = "get request received";
    std::string resp_str = resp_json.dump();
    response_ptr->Write(resp_str.c_str(), resp_str.length());
    return;
}
static void EchoHandle(const HttpRequest* request, std::shared_ptr<HttpResponse> response_ptr) {
    Logger* logger = request->GetLogger();

    std::string body_str = std::string(request->content_body_, request->content_length_);
    LogInfof(logger, "echo post:%s", body_str.c_str());

    response_ptr->SetStatusCode(200);
    response_ptr->SetStatus("OK");
    response_ptr->AddHeader("Content-Type", "application/json");
    response_ptr->Write(body_str.c_str(), body_str.length());
    return;
}

static void WhipDeleteHandle(const HttpRequest* request, std::shared_ptr<HttpResponse> response_ptr) {
    Logger* logger = request->GetLogger();

    LogInfof(logger, "whip delete request received");
    

    auto room_id_it = request->params.find("roomid");
    if (room_id_it == request->params.end()) {
        response_ptr->SetStatusCode(400);
        response_ptr->SetStatus("Bad Request");
        response_ptr->AddHeader("Content-Type", "application/json");
        json error_json;
        error_json["message"] = "missing roomid param";
        error_json["code"] = 400;
        std::string resp_str = error_json.dump();

        LogErrorf(logger, "WhipHandle missing roomid param");
        response_ptr->Write(resp_str.c_str(), resp_str.length());
        return;
    }
    std::string room_id = room_id_it->second;

    auto user_id_it = request->params.find("userid");
    if (user_id_it == request->params.end()) {
        response_ptr->SetStatusCode(400);
        response_ptr->SetStatus("Bad Request");
        response_ptr->AddHeader("Content-Type", "application/json");
        json error_json;
        error_json["message"] = "missing userid param";
        error_json["code"] = 400;
        std::string resp_str = error_json.dump();

        LogErrorf(logger, "WhipHandle missing userid param");
        response_ptr->Write(resp_str.c_str(), resp_str.length());
        return;
    }
    std::string user_id = user_id_it->second;

    auto room_ptr = RoomMgr::Instance(request->GetLoop(), logger).GetRoom(room_id);
    if (!room_ptr) {
        response_ptr->SetStatusCode(404);
        response_ptr->SetStatus("Not Found");
        response_ptr->AddHeader("Content-Type", "application/json");
        json error_json;
        error_json["message"] = "room not found";
        error_json["code"] = 404;
        std::string resp_str = error_json.dump();

        LogErrorf(logger, "WhipDeleteHandle room not found, room_id:%s", room_id.c_str());
        response_ptr->Write(resp_str.c_str(), resp_str.length());
        return;
    }

    try {
        WebRtcServer::RemoveSessionByRoomId(room_id);
        RoomMgr::Instance(request->GetLoop(), logger).RemoveRoom(room_id);
    } catch(const std::exception& e) {
        LogErrorf(logger, "WhipDeleteHandle RemoveRoom exception, room_id:%s, error:%s",
            room_id.c_str(), e.what());
        response_ptr->SetStatusCode(500);
        response_ptr->SetStatus("Internal Server Error");
        response_ptr->AddHeader("Content-Type", "application/json");
        json error_json;
        error_json["message"] = "remove room failed";
        error_json["code"] = 500;
        std::string resp_str = error_json.dump();
        response_ptr->Write(resp_str.c_str(), resp_str.length());
        return;
    }

    response_ptr->SetStatusCode(200);
    response_ptr->SetStatus("OK");
    response_ptr->AddHeader("Content-Type", "application/json");
    json resp_json = json::object();
    resp_json["message"] = "whip delete request received";
    resp_json["code"] = 0;
    std::string resp_str = resp_json.dump();
    response_ptr->Write(resp_str.c_str(), resp_str.length());
    return;
}

static void WhipHandle(const HttpRequest* request, std::shared_ptr<HttpResponse> response_ptr) {
    Logger* logger = request->GetLogger();

    std::string body_str = std::string(request->content_body_, request->content_length_);

	std::string headers_str;
    for (auto header : request->headers_) {
		headers_str += header.first + ": " + header.second + "\r\n";
	}
	LogInfof(logger, "WhipHandle headers:\r\n%s", headers_str.c_str());
	std::string param_str;
    for (auto param : request->params) {
        param_str += param.first + "=" + param.second + "&";
    }
	LogInfof(logger, "WhipHandle params:%s", param_str.c_str());

    auto room_id_it = request->params.find("roomid");
    if (room_id_it == request->params.end()) {
        response_ptr->SetStatusCode(400);
        response_ptr->SetStatus("Bad Request");
        response_ptr->AddHeader("Content-Type", "application/json");
        json error_json;
        error_json["message"] = "missing roomid param";
        error_json["code"] = 400;
        std::string resp_str = error_json.dump();

        LogErrorf(logger, "WhipHandle missing roomid param");
        response_ptr->Write(resp_str.c_str(), resp_str.length());
        return;
    }
    std::string room_id = room_id_it->second;

    auto user_id_it = request->params.find("userid");
    if (user_id_it == request->params.end()) {
        response_ptr->SetStatusCode(400);
        response_ptr->SetStatus("Bad Request");
        response_ptr->AddHeader("Content-Type", "application/json");
        json error_json;
        error_json["message"] = "missing userid param";
        error_json["code"] = 400;
        std::string resp_str = error_json.dump();

        LogErrorf(logger, "WhipHandle missing userid param");
        response_ptr->Write(resp_str.c_str(), resp_str.length());
        return;
    }
    std::string user_id = user_id_it->second;

    auto room_ptr = RoomMgr::Instance(request->GetLoop(), logger).GetOrCreateRoom(room_id);
    LogInfof(logger, "whip post:%s", body_str.c_str());

    int ret = room_ptr->WhipUserJoin(user_id, user_id);
    if (ret != 0) {
        response_ptr->SetStatusCode(500);
        response_ptr->SetStatus("Internal Server Error");
        response_ptr->AddHeader("Content-Type", "application/json");
        json error_json;
        error_json["message"] = "whip user join failed";
        error_json["code"] = 500;
        std::string resp_str = error_json.dump();

        response_ptr->Write(resp_str.c_str(), resp_str.length());
        return;
    }
    std::string answer_sdp_str;
    ret = room_ptr->HandleWhipPushSdp(user_id, "offer", body_str, answer_sdp_str);
    if (ret != 0) {
        response_ptr->SetStatusCode(500);
        response_ptr->SetStatus("Internal Server Error");
        response_ptr->AddHeader("Content-Type", "application/json");
        json error_json;
        error_json["message"] = "whip push sdp failed";
        error_json["code"] = 500;
        std::string resp_str = error_json.dump();

        response_ptr->Write(resp_str.c_str(), resp_str.length());
        return;
    }
    std::string host_str;
    auto host_it = request->headers_.find("Host");
    if (host_it != request->headers_.end()) {
        host_str = host_it->second;
    } else {
        host_str = "localhost";
    }
    std::string location_str;
    
    if (Whip::ssl_enable_) {
        location_str = "https://" + host_str + "/whip?roomid=" + room_id + "&userid=" + user_id;
    } else {
        location_str = "http://" + host_str + "/whip?roomid=" + room_id + "&userid=" + user_id;
    }
    response_ptr->AddHeader("Location", location_str);

    response_ptr->SetStatusCode(201);
    response_ptr->SetStatus("Created");
    response_ptr->AddHeader("Content-Type", "application/sdp");
    response_ptr->Write(answer_sdp_str.c_str(), answer_sdp_str.length());

    auto resp_headers = response_ptr->Headers();
    std::string resp_headers_str;
    for (auto header : resp_headers) {
        resp_headers_str += header.first + ": " + header.second + "\r\n";
    }
    LogInfof(logger, "WhipHandle response headers:\r\n%s", resp_headers_str.c_str());
    LogInfof(logger, "WhipHandle response user_id:%s, room_id:%s, answer_sdp:\r\n%s",
		user_id.c_str(), room_id.c_str(), answer_sdp_str.c_str());
    return;
}

Whip::Whip(uv_loop_t* loop, const std::string& ip, uint16_t port, Logger* logger) {
    loop_ = loop;
    logger_ = logger;
    http_server_.reset(new HttpServer(loop, ip, port, logger));
    http_server_->AddPostHandle("/whip", WhipHandle);
    http_server_->AddPostHandle("/echo", EchoHandle);
    http_server_->AddPostHandle("/get", GetHandle);
    http_server_->AddDeleteHandle("/whip", WhipDeleteHandle);

    Whip::ssl_enable_ = false;
    LogInfof(logger_, "Whip http server started at %s:%u", ip.c_str(), port);
}

Whip::Whip(uv_loop_t* loop, const std::string& ip, uint16_t port, const std::string& key_file, const std::string& cert_file, Logger* logger) {
    loop_ = loop;
    logger_ = logger;
    http_server_.reset(new HttpServer(loop,  ip, port, key_file, cert_file, logger));
    http_server_->AddPostHandle("/whip", WhipHandle);
    http_server_->AddPostHandle("/echo", EchoHandle);
    http_server_->AddGetHandle("/get", GetHandle);
    http_server_->AddDeleteHandle("/whip", WhipDeleteHandle);

    Whip::ssl_enable_ = true;
    LogInfof(logger_, "Whip https server started at %s:%u", ip.c_str(), port);
}

Whip::~Whip() {
    http_server_.reset();
}
}