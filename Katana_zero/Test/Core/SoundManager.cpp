#include "SoundManager.h"
#include <mmsystem.h>

std::map<std::string, std::wstring> SoundManager::m_sounds;

void SoundManager::Init() {
    // 초기화 시 필요한 로직이 있다면 추가
}

void SoundManager::Release() {
    StopAll();
    m_sounds.clear();
}

void SoundManager::Load(const std::string& key, const std::wstring& path) {
    m_sounds[key] = path;
    
    // MCI를 사용하여 사운드 파일을 미리 엽니다.
    std::wstring command = L"open \"" + path + L"\" type mpegvideo alias " + std::wstring(key.begin(), key.end());
    mciSendStringW(command.c_str(), NULL, 0, NULL);
}

void SoundManager::Play(const std::string& key, bool loop) {
    if (m_sounds.find(key) == m_sounds.end()) return;

    // 재생 전 위치를 처음으로 되돌립니다.
    std::wstring seekCmd = L"seek " + std::wstring(key.begin(), key.end()) + L" to start";
    mciSendStringW(seekCmd.c_str(), NULL, 0, NULL);

    // 재생 명령
    std::wstring playCmd = L"play " + std::wstring(key.begin(), key.end());
    if (loop) playCmd += L" repeat";
    
    mciSendStringW(playCmd.c_str(), NULL, 0, NULL);
}

void SoundManager::Stop(const std::string& key) {
    if (m_sounds.find(key) == m_sounds.end()) return;

    std::wstring stopCmd = L"stop " + std::wstring(key.begin(), key.end());
    mciSendStringW(stopCmd.c_str(), NULL, 0, NULL);
}

void SoundManager::StopAll() {
    for (auto const& [key, path] : m_sounds) {
        Stop(key);
        std::wstring closeCmd = L"close " + std::wstring(key.begin(), key.end());
        mciSendStringW(closeCmd.c_str(), NULL, 0, NULL);
    }
}
