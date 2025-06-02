#include "os-dependencies.h"

#include "SodiumCrypt.h"

#include <algorithm>
#include <sodium.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>

// Encrypt using XChaCha20-Poly1305
std::string SodiumEncrypt(const std::string& plaintext, const std::string& key)
{
    if (key.size() != crypto_aead_xchacha20poly1305_ietf_KEYBYTES) {
        throw std::runtime_error("Invalid key size");
    }

    // Generate a random nonce
    unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
    randombytes_buf(nonce, sizeof(nonce));

    // Allocate space for the ciphertext
    std::vector<unsigned char> ciphertext(plaintext.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES);

    // SodiumEncrypt the plaintext
    unsigned long long ciphertext_len;
    crypto_aead_xchacha20poly1305_ietf_encrypt(
        ciphertext.data(),
        &ciphertext_len,
        reinterpret_cast<const unsigned char*>(plaintext.data()),
        plaintext.size(),
        nullptr, 0, nullptr, nonce, reinterpret_cast<const unsigned char*>(key.data())
        );

        // Resize ciphertext to actual size
        ciphertext.resize(ciphertext_len);

        // Prepend the nonce to the ciphertext
        ciphertext.insert(ciphertext.begin(), nonce, nonce + sizeof(nonce));

        // Convert the binary ciphertext to a Base64-encoded string
        std::stringstream ss;
        for (unsigned char c : ciphertext) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
        return ss.str();
}

// Decrypt using XChaCha20-Poly1305
std::string SodiumDecrypt(const std::string& ciphertext, const std::string& key)
{
    if (key.size() != crypto_aead_xchacha20poly1305_ietf_KEYBYTES) {
        throw std::runtime_error("Invalid key size");
    }

    // Convert the hex-encoded ciphertext back to binary
    std::vector<unsigned char> binary_ciphertext;
    for (size_t i = 0; i < ciphertext.size(); i += 2) {
        std::string byteString = ciphertext.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoul(byteString, nullptr, 16));
        binary_ciphertext.push_back(byte);
    }

    // Extract the nonce from the beginning of the ciphertext
    unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
    std::copy_n(binary_ciphertext.begin(), sizeof(nonce), nonce);

    // Allocate space for the plaintext
    std::vector<unsigned char> plaintext(binary_ciphertext.size() - sizeof(nonce));

    // Decrypt the ciphertext
    unsigned long long plaintext_len;
    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
        plaintext.data(), &plaintext_len, nullptr,
        binary_ciphertext.data() + sizeof(nonce), binary_ciphertext.size() - sizeof(nonce),
        nullptr, 0, nonce, reinterpret_cast<const unsigned char*>(key.data())
    ) != 0) {
        throw std::runtime_error("Decryption failed");
    }

    // Resize plaintext to actual size
    plaintext.resize(plaintext_len);

    return std::string(plaintext.begin(), plaintext.end());  // NOLINT(modernize-return-braced-init-list)
}

