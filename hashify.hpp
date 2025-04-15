#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <cstring>
#include <string>
#include <type_traits>
#include <sstream>
#include <iomanip>
#include <ostream>

namespace Hashify {

    using u8  = uint8_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    class MerryGoRound {
    public:
        // conversion helpers to byte vector
        static std::vector<u8> to_bytes(const std::string &str) {
            return std::vector<u8>(str.begin(), str.end());
        }

        static std::vector<u8> to_bytes(const char *cstr) {
            return std::vector<u8>(cstr, cstr + std::strlen(cstr));
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        static std::vector<u8> to_bytes(T value) {
            std::vector<u8> bytes(sizeof(T));
            std::memcpy(bytes.data(), &value, sizeof(T));
            return bytes;
        }

        template <typename T>
        static std::vector<u8> to_bytes(const std::vector<T> &v) {
            std::vector<u8> bytes(v.size() * sizeof(T));
            std::memcpy(bytes.data(), v.data(), bytes.size());
            return bytes;
        }

        static std::vector<u8> hash(const std::vector<u8> &raw_input) {
            std::vector<u8> input = raw_input;

            // calc total bits
            u64 input_bits = input.size() * 8;

            // append 8 bytes of the bit length to the input
            // little endian (padding)
            for (int i = 0; i < 8; ++i)
                input.push_back((input_bits >> (i * 8)) & 0xFF);

            // pad input with 0x80 until size is a multiple of 64
            // this is in order to process in blocks of 64
            while (input.size() % 64 != 0)
                input.push_back(0x80);

            // message schedule array of 64 32bit words
            std::vector<u32> msg_schedule(64);
            for (size_t i = 0; i < 64; ++i) {
                // choose a byte from input for each idx
                // if i >= input.size() use a fallback based on a cyclic input byte plus an offset
                u32 v = i < input.size() ? input[i] : (input[i % input.size()] + i * 2654435761u);

                // xor
                // rol
                // add offset
                msg_schedule[i] = rol_(v ^ M_[i % 8], i % 31) + (i * 31);
            }

            // derived from fractional parts of cube roots of primes
            // 167, 173, 179, 181, 191, 193, 197, 199
            u32 state[8] = {
                0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
                0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624
            };

            // used for state mixing
            // cube roots of primes
            u32 phase[4] = { 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb }; // 149, 151, 157, 163
            u32 freq[4] = { 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85 }; // 113, 127, 131, 137

            // process each byte
            for (size_t i = 0; i < input.size(); ++i) {
                u8 b = input[i];

                for (int j = 0; j < 4; ++j) {
                    // ror
                    // xor
                    freq[j] ^= ror_(phase[j] + b + state[j * 2], (b % 5) + 1);

                    // add xor of freq and next state element
                    phase[j] += freq[j] ^ state[(j + 1) % 8];
                }

                // update internal state
                for (int j = 0; j < 8; ++j) {
                    state[j] ^= rol_((phase[j % 4] ^ state[(j + 3) % 8]), (j + b) % 32);
                    state[j] = (state[j] * M_[j]) ^ (state[(j + 5) % 8] >> (b % 7));
                    state[j] ^= SBOX_[SBOX_[(state[j] ^ b) & 0xFF]];
                    state[j] ^= rol_(state[(j + 1) % 8] + state[(j + 2) % 8], (j * 7) % 32);
                }

                // more mixing
                for (int k = 0; k < 4; ++k) {
                    state[k] ^= phase[k] ^ freq[k];
                    phase[k] ^= state[(k * 2 + 1) % 8];
                }

                // conditional swap
                if (b % 7 == 0)
                    std::swap(state[b % 8], state[(b * 3) % 8]);

                // every 8 bytes we apply an extra mixing round
                if (i % 8 == 0) {
                    for (int j = 0; j < 8; ++j) {
                        u32 mix = state[j] ^ state[(j + 2) % 8] ^ rol_(state[(j + 5) % 8], (j * 3) % 32);
                        state[j] = rol_(mix * 0xC1A551F1ED, (j * 7) % 32);
                    }
                }
            }

            // final mixing
            // 20 rounds of mixing using msg_schedule
            for (int r = 0; r < 20; ++r) {
                for (int i = 0; i < 8; ++i) {
                    state[i] ^= rol_(state[(i + r) % 8] + msg_schedule[(i + r * 3) % 64], (i + r) % 31);
                }
            }

            for (int i = 0; i < 8; ++i) {
                state[i] ^= msg_schedule[(i * 3) % 64] ^ phase[i % 4];
                state[i] ^= rol_(state[(i + 3) % 8] + state[(i + 5) % 8], (i * 13) % 32);
            }

            std::vector<u8> output;
            output.reserve(32); // 8 state words * 4 bytes
            for (int i = 0; i < 8; ++i)
                for (int b = 0; b < 4; ++b)
                    output.push_back(static_cast<u8>((state[i] >> (b * 8)) & 0xFF));

            return output;
        }
    
        template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, std::vector<u8>>>>
        static std::vector<u8> hash(const T& input) {
            return hash(to_bytes(input));
        }

        // for ease of display
        static std::string hash_to_hex(const std::vector<u8>& raw_input) {
            std::vector<u8> result = hash(raw_input);
            return to_hex_string(result);
        }

        static std::string to_hex_string(const std::vector<u8>& bytes) {
            std::ostringstream oss;
            for (u8 b : bytes) {
                oss << std::hex << std::setfill('0') << std::setw(2)
                    << static_cast<int>(b);
            }
            return oss.str();
        }

    private:
        static u32 rol_(u32 x, u32 r) {
            return (x << r) | (x >> (32 - r));
        }

        static u32 ror_(u32 x, u32 r) {
            return (x >> r) | (x << (32 - r));
        }

        // shuffled bijective sbox from 0x00 to 0xFF
        static constexpr u8 SBOX_[256] = {
            0x8e, 0xd5, 0x61, 0x60, 0xc3, 0x79, 0x52, 0xf2, 0x29, 0x68, 0x97, 0xdf, 0xe9, 0x94, 0x3d, 0x6e,
            0x11, 0xde, 0x93, 0x81, 0xcb, 0x1c, 0xf0, 0x08, 0x89, 0x5e, 0x2a, 0x20, 0x59, 0xf8, 0xa8, 0xf7,
            0xc8, 0x00, 0x10, 0x0a, 0x07, 0x02, 0xf5, 0x3e, 0x18, 0xc9, 0x36, 0xa7, 0x03, 0x0b, 0x41, 0x21,
            0x35, 0xae, 0xa1, 0x9f, 0xdc, 0xfc, 0xb7, 0xb6, 0x19, 0xd8, 0x56, 0xc7, 0xb5, 0xbb, 0xff, 0xf6,
            0x43, 0x54, 0x06, 0x23, 0x6d, 0xef, 0x91, 0x87, 0x4e, 0x7d, 0xec, 0x9b, 0xb3, 0x65, 0x58, 0x09,
            0xe3, 0x69, 0x67, 0xfd, 0x2d, 0x84, 0xd7, 0x3b, 0x95, 0x49, 0xf1, 0xcf, 0x2b, 0x83, 0xc6, 0x64,
            0xf4, 0xc5, 0xbf, 0xda, 0x3a, 0x6c, 0xe1, 0x63, 0x47, 0xe5, 0x33, 0xcc, 0x24, 0xa5, 0x31, 0x34,
            0x45, 0x74, 0x14, 0x5c, 0x7c, 0xb4, 0xc2, 0x99, 0x44, 0x57, 0xa0, 0x32, 0x13, 0xc4, 0xe7, 0x5a,
            0x3c, 0x9a, 0x0c, 0x1e, 0xe2, 0xbe, 0x27, 0xe4, 0x22, 0x15, 0x05, 0xb9, 0x42, 0x7a, 0x53, 0x46,
            0xc0, 0xed, 0x30, 0x7b, 0x1b, 0xca, 0x4a, 0xa4, 0xee, 0x26, 0xe0, 0xd3, 0x0d, 0x85, 0x0e, 0xc1,
            0xd6, 0xd4, 0x55, 0x66, 0xe6, 0xb8, 0x4c, 0x4f, 0x78, 0xa3, 0xd0, 0x90, 0x37, 0x38, 0xaa, 0x39,
            0xe8, 0x75, 0x8b, 0x96, 0xce, 0xdb, 0x16, 0x4b, 0x6f, 0xa6, 0x6a, 0xd9, 0xdd, 0x51, 0x88, 0x8d,
            0x2e, 0x62, 0x9e, 0x0f, 0x1a, 0xea, 0x8a, 0x5f, 0xcd, 0x76, 0x9c, 0xba, 0xb2, 0x12, 0x72, 0x04,
            0x77, 0x7f, 0xb1, 0x80, 0x25, 0x1f, 0x50, 0x82, 0x92, 0xfe, 0x9d, 0x17, 0x01, 0x8c, 0xac, 0xab,
            0x73, 0xd2, 0x48, 0x40, 0xfa, 0xa9, 0xbd, 0x7e, 0x70, 0x86, 0xfb, 0x98, 0xbc, 0xf3, 0x2f, 0x6b,
            0x5b, 0xb0, 0x3f, 0x5d, 0xad, 0x2c, 0x8f, 0xa2, 0x71, 0xeb, 0x28, 0xaf, 0x4d, 0x1d, 0xf9, 0xd1
        };

        // used for mixing
        static constexpr u32 M_[8] = {
            0x12835b01, // 29
            0x243185be, // 31
            0x550c7dc3, // 37
            0x72be5d74, // 41
            0x80deb1fe, // 43
            0x9bdc06a7, // 47
            0xc19bf174, // 53
            0xe49b69c1  // 59
        };
    };

    inline std::ostream& operator<<(std::ostream& os, const std::vector<u8>& bytes) {
        os << MerryGoRound::to_hex_string(bytes);
        return os;
    }

}
