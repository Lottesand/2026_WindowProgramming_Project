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
    
    // 사운드 일시정지 및 재개
    static void Pause(const std::string& key);
    static void Resume(const std::string& key);
    
    // 사운드 중지
    static void Stop(const std::string& key);

    // 모든 사운드 중지
    static void StopAll();

    // 메인 스레드에서 디바이스 열기
    static void InitDevices();

    // 볼륨 조절 (0 ~ 1.0)
    static void SetGlobalVolume(float volume);
    static float GetGlobalVolume() { return m_globalVolume; }

private:
    static std::map<std::string, std::wstring> m_sounds;
    static float m_globalVolume;
};
