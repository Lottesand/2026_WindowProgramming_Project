#pragma once
#include <windows.h>
#include <string>
#include <map>

// Windows Multimedia API를 사용하기 위해 winmm.lib 링크
#pragma comment(lib, "winmm.lib")

class SoundManager {
public:
    static void Init();
    static void Release();

    // 사운드 파일 로드 (mp3, wav 등 지원)
    static void Load(const std::string& key, const std::wstring& path);

    // 사운드 재생
    static void Play(const std::string& key, bool loop = false);
    
    // 사운드 중지
    static void Stop(const std::string& key);

    // 모든 사운드 중지
    static void StopAll();

private:
    static std::map<std::string, std::wstring> m_sounds;
};
