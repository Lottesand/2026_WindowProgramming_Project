#include "SoundManager.h"
#include <mmsystem.h>
#include <vector>
#include <algorithm>

std::map<std::string, std::wstring> SoundManager::m_sounds;
float SoundManager::m_globalVolume = 1.0f;

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

const int CHANNEL_COUNT = 5;

void SoundManager::InitDevices() {
    for (auto const& [key, path] : m_sounds) {
        std::wstring aliasBase = std::wstring(key.begin(), key.end());
        
        for (int i = 0; i < CHANNEL_COUNT; i++) {
            std::wstring alias = aliasBase + L"_" + std::to_wstring(i);
            mciSendStringW((L"close " + alias).c_str(), NULL, 0, NULL);

            std::wstring command = L"open \"" + path + L"\" type mpegvideo alias " + alias;
            MCIERROR err = mciSendStringW(command.c_str(), NULL, 0, NULL);
            
            if (err) {
                command = L"open \"" + path + L"\" alias " + alias;
                err = mciSendStringW(command.c_str(), NULL, 0, NULL);
            }
        }
    }
    SetGlobalVolume(m_globalVolume);
}

static std::map<std::string, int> g_lastChannel;

void SoundManager::Play(const std::string& key, bool loop) {
    auto it = m_sounds.find(key);
    if (it == m_sounds.end()) return;

    std::wstring aliasBase = std::wstring(key.begin(), key.end());
    
    // 루프인 경우 0번 채널만 사용 (중복 재생 방지)
    if (loop) {
        std::wstring alias = aliasBase + L"_0";
        mciSendStringW((L"seek " + alias + L" to start").c_str(), NULL, 0, NULL);
        mciSendStringW((L"play " + alias + L" repeat").c_str(), NULL, 0, NULL);
        return;
    }

    // 일반 효과음은 채널을 돌려가며 재생 (Overlapping 지원)
    int channel = g_lastChannel[key];
    std::wstring alias = aliasBase + L"_" + std::to_wstring(channel);
    
    mciSendStringW((L"seek " + alias + L" to start").c_str(), NULL, 0, NULL);
    mciSendStringW((L"play " + alias).c_str(), NULL, 0, NULL);

    g_lastChannel[key] = (channel + 1) % CHANNEL_COUNT;
}

void SoundManager::Pause(const std::string& key) {
    if (m_sounds.find(key) == m_sounds.end()) return;
    std::wstring aliasBase = std::wstring(key.begin(), key.end());
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        mciSendStringW((L"pause " + aliasBase + L"_" + std::to_wstring(i)).c_str(), NULL, 0, NULL);
    }
}

void SoundManager::Resume(const std::string& key) {
    if (m_sounds.find(key) == m_sounds.end()) return;
    std::wstring aliasBase = std::wstring(key.begin(), key.end());
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        mciSendStringW((L"resume " + aliasBase + L"_" + std::to_wstring(i)).c_str(), NULL, 0, NULL);
    }
}

void SoundManager::Stop(const std::string& key) {
    if (m_sounds.find(key) == m_sounds.end()) return;
    std::wstring aliasBase = std::wstring(key.begin(), key.end());
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        mciSendStringW((L"stop " + aliasBase + L"_" + std::to_wstring(i)).c_str(), NULL, 0, NULL);
    }
}

void SoundManager::StopAll() {
    for (auto const& [key, path] : m_sounds) {
        std::wstring aliasBase = std::wstring(key.begin(), key.end());
        for (int i = 0; i < CHANNEL_COUNT; i++) {
            mciSendStringW((L"stop " + aliasBase + L"_" + std::to_wstring(i)).c_str(), NULL, 0, NULL);
        }
    }
}

void SoundManager::SetGlobalVolume(float volume) {
    m_globalVolume = (std::max)(0.0f, (std::min)(1.0f, volume));
    
    int mciVol = (int)(m_globalVolume * 1000.0f);
    for (auto const& [key, path] : m_sounds) {
        std::wstring aliasBase = std::wstring(key.begin(), key.end());
        for (int i = 0; i < CHANNEL_COUNT; i++) {
            wchar_t volCmd[256];
            swprintf_s(volCmd, L"setaudio %s_%d volume to %d", aliasBase.c_str(), i, mciVol);
            mciSendStringW(volCmd, NULL, 0, NULL);
        }
    }
}
