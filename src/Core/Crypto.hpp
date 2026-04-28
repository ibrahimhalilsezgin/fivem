#pragma once
#include <cstdint>
#include <cstring>
#include <array>

// ============================================================
//  Compile-time XOR string encryption
//  Tüm stringler derleme zamanında şifrelenir, runtime'da çözülür
// ============================================================

namespace Cx {

    // Compile-time seed generation from __TIME__
    constexpr uint8_t _ks() {
        return static_cast<uint8_t>(
            (__TIME__[0] * 31 + __TIME__[1] * 17 + __TIME__[3] * 13 +
             __TIME__[4] * 7  + __TIME__[6] * 5  + __TIME__[7] * 3) ^ 0xA7
        );
    }

    // Per-character key derivation
    constexpr uint8_t _kc(size_t i, uint8_t seed) {
        return static_cast<uint8_t>((seed ^ (i * 0x6D + 0x3B)) & 0xFF);
    }

    template<size_t N>
    struct Es {
        std::array<char, N> d;
        uint8_t s;

        constexpr Es(const char(&str)[N], uint8_t seed) : d{}, s(seed) {
            for (size_t i = 0; i < N; ++i) {
                d[i] = str[i] ^ _kc(i, seed);
            }
        }

        // Runtime decryption - returns stack buffer (no heap allocation)
        void dec(char* out) const {
            for (size_t i = 0; i < N; ++i) {
                out[i] = d[i] ^ _kc(i, s);
            }
        }

        // Convenience: decrypt to stack array
        template<size_t M = N>
        std::array<char, M> get() const {
            std::array<char, M> buf{};
            for (size_t i = 0; i < N && i < M; ++i) {
                buf[i] = d[i] ^ _kc(i, s);
            }
            return buf;
        }
    };

    // Helper macro - creates encrypted string at compile time
    #define XS(str) []() { \
        constexpr auto _e = ::Cx::Es<sizeof(str)>(str, ::Cx::_ks()); \
        char _b[sizeof(str)]; \
        _e.dec(_b); \
        return std::string(_b); \
    }()

    // Helper macro for const char* (stack allocated, no std::string)
    #define XC(str, buf) \
        constexpr auto _xe_##buf = ::Cx::Es<sizeof(str)>(str, ::Cx::_ks()); \
        char buf[sizeof(str)]; \
        _xe_##buf.dec(buf)

    // Numeric value obfuscation
    template<typename T, T V, uint8_t K = _ks()>
    struct Nv {
        static constexpr T enc = V ^ static_cast<T>(K);
        static __forceinline T get() {
            volatile T r = enc ^ static_cast<T>(K);
            return r;
        }
    };

    #define XN(val) ::Cx::Nv<decltype(val), val>::get()
}
