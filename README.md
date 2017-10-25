[![Build Status](https://travis-ci.org/Comcast/aker.svg?branch=master)](https://travis-ci.org/Comcast/aker)
[![codecov.io](http://codecov.io/github/Comcast/aker/coverage.svg?branch=master)](http://codecov.io/github/Comcast/aker?branch=master)
[![Coverity](https://img.shields.io/coverity/scan/14083.svg)](https://scan.coverity.com/projects/comcast-aker)
[![Apache V2 License](http://img.shields.io/badge/license-Apache%20V2-blue.svg)](https://github.com/Comcast/aker/blob/master/LICENSE)

# aker

Aker is an experimental MAC address blocking scheduler.

For details:

https://github.com/Comcast/aker/wiki


# Building and Testing Instructions

```
mkdir build
cd build
cmake ..
make
make test
```

# Running the application
1. Ensure parodus is running first successfully.
2. Start the application after building, like so - ```./src/aker -p <parodus local URL> -c <URL to receive parodus response> -w firewall -d pcs.bin -m "log.txt"``` 
3. From a separate terminal, send the following curl command - ```curl -i -d '{0x83, 0xaf, 'w', 'e', 'e', 'k', 'l', 'y', '-', 's', 'c', 'h', 'e', 'd', 'u', 'l', 'e', 0x93, 0x82, 0xa4, 't', 'i', 'm', 'e', 0x0a, 0xa7, 'i', 'n', 'd', 'e', 'x', 'e', 's', 0x93, 0x00, 0x01, 0x03, 0x82, 0xa4, 't', 'i', 'm', 'e', 0x14, 0xa7, 'i', 'n', 'd', 'e', 'x', 'e', 's', 0x91, 0x00, 0x82, 0xa4, 't', 'i', 'm', 'e', 0x14, 0xa7, 'i', 'n', 'd', 'e', 'x', 'e', 's', 0x90, 0xa4, 'm', 'a', 'c', 's', 0x94, 0xb1, '1', '1', ':', '2', '2', ':', '3', '3', ':', '4', '4', ':', '5', '5', ':', 'a', 'a', 0xb1, '2', '2', ':', '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6', ':', 'b', 'b', 0xb1, '3', '3', ':', '4', '4', ':', '5', '5', ':', '6', '6', ':', '7', '7', ':', 'c', 'c', 0xb1, '4', '4', ':', '5', '5', ':', '6', '6', ':', '7', '7', ':', '8', '8', ':', 'd', 'd', 0xb1, 'a', 'b', 's', 'o', 'l', 'u', 't', 'e', '-', 's', 'c', 'h', 'e', 'd', 'u', 'l', 'e', 0x91, 0x82, 0xa9, 'u', 'n', 'i', 'x', '-', 't', 'i', 'm', 'e', 0xce, 0x59, 0xe5, 0x83, 0x17, 0xa7, 'i', 'n', 'd', 'e', 'x', 'e', 's', 0x92, 0x00, 0x02}' -X POST https://api.webpa.comcast.net:8090/api/v2/device/mac:<mac address provided to parodus>/iot -H "Authorization: Bearer <valid token>"```


# Expected behavior
1. The Parodus debug should be something similar to the following:-
```
[1508966349][PARODUS][Info]: Received msg from server
[1508966349][WRP-C][Debug]: unpacking encoded data
[1508966349][WRP-C][Debug]: unpack_ret:2
[1508966349][WRP-C][Debug]: Binary payload {0x83, 0xaf, w, e, e, k, l, y, -, s, c, h, e, d, u, l, e, 0x93, 0x82, 0xa4, t, i, m, e, 0x0a, 0xa7, i, n, d, e, x, e, s, 0x93, 0x00, 0x01, 0x03, 0x82, 0xa4, t, i, m, e, 0x14, 0xa7, i, n, d, e, x, e, s, 0x91, 0x00, 0x82, 0xa4, t, i, m, e, 0x14, 0xa7, i, n, d, e, x, e, s, 0x90, 0xa4, m, a, c, s, 0x94, 0xb1, 1, 1, :, 2, 2, :, 3, 3, :, 4, 4, :, 5, 5, :, a, a, 0xb1, 2, 2, :, 3, 3, :, 4, 4, :, 5, 5, :, 6, 6, :, b, b, 0xb1, 3, 3, :, 4, 4, :, 5, 5, :, 6, 6, :, 7, 7, :, c, c, 0xb1, 4, 4, :, 5, 5, :, 6, 6, :, 7, 7, :, 8, 8, :, d, d, 0xb1, a, b, s, o, l, u, t, e, -, s, c, h, e, d, u, l, e, 0x91, 0x82, 0xa9, u, n, i, x, -, t, i, m, e, 0xce, 0x59, 0xe5, 0x83, 0x17, 0xa7, i, n, d, e, x, e, s, 0x92, 0x00, 0x02}
[1508966349][WRP-C][Debug]: spans is nil
[1508966349][PARODUS][Debug]: 
Decoded recivedMsg of size:920
[1508966349][PARODUS][Info]: msgType received:3
[1508966349][PARODUS][Debug]: numOfClients registered is 1
[1508966349][PARODUS][Debug]: ********* validate_partner_id ********
[1508966349][PARODUS][Debug]: partner_id is not available to validate
[1508966349][PARODUS][Info]: Received downstream dest as :iot and transaction_uuid :23b5bd16-b9ca-11e7-9aa2-fa163e195609
[1508966349][PARODUS][Debug]: node is pointing to temp->service_name iot 
[1508966349][PARODUS][Debug]: sending to nanomsg client iot
[1508966349][PARODUS][Info]: sent downstream message to reg_client 'tcp://172.20.5.2:6668'
[1508966349][PARODUS][Debug]: downstream bytes sent:920
[1508966349][PARODUS][Debug]: free for downstream decoded msg
[1508966349][PARODUS][Debug]: Mutex destroyed 
[1508966349][PARODUS][Debug]: mutex lock in consumer thread
[1508966349][PARODUS][Debug]: Before pthread cond wait in consumer thread
[1508966349][PARODUS][Info]: Upstream message received from nanomsg client
[1508966349][PARODUS][Debug]: Producer added message
[1508966349][PARODUS][Debug]: mutex unlock in producer thread
[1508966349][PARODUS][Info]: nanomsg server gone into the listening mode...
[1508966349][PARODUS][Debug]: mutex unlock in consumer thread after cond wait
[1508966349][PARODUS][Debug]: mutex lock in consumer thread
[1508966349][PARODUS][Debug]: mutex unlock in consumer thread
[1508966349][PARODUS][Debug]: ---- Decoding Upstream Msg ----
[1508966349][WRP-C][Debug]: unpacking encoded data
[1508966349][WRP-C][Debug]: unpack_ret:2
[1508966349][WRP-C][Debug]: Binary payload 
[1508966349][PARODUS][Info]:  Received upstream data with MsgType: 3 dest: 'dns:scytale.webpa.comcast.net/iot' transaction_uuid: 23b5bd16-b9ca-11e7-9aa2-fa163e195609
[1508966349][PARODUS][Debug]: Appending received msg with metadata
[1508966349][WRP-C][Debug]: First byte in hex : 86
[1508966349][WRP-C][Debug]: Map size is :6
[1508966349][WRP-C][Debug]: New Map size : 7
[1508966349][PARODUS][Debug]: encodedSize after appending :460
[1508966349][PARODUS][Debug]: metadata appended upstream msg msg_typesourcemac:123456789012/iotdest!dns:scytale.webpa.comcast.net/iottransaction_uuid$23b5bd16-b9ca-11e7-9aa2-fa163e195609include_spanspayload
[1508966349][PARODUS][Info]: Sending metadata appended upstream msg to server
[1508966349][PARODUS][Info]: sendMessage length 460
[1508966349][PARODUS][Debug]: Number of bytes written: 460
[1508966349][PARODUS][Debug]: Free for upstream decoded msg
[1508966349][PARODUS][Debug]: mutex lock in consumer thread
```

2. The debug output of the aker should be similar to the following:-
```
[1508966343][WRP-C][Debug]: ***   Start of Msgpack Encoding  ***
[1508966343][Aker][Info]: Init for parodus Success..!!
[1508966343][Aker][Debug]: starting the main loop...
libparodus_receive - instance = 0x63c0b0
libparodus_receive - inst = 0x63c0b0
[1508966343][WRP-C][Debug]: unpacking encoded data
[1508966343][WRP-C][Debug]: unpack_ret:2
[1508966345][Aker][Info]: Timed out or message closed.
libparodus_receive - instance = 0x63c0b0
libparodus_receive - inst = 0x63c0b0
[1508966347][Aker][Info]: Timed out or message closed.
libparodus_receive - instance = 0x63c0b0
libparodus_receive - inst = 0x63c0b0
[1508966349][WRP-C][Debug]: unpacking encoded data
[1508966349][WRP-C][Debug]: unpack_ret:2
[1508966349][WRP-C][Debug]: Binary payload {0x83, 0xaf, w, e, e, k, l, y, -, s, c, h, e, d, u, l, e, 0x93, 0x82, 0xa4, t, i, m, e, 0x0a, 0xa7, i, n, d, e, x, e, s, 0x93, 0x00, 0x01, 0x03, 0x82, 0xa4, t, i, m, e, 0x14, 0xa7, i, n, d, e, x, e, s, 0x91, 0x00, 0x82, 0xa4, t, i, m, e, 0x14, 0xa7, i, n, d, e, x, e, s, 0x90, 0xa4, m, a, c, s, 0x94, 0xb1, 1, 1, :, 2, 2, :, 3, 3, :, 4, 4, :, 5, 5, :, a, a, 0xb1, 2, 2, :, 3, 3, :, 4, 4, :, 5, 5, :, 6, 6, :, b, b, 0xb1, 3, 3, :, 4, 4, :, 5, 5, :, 6, 6, :, 7, 7, :, c, c, 0xb1, 4, 4, :, 5, 5, :, 6, 6, :, 7, 7, :, 8, 8, :, d, d, 0xb1, a, b, s, o, l, u, t, e, -, s, c, h, e, d, u, l, e, 0x91, 0x82, 0xa9, u, n, i, x, -, t, i, m, e, 0xce, 0x59, 0xe5, 0x83, 0x17, 0xa7, i, n, d, e, x, e, s, 0x92, 0x00, 0x02}
[1508966349][WRP-C][Debug]: spans is nil
[1508966349][Aker][Info]: Got something from parodus.
[1508966349][Aker][Debug]: req->dest = mac:123456789012/iot
[1508966349][Aker][Debug]: process_request_set -  file_handle = 0x6408d0
[1508966349][Aker][Debug]: process_request_set - write_size = 705
[1508966349][Aker][Debug]: Sending response to libparodus.
libparodus_send - instance = 0x63c0b0
libparodus_send - inst = 0x63c0b0
[1508966349][WRP-C][Debug]: ***   Start of Msgpack Encoding  ***
[1508966349][Aker][Debug]: arv = 0 (Success). Cleaning up WRP response next.
[1508966349][Aker][Debug]: [1508966349][Aker][Debug]: After cleaning up WRP response.
libparodus_receive - instance = 0x63c0b0
libparodus_receive - inst = 0x63c0b0
[1508966351][Aker][Info]: Timed out or message closed.
libparodus_receive - instance = 0x63c0b0
```

3. The response to curl should be something similar to the following:-
```
HTTP/1.1 200 OK
Content-Type: application/json
Date: Wed, 25 Oct 2017 21:19:09 GMT
X-Webpa-Device-Id: mac:123456789012
X-Webpa-Transaction-Id: 23b5bd16-b9ca-11e7-9aa2-fa163e195609
Content-Length: 0
```
