# websocket protoo message
## join message
client ---> server

info: client join the meeting room

request:
```
{
    "request": true,
    "id": 6385452,
    "method": "join",
    "data": {
        "roomId": "6qtz8zit",
        "userId": "5860",
        "userName": "User_5860",
        "audit": false
    }
}
```
response:
```
{
    "data": {
        "code": 0,
        "message": "join success",
        "users": [
            {
                "pushers": [
                    {
                        "pusherId": "7a8b7bd4-97c1-cfad-ee11-453ce71e9a69",
                        "rtpParam": {
                            "av_type": "audio",
                            "channel": 2,
                            "clock_rate": 48000,
                            "codec": "opus",
                            "fmtp_param": "minptime=10;useinbandfec=1",
                            "mid_ext_id": 4,
                            "payload_type": 111,
                            "rtcp_features": [
                                "transport-cc"
                            ],
                            "rtx_payload_type": 0,
                            "rtx_ssrc": 0,
                            "ssrc": 3779648384,
                            "tcc_ext_id": 3,
                            "use_nack": false
                        }
                    },
                    {
                        "pusherId": "d85cab69-9564-4c22-0c97-a0fb3d8cab16",
                        "rtpParam": {
                            "av_type": "video",
                            "clock_rate": 90000,
                            "codec": "H264",
                            "fmtp_param": "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f",
                            "key_request": true,
                            "mid_ext_id": 4,
                            "payload_type": 109,
                            "rtcp_features": [
                                "goog-remb",
                                "transport-cc",
                                "ccm fir",
                                "nack",
                                "nack pli"
                            ],
                            "rtx_payload_type": 114,
                            "rtx_ssrc": 3406485371,
                            "ssrc": 1192252089,
                            "tcc_ext_id": 3,
                            "use_nack": true
                        }
                    }
                ],
                "userId": "7760",
                "userName": "User_7760"
            }
        ]
    },
    "id": 6385452,
    "ok": true,
    "response": true
}
```

## push message
client ---> server

info: client push rtc media to server

request:
```
{
    "request": true,
    "id": 1050195,
    "method": "push",
    "data": {
        "sdp": {
            "type": "offer",
            "sdp": "v=0\r\no=- 6595943067423632257 2 IN IP4 127.0.0.1....\r\n"
        },
        "roomId": "6qtz8zit",
        "userId": "7760"
    }
}
```
response:
```
{
    "data": {
        "code": 0,
        "message": "push success",
        "sdp": "v=0\r\no=- 6595943067423632257 2 IN IP4 127.0.0.1....\r\n"
    },
    "id": 1050195,
    "ok": true,
    "response": true
}
```

## pull message

client--->message

info: client request pulling rtc streamm from server

request:
```
{
    "request": true,
    "id": 7448881,
    "method": "pull",
    "data": {
        "sdp": {
            "type": "offer",
            "sdp": "v=0\r\no=- 3218343350439408859...."
        },
        "roomId": "6qtz8zit",
        "userId": "5860",
        "targetUserId": "7760",
        "specs": [
            {
                "type": "audio",
                "pusher_id": "7a8b7bd4-97c1-cfad-ee11-453ce71e9a69"
            },
            {
                "type": "video",
                "pusher_id": "d85cab69-9564-4c22-0c97-a0fb3d8cab16"
            }
        ]
    }
}
```
response:
```
{
    "data": {
        "code": 0,
        "message": "pull success",
        "sdp": "v=0\r\no=- 3218343350439408859..."
    },
    "id": 7448881,
    "ok": true,
    "response": true
}
```

## userLeft
server ---> client

info: when user leave or user websocket closed.

notification:
```
{
    "data": [
        "4253"
    ],
    "method": "userLeft",
    "notification": true
}
```

## newUser
server ---> client

info: new user joined

notification:
```
{
    "data": [
        {
            "pushers": [

            ],
            "userId": "5860",
            "userName": "User_5860"
        }
    ],
    "method": "newUser",
    "notification": true
}
```

## newPusher
server ---> client

info: new pusher 

notification:
```
{
    "data": {
        "pushers": [
            {
                "pusherId": "80157a6b-7d69-6a19-b054-aeb39c8c574d",
                "rtpParam": {
                    "av_type": "video",
                    "clock_rate": 90000,
                    "codec": "H264",
                    "fmtp_param": "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f",
                    "key_request": true,
                    "mid_ext_id": 4,
                    "payload_type": 109,
                    "rtcp_features": [
                        "goog-remb",
                        "transport-cc",
                        "ccm fir",
                        "nack",
                        "nack pli"
                    ],
                    "rtx_payload_type": 114,
                    "rtx_ssrc": 1477625286,
                    "ssrc": 3460829504,
                    "tcc_ext_id": 3,
                    "use_nack": true
                }
            },
            {
                "pusherId": "8dd1431b-5e6d-00a0-1640-9651e24dc90c",
                "rtpParam": {
                    "av_type": "audio",
                    "channel": 2,
                    "clock_rate": 48000,
                    "codec": "opus",
                    "fmtp_param": "minptime=10;useinbandfec=1",
                    "mid_ext_id": 4,
                    "payload_type": 111,
                    "rtcp_features": [
                        "transport-cc"
                    ],
                    "rtx_payload_type": 0,
                    "rtx_ssrc": 0,
                    "ssrc": 2535593392,
                    "tcc_ext_id": 3,
                    "use_nack": false
                }
            }
        ],
        "roomId": "6qtz8zit",
        "userId": "5860",
        "userName": "User_5860"
    },
    "method": "newPusher",
    "notification": true
}
```