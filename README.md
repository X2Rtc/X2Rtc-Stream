# X2Rtc-Stream

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

