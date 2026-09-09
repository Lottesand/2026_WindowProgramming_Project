#include "SoundManager.h"
#include <mmsystem.h>
#include <vector>
#include <algorithm>

std::map<std::string, std::wstring> SoundManager::m_sounds;
float SoundManager::m_globalVolume = 1.0f;

const int MAX_CHANNELS = 3;
static std::map<std::string, int> g_playCount;
static std::map<std::wstring, bool> g_openedAliases;

void SoundManager::Init() {
}

void SoundManager::Release() {
    StopAll();
    m_sounds.clear();
}

void SoundManager::Load(const std::string& key, const std::wstring& path) {
    wchar_t fullPath[MAX_PATH];
    GetFullPathNameW(path.c_str(), MAX_PATH, fullPath, NULL);
    
    if (GetFileAttributesW(fullPath) == INVALID_FILE_ATTRIBUTES) {
        std::wstring alt = L"../" + path;
        GetFullPathNameW(alt.c_str(), MAX_PATH, fullPath, NULL);
        if (GetFileAttributesW(fullPath) == INVALID_FILE_ATTRIBUTES) {
            alt = L"../../" + path;
            GetFullPathNameW(alt.c_str(), MAX_PATH, fullPath, NULL);
        }
    }

    if (GetFileAttributesW(fullPath) == INVALID_FILE_ATTRIBUTES) {
        OutputDebugStringW((L"[SoundManager] ERROR: File NOT FOUND -> " + path + L"\n").c_str());
        return;
    }

    m_sounds[key] = fullPath;
}

void SoundManager::InitDevices() {
    // Lazy-loading: 디바이스는 Play 호출 시 메인 스레드에서 생성됩니다.
}

void SoundManager::Play(const std::string& key, bool loop) {
    auto it = m_sounds.find(key);
    if (it == m_sounds.end()) return;

    int channel = 0;
    if (!loop) {
        channel = g_playCount[key] % MAX_CHANNELS;
        g_playCount[key]++;
    }

    std::wstring aliasBase = std::wstring(key.begin(), key.end());
    std::wstring alias = aliasBase + L"_" + std::to_wstring(channel);

    // 메인 스레드에서 처음 재생될 때 한 번만 디바이스를 엽니다.
    if (!g_openedAliases[alias]) {
        std::wstring openCmd = L"open \"" + it->second + L"\" type mpegvideo alias " + alias;
        MCIERROR err = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);
        if (err) {
            openCmd = L"open \"" + it->second + L"\" alias " + alias;
            mciSendStringW(openCmd.c_str(), NULL, 0, NULL);
        }
        
        int mciVol = (int)(m_globalVolume * 1000.0f);
        wchar_t volCmd[256];
        swprintf_s(volCmd, L"setaudio %s volume to %d", alias.c_str(), mciVol);
        mciSendStringW(volCmd, NULL, 0, NULL);
        
        g_openedAliases[alias] = true;
    }

    mciSendStringW((L"seek " + alias + L" to start").c_str(), NULL, 0, NULL);

    std::wstring playCmd = L"play " + alias;
    if (loop) playCmd += L" repeat";
    
    mciSendStringW(playCmd.c_str(), NULL, 0, NULL);
}

void SoundManager::Pause(const std::string& key) {
    if (m_sounds.find(key) == m_sounds.end()) return;
    std::wstring aliasBase = std::wstring(key.begin(), key.end());
    for (int i = 0; i < MAX_CHANNELS; i++) {
        std::wstring alias = aliasBase + L"_" + std::to_wstring(i);
        mciSendStringW((L"pause " + alias).c_str(), NULL, 0, NULL);
    }
}

void SoundManager::Resume(const std::string& key) {
    if (m_sounds.find(key) == m_sounds.end()) return;
    std::wstring aliasBase = std::wstring(key.begin(), key.end());
    for (int i = 0; i < MAX_CHANNELS; i++) {
        std::wstring alias = aliasBase + L"_" + std::to_wstring(i);
        mciSendStringW((L"resume " + alias).c_str(), NULL, 0, NULL);
    }
}

void SoundManager::Stop(const std::string& key) {
    if (m_sounds.find(key) == m_sounds.end()) return;
    std::wstring aliasBase = std::wstring(key.begin(), key.end());
    for (int i = 0; i < MAX_CHANNELS; i++) {
        std::wstring alias = aliasBase + L"_" + std::to_wstring(i);
        mciSendStringW((L"stop " + alias).c_str(), NULL, 0, NULL);
    }
}

void SoundManager::StopAll() {
    for (auto const& [key, path] : m_sounds) {
        std::wstring aliasBase = std::wstring(key.begin(), key.end());
        for (int i = 0; i < MAX_CHANNELS; i++) {
            std::wstring alias = aliasBase + L"_" + std::to_wstring(i);
            mciSendStringW((L"stop " + alias).c_str(), NULL, 0, NULL);
        }
    }
}

void SoundManager::SetGlobalVolume(float volume) {
    m_globalVolume = (std::max)(0.0f, (std::min)(1.0f, volume));
    
    int mciVol = (int)(m_globalVolume * 1000.0f);
    for (auto const& [alias, isOpen] : g_openedAliases) {
        if (isOpen) {
            wchar_t volCmd[256];
            swprintf_s(volCmd, L"setaudio %s volume to %d", alias.c_str(), mciVol);
            mciSendStringW(volCmd, NULL, 0, NULL);
        }
    }
}
