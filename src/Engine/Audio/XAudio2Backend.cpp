#include "Engine/Audio/XAudio2Backend.hpp"
#include "Engine/Core/Logger.hpp"

#ifdef _WIN32
#include <fstream>
#include <iostream>

#pragma comment(lib, "xaudio2.lib")

namespace VECTOR {

    // Helper macro for finding chunks in RIFF file
    #define fourccRIFF 'FFIR'
    #define fourccDATA 'atad'
    #define fourccFMT ' tmf'
    #define fourccWAVE 'EVAW'
    #define fourccXWMA 'AMWX'
    #define fourccDPDS 'sdpd'

    static HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition) {
        HRESULT hr = S_OK;
        if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
            return HRESULT_FROM_WIN32(GetLastError());

        DWORD dwChunkType;
        DWORD dwChunkDataSize;
        DWORD dwRIFFDataSize = 0;
        DWORD dwFileType;
        DWORD bytesRead = 0;
        DWORD dwOffset = 0;

        while (hr == S_OK) {
            DWORD dwRead;
            if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());

            if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());

            switch (dwChunkType) {
                case fourccRIFF:
                    dwRIFFDataSize = dwChunkDataSize;
                    dwChunkDataSize = 4;
                    if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    break;

                default:
                    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                        return HRESULT_FROM_WIN32(GetLastError());
            }

            dwOffset += sizeof(DWORD) * 2;
            if (dwChunkType == fourcc) {
                dwChunkSize = dwChunkDataSize;
                dwChunkDataPosition = dwOffset;
                return S_OK;
            }

            dwOffset += dwChunkDataSize;
            if (bytesRead >= dwRIFFDataSize) return S_FALSE;
        }

        return S_OK;
    }

    static HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset) {
        HRESULT hr = S_OK;
        if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
            return HRESULT_FROM_WIN32(GetLastError());
        DWORD dwRead;
        if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    XAudio2Backend::XAudio2Backend() {}

    XAudio2Backend::~XAudio2Backend() {
        Shutdown();
    }

    bool XAudio2Backend::Initialize() {
        HRESULT hr;
        hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to initialize COM for XAudio2");
            return false;
        }

        hr = XAudio2Create(&m_XAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to init XAudio2 engine");
            return false;
        }

        hr = m_XAudio2->CreateMasteringVoice(&m_MasterVoice);
        if (FAILED(hr)) {
            VECTOR_LOG_ERROR("Failed to create mastering voice");
            return false;
        }

        VECTOR_LOG_INFO("XAudio2Backend initialized successfully");
        return true;
    }

    void XAudio2Backend::Shutdown() {
        if (m_MusicVoice) {
            m_MusicVoice->Stop();
            m_MusicVoice->DestroyVoice();
            m_MusicVoice = nullptr;
        }

        m_Sounds.clear();

        if (m_MasterVoice) {
            m_MasterVoice->DestroyVoice();
            m_MasterVoice = nullptr;
        }

        if (m_XAudio2) {
            m_XAudio2.Reset();
        }

        CoUninitialize();
    }

    bool XAudio2Backend::LoadWAV(const std::string& filepath, XAudio2Sound& outSound) {
        HANDLE hFile = CreateFile(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (INVALID_HANDLE_VALUE == hFile) {
            VECTOR_LOG_WARN("Could not open WAV file: " + filepath);
            return false;
        }
        if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN)) {
            CloseHandle(hFile);
            return false;
        }

        DWORD dwChunkSize;
        DWORD dwChunkPosition;
        FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
        DWORD filetype;
        ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
        if (filetype != fourccWAVE) {
            CloseHandle(hFile);
            VECTOR_LOG_WARN("File is not a valid WAV: " + filepath);
            return false;
        }

        FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
        ReadChunkData(hFile, &outSound.wfx, dwChunkSize, dwChunkPosition);

        FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
        outSound.audioData.resize(dwChunkSize);
        ReadChunkData(hFile, outSound.audioData.data(), dwChunkSize, dwChunkPosition);

        CloseHandle(hFile);
        return true;
    }

    void XAudio2Backend::PlaySound(const std::string& filepath) {
        if (m_Sounds.find(filepath) == m_Sounds.end()) {
            XAudio2Sound sound;
            if (!LoadWAV(filepath, sound)) return;
            m_Sounds[filepath] = sound;
        }

        const auto& sound = m_Sounds[filepath];
        IXAudio2SourceVoice* pSourceVoice;
        if (FAILED(m_XAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*)&sound.wfx))) {
            return;
        }

        XAUDIO2_BUFFER buffer = {0};
        buffer.AudioBytes = (UINT32)sound.audioData.size();
        buffer.pAudioData = sound.audioData.data();
        buffer.Flags = XAUDIO2_END_OF_STREAM;

        pSourceVoice->SubmitSourceBuffer(&buffer);
        pSourceVoice->Start(0);
        // Note: For a robust system, we would track source voices and release them when done.
    }

    void XAudio2Backend::PlayMusic(const std::string& filepath, int loops) {
        if (m_Sounds.find(filepath) == m_Sounds.end()) {
            XAudio2Sound sound;
            if (!LoadWAV(filepath, sound)) return;
            m_Sounds[filepath] = sound;
        }

        const auto& sound = m_Sounds[filepath];

        if (m_MusicVoice) {
            m_MusicVoice->Stop();
            m_MusicVoice->DestroyVoice();
            m_MusicVoice = nullptr;
        }

        if (FAILED(m_XAudio2->CreateSourceVoice(&m_MusicVoice, (WAVEFORMATEX*)&sound.wfx))) {
            return;
        }

        m_MusicVoice->SetVolume(m_MusicVolume);

        XAUDIO2_BUFFER buffer = {0};
        buffer.AudioBytes = (UINT32)sound.audioData.size();
        buffer.pAudioData = sound.audioData.data();
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        
        if (loops != 0) {
            buffer.LoopCount = (loops < 0) ? XAUDIO2_LOOP_INFINITE : loops;
        }

        m_MusicVoice->SubmitSourceBuffer(&buffer);
        m_MusicVoice->Start(0);
    }

    void XAudio2Backend::StopMusic() {
        if (m_MusicVoice) {
            m_MusicVoice->Stop();
            m_MusicVoice->FlushSourceBuffers();
        }
    }

    void XAudio2Backend::SetMusicVolume(float volume) {
        m_MusicVolume = volume;
        if (m_MusicVolume < 0.0f) m_MusicVolume = 0.0f;
        if (m_MusicVolume > 1.0f) m_MusicVolume = 1.0f;
        
        if (m_MusicVoice) {
            m_MusicVoice->SetVolume(m_MusicVolume);
        }
    }

} // namespace VECTOR
#endif
