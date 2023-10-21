# X2Rtc-Stream

[![][x2rtc-banner]][x2rtc-website]

## Website and Documentation

* [x2rtc.com][x2rtc-website]

## Design Goals

x2rtc is designed to accomplish with the following goals:

* Be a powerful [SFU](https://webrtcglossary.com/sfu/) (Selective Forwarding Unit).
* Support all live stream like: rtmp, srt, flv, rtp, webrtc etc.
* Be simple, low level API for easy use.
* Layered architecture: Network; ProtocolHandler; MediaHandler; MediaDispatch.
* Support WebRtc whep/whip signaling protocol.
* Extremely powerful (Support multi-thread coded in C++ ).
* Super fast ,stable and useful are our ultimate goal!

## How to use?

1、Download the X2RtcStream source code

```
git clone https://github.com/X2Rtc/X2Rtc-Stream.git
```

2、Executing the installation script

```
cd X2Rtc-Stream
sh make.sh all
```

3、Start X2RtcStream service

```
sh x2rtc.sh start
```

4、Other parameters

```
## Multiple cpu installation 
sh make.sh all -j4

## Uninstall X2RtcStream service
sh make.sh all clean

## Stop X2RtcStream service
sh x2rtc.sh stop

## Restart X2RtcStream service
sh x2rtc.sh restart
```

## Dependencies

To use it, you'll need to satisfy the following dependencies, and must to comply with the licensing agreement of each project:

- [abseil-cpp](https://github.com/abseil-cpp)
- [faac](https://github.com/faac)
- [faad](https://github.com/faad)
- [HttpParser](https://github.com/)
- [jsoncpp](https://github.com/)
- [jthread](https://github.com/)
- [mediaserver](https://github.com/)
- [srtp](https://github.com/)
- [libuv](https://github.com/)
- [MediaSoup](https://github.com/)
- [openssl](https://github.com/)
- [opus](https://github.com/)
- [rapidjson](https://github.com/)
- [spdlog](https://github.com/)
- [srt](https://github.com/)
- [usrsctp](https://github.com/)
- [WsSocket](https://github.com/)
- [ZlMediaKit](https://github.com/) 
- [ZlToolKit](https://github.com/)
- [WebRtc](https://github.com/)

## Social

* Twitter: [@X2rtc_cloud](https://twitter.com/X2rtc_cloud)

## License

GUN License - see [LICENSE](https://github.com/X2Rtc/X2Rtc-Stream/blob/master/LICENSE) for full text

[x2rtc-website]: x2rtc
[x2rtc-banner]: /art/x2rtc-banner.png
