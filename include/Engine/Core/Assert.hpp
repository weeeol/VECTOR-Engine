#pragma once

#include "Engine/Core/Logger.hpp"
#include <string>

#if defined(_MSC_VER)
    #define VECTOR_DEBUG_BREAK __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
    #define VECTOR_DEBUG_BREAK __builtin_trap()
#else
    #define VECTOR_DEBUG_BREAK
#endif

// Always enable asserts in Debug builds
#ifndef NDEBUG
    #define VECTOR_ENABLE_ASSERTS
#endif

#ifdef _DEBUG
    #ifndef VECTOR_ENABLE_ASSERTS
        #define VECTOR_ENABLE_ASSERTS
    #endif
#endif

// Base Assertion
#ifdef VECTOR_ENABLE_ASSERTS
    #define VECTOR_ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                std::string assertMsg = std::string("Assertion Failed: ") + #condition + "\nMessage: " + (message) + "\nFile: " + __FILE__ + "\nLine: " + std::to_string(__LINE__); \
                VECTOR_LOG_ERROR(assertMsg); \
                VECTOR_DEBUG_BREAK; \
            } \
        } while (false)
#else
    #define VECTOR_ASSERT(condition, message) do { (void)(condition); } while(false)
#endif

// Halt execution unconditionally
#define VECTOR_HALT(message) \
    do { \
        std::string haltMsg = std::string("Fatal Error: ") + (message) + "\nFile: " + __FILE__ + "\nLine: " + std::to_string(__LINE__); \
        VECTOR_LOG_ERROR(haltMsg); \
        VECTOR_DEBUG_BREAK; \
        std::exit(-1); \
    } while (false)
