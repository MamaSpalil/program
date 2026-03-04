#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace crypto {

// AES-256-GCM encryption for API keys
// Uses PBKDF2-HMAC-SHA256 for key derivation (100000 iterations, 16-byte salt)
class KeyVault {
public:
    // Derive encryption key from password using PBKDF2
    static std::vector<uint8_t> deriveKey(const std::string& password,
                                           const std::vector<uint8_t>& salt,
                                           int iterations = 100000);

    // Generate random salt (16 bytes)
    static std::vector<uint8_t> generateSalt(int length = 16);

    // Encrypt plaintext with AES-256-GCM
    // Returns: salt(16) + iv(12) + tag(16) + ciphertext
    static std::vector<uint8_t> encrypt(const std::string& plaintext,
                                         const std::string& password);

    // Decrypt ciphertext encrypted with encrypt()
    // Input format: salt(16) + iv(12) + tag(16) + ciphertext
    static std::string decrypt(const std::vector<uint8_t>& data,
                                const std::string& password);

    // Convenience: encrypt to hex string
    static std::string encryptToHex(const std::string& plaintext,
                                     const std::string& password);

    // Convenience: decrypt from hex string
    static std::string decryptFromHex(const std::string& hexData,
                                       const std::string& password);

    // Hex encoding helpers
    static std::string toHex(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> fromHex(const std::string& hex);
};

} // namespace crypto
