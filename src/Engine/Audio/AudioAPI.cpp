#include "Engine/Audio/AudioAPI.hpp"

#include "Engine/Audio/SDLMixerBackend.hpp"
#ifdef _WIN32
#include "Engine/Audio/XAudio2Backend.hpp"
#endif

namespace VECTOR {

#ifdef VECTOR_BUILD_DIRECTX
    AudioAPI::API AudioAPI::s_API = AudioAPI::API::XAudio2;
#else
    AudioAPI::API AudioAPI::s_API = AudioAPI::API::SDL_Mixer;
#endif

    AudioAPI* AudioAPI::Create() {
        switch (s_API) {
            case API::None:
                return nullptr;
            case API::SDL_Mixer:
                return new SDLMixerBackend();
            case API::XAudio2:
#ifdef _WIN32
                return new XAudio2Backend();
#else
                return nullptr;
#endif
        }
        return nullptr;
    }

} // namespace VECTOR
