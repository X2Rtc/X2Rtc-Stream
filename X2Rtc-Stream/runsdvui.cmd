cd /d "C:\Work\X2Rtc-Project\X2Rtc-Stream" &msbuild "X2Mod-Rtmp.vcxproj" /t:sdvViewer /p:configuration="Debug" /p:platform="Win32" /p:SolutionDir="C:\Work\X2Rtc-Project" 
exit %errorlevel% 