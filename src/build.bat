@echo off

IF NOT EXIST ..\bin mkdir ..\bin
pushd ..\bin


cl -wd4577 -O2 -Oi /DDEVELOPER_MODE=1 /DDESKTOP=1 /DNOMINMAX /Fe../bin/my-anki-deck.exe /I../engine /I ../SDL2 /I../libs /I../libs/gl3w /IE:/include /Zi ../src/main.cpp ../libs/gl3w/GL/gl3w.cpp /link ..\bin\SDL2.lib ..\bin\SDL2main.lib Winmm.lib opengl32.lib shlwapi.lib /SUBSYSTEM:WINDOWS

popd