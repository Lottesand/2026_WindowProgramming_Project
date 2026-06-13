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

void SoundManager::InitDevices() {
    // 모든 사운드를 MCI로 열어서 다중 재생이 가능하도록 합니다.
    for (auto const& [key, path] : m_sounds) {
        std::wstring alias = std::wstring(key.begin(), key.end());
        mciSendStringW((L"close " + alias).c_str(), NULL, 0, NULL);

        // WAV와 MP3 모두 mpegvideo 타입으로 열면 범용적으로 작동하며 다중 채널 재생이 가능해집니다.
        std::wstring command = L"open \"" + path + L"\" type mpegvideo alias " + alias;
        MCIERROR err = mciSendStringW(command.c_str(), NULL, 0, NULL);
        
        if (err) {
            // mpegvideo로 실패하면 자동 감지로 재시도
            command = L"open \"" + path + L"\" alias " + alias;
            err = mciSendStringW(command.c_str(), NULL, 0, NULL);
        }

        if (err) {
            wchar_t errText[256];
            mciGetErrorStringW(err, errText, 256);
            OutputDebugStringW((L"[SoundManager] MCI Load Error (" + alias + L"): " + errText + L"\n").c_str());
        } else {
            OutputDebugStringW((L"[SoundManager] MCI Loaded: " + alias + L" -> " + path + L"\n").c_str());
        }
    }
    SetGlobalVolume(m_globalVolume);
}

void SoundManager::Play(const std::string& key, bool loop) {
    auto it = m_sounds.find(key);
    if (it == m_sounds.end()) return;

    std::wstring alias = std::wstring(key.begin(), key.end());

    // 재생 전 처음으로 감기 (다중 재생을 위해 필수)
    mciSendStringW((L"seek " + alias + L" to start").c_str(), NULL, 0, NULL);
    
    std::wstring playCmd = L"play " + alias;
    if (loop) playCmd += L" repeat";
    mciSendStringW(playCmd.c_str(), NULL, 0, NULL);
}

void SoundManager::Pause(const std::string& key) {
    if (m_sounds.find(key) == m_sounds.end()) return;
    std::wstring alias = std::wstring(key.begin(), key.end());
    mciSendStringW((L"pause " + alias).c_str(), NULL, 0, NULL);
}

void SoundManager::Resume(const std::string& key) {
    if (m_sounds.find(key) == m_sounds.end()) return;
    std::wstring alias = std::wstring(key.begin(), key.end());
    mciSendStringW((L"resume " + alias).c_str(), NULL, 0, NULL);
}

void SoundManager::Stop(const std::string& key) {
    if (m_sounds.find(key) == m_sounds.end()) return;
    std::wstring alias = std::wstring(key.begin(), key.end());
    mciSendStringW((L"stop " + alias).c_str(), NULL, 0, NULL);
}

void SoundManager::StopAll() {
    for (auto const& [key, path] : m_sounds) {
        std::wstring alias = std::wstring(key.begin(), key.end());
        mciSendStringW((L"stop " + alias).c_str(), NULL, 0, NULL);
    }
}

void SoundManager::SetGlobalVolume(float volume) {
    m_globalVolume = (std::max)(0.0f, (std::min)(1.0f, volume));
    
    // MCI 볼륨 설정 (0 ~ 1000)
    int mciVol = (int)(m_globalVolume * 1000.0f);
    for (auto const& [key, path] : m_sounds) {
        std::wstring alias = std::wstring(key.begin(), key.end());
        wchar_t volCmd[256];
        swprintf_s(volCmd, L"setaudio %s volume to %d", alias.c_str(), mciVol);
        mciSendStringW(volCmd, NULL, 0, NULL);
    }
}
