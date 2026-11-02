#include "Hardware/Core/SHA256.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <format>
#include <span>

namespace Hardware {

namespace {

constexpr std::array<uint32_t, 64> K = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

void ProcessBlock(std::array<uint32_t, 8>& state, std::span<const uint8_t> block) {
    std::array<uint32_t, 64> w{};

    for (size_t t = 0; t < 16; ++t) {
        w[t] = (static_cast<uint32_t>(block[t * 4]) << 24) | (static_cast<uint32_t>(block[t * 4 + 1]) << 16) |
               (static_cast<uint32_t>(block[t * 4 + 2]) << 8) | (static_cast<uint32_t>(block[t * 4 + 3]));
    }

    for (size_t t = 16; t < 64; ++t) {
        const uint32_t s0 = std::rotr(w[t - 15], 7) ^ std::rotr(w[t - 15], 18) ^ (w[t - 15] >> 3);
        const uint32_t s1 = std::rotr(w[t - 2], 17) ^ std::rotr(w[t - 2], 19) ^ (w[t - 2] >> 10);
        w[t] = w[t - 16] + s0 + w[t - 7] + s1;
    }

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];

    for (size_t t = 0; t < 64; ++t) {
        const uint32_t s1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
        const uint32_t ch = (e & f) ^ (~e & g);
        const uint32_t temp1 = h + s1 + ch + K[t] + w[t];
        const uint32_t s0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
        const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const uint32_t temp2 = s0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

}  // namespace

std::string SHA256::HashHex(std::string_view input) {
    std::array<uint32_t, 8> state = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                     0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    const auto bytes = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(input.data()), input.size());

    const size_t numBlocks = bytes.size() / 64;
    for (size_t i = 0; i < numBlocks; ++i) {
        ProcessBlock(state, bytes.subspan(i * 64, 64));
    }

    const size_t rem = bytes.size() % 64;
    std::array<uint8_t, 64> tail{};
    if (rem > 0) {
        std::copy_n(bytes.data() + (numBlocks * 64), rem, tail.begin());
    }
    tail[rem] = 0x80;

    const uint64_t bitLength = static_cast<uint64_t>(bytes.size()) * 8ULL;
    auto appendBitLength = [&tail](uint64_t len) {
        tail[56] = static_cast<uint8_t>(len >> 56);
        tail[57] = static_cast<uint8_t>(len >> 48);
        tail[58] = static_cast<uint8_t>(len >> 40);
        tail[59] = static_cast<uint8_t>(len >> 32);
        tail[60] = static_cast<uint8_t>(len >> 24);
        tail[61] = static_cast<uint8_t>(len >> 16);
        tail[62] = static_cast<uint8_t>(len >> 8);
        tail[63] = static_cast<uint8_t>(len);
    };

    if (rem < 56) {
        appendBitLength(bitLength);
        ProcessBlock(state, tail);
    } else {
        ProcessBlock(state, tail);
        tail.fill(0);
        appendBitLength(bitLength);
        ProcessBlock(state, tail);
    }

    return std::format("{:08x}{:08x}{:08x}{:08x}{:08x}{:08x}{:08x}{:08x}", state[0], state[1], state[2], state[3],
                       state[4], state[5], state[6], state[7]);
}

}  // namespace Hardware
