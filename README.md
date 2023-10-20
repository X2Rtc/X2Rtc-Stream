# X2Rtc-Stream

## Website and Documentation

* [www.x2rtc.com][x2rtc-website]

## Design Goals

x2rtc is designed to accomplish with the following goals:

* Be a powerful [SFU](https://webrtcglossary.com/sfu/) (Selective Forwarding Unit).
* Support all live stream like: rtmp, srt, flv, rtp, webrtc etc.
* Be simple, low level API for easy use.

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
## Uninstall X2RtcStream service
sh make.sh all clean

## Stop X2RtcStream service
sh x2rtc.sh stop

## Restart X2RtcStream service
sh x2rtc.sh restart
```

