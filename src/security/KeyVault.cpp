#include "KeyVault.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace crypto {

static constexpr int kSaltLen = 16;
static constexpr int kIVLen = 12;
static constexpr int kTagLen = 16;
static constexpr int kKeyLen = 32; // AES-256

std::vector<uint8_t> KeyVault::deriveKey(const std::string& password,
                                          const std::vector<uint8_t>& salt,
                                          int iterations)
{
    std::vector<uint8_t> key(kKeyLen);
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                           salt.data(), static_cast<int>(salt.size()),
                           iterations, EVP_sha256(),
                           kKeyLen, key.data()) != 1) {
        throw std::runtime_error("PBKDF2 key derivation failed");
    }
    return key;
}

std::vector<uint8_t> KeyVault::generateSalt(int length) {
    std::vector<uint8_t> salt(length);
    if (RAND_bytes(salt.data(), length) != 1) {
        throw std::runtime_error("Failed to generate random salt");
    }
    return salt;
}

std::vector<uint8_t> KeyVault::encrypt(const std::string& plaintext,
                                        const std::string& password)
{
    auto salt = generateSalt(kSaltLen);
    auto key = deriveKey(password, salt);

    std::vector<uint8_t> iv(kIVLen);
    if (RAND_bytes(iv.data(), kIVLen) != 1)
        throw std::runtime_error("Failed to generate IV");

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    std::vector<uint8_t> ciphertext(plaintext.size());
    std::vector<uint8_t> tag(kTagLen);
    int len = 0, ciphertextLen = 0;

    try {
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
            throw std::runtime_error("EncryptInit failed");
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, kIVLen, nullptr) != 1)
            throw std::runtime_error("Set IV length failed");
        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1)
            throw std::runtime_error("EncryptInit key/iv failed");
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                              reinterpret_cast<const uint8_t*>(plaintext.data()),
                              static_cast<int>(plaintext.size())) != 1)
            throw std::runtime_error("EncryptUpdate failed");
        ciphertextLen = len;
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1)
            throw std::runtime_error("EncryptFinal failed");
        ciphertextLen += len;
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, kTagLen, tag.data()) != 1)
            throw std::runtime_error("Get tag failed");
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
    EVP_CIPHER_CTX_free(ctx);

    ciphertext.resize(ciphertextLen);

    // Output format: salt(16) + iv(12) + tag(16) + ciphertext
    std::vector<uint8_t> result;
    result.reserve(kSaltLen + kIVLen + kTagLen + ciphertextLen);
    result.insert(result.end(), salt.begin(), salt.end());
    result.insert(result.end(), iv.begin(), iv.end());
    result.insert(result.end(), tag.begin(), tag.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());
    return result;
}

std::string KeyVault::decrypt(const std::vector<uint8_t>& data,
                               const std::string& password)
{
    if (data.size() < static_cast<size_t>(kSaltLen + kIVLen + kTagLen))
        throw std::runtime_error("Encrypted data too short");

    std::vector<uint8_t> salt(data.begin(), data.begin() + kSaltLen);
    std::vector<uint8_t> iv(data.begin() + kSaltLen, data.begin() + kSaltLen + kIVLen);
    std::vector<uint8_t> tag(data.begin() + kSaltLen + kIVLen,
                              data.begin() + kSaltLen + kIVLen + kTagLen);
    std::vector<uint8_t> ciphertext(data.begin() + kSaltLen + kIVLen + kTagLen, data.end());

    auto key = deriveKey(password, salt);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    std::vector<uint8_t> plaintext(ciphertext.size());
    int len = 0, plaintextLen = 0;

    try {
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
            throw std::runtime_error("DecryptInit failed");
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, kIVLen, nullptr) != 1)
            throw std::runtime_error("Set IV length failed");
        if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1)
            throw std::runtime_error("DecryptInit key/iv failed");
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                              ciphertext.data(), static_cast<int>(ciphertext.size())) != 1)
            throw std::runtime_error("DecryptUpdate failed");
        plaintextLen = len;
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, kTagLen,
                                const_cast<uint8_t*>(tag.data())) != 1)
            throw std::runtime_error("Set tag failed");
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1)
            throw std::runtime_error("Decryption failed (wrong password or corrupted data)");
        plaintextLen += len;
    } catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
    EVP_CIPHER_CTX_free(ctx);

    return std::string(reinterpret_cast<char*>(plaintext.data()), plaintextLen);
}

std::string KeyVault::encryptToHex(const std::string& plaintext,
                                    const std::string& password)
{
    return toHex(encrypt(plaintext, password));
}

std::string KeyVault::decryptFromHex(const std::string& hexData,
                                      const std::string& password)
{
    return decrypt(fromHex(hexData), password);
}

std::string KeyVault::toHex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (uint8_t b : data)
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    return oss.str();
}

std::vector<uint8_t> KeyVault::fromHex(const std::string& hex) {
    if (hex.size() % 2 != 0)
        throw std::runtime_error("Invalid hex string length");
    std::vector<uint8_t> data;
    data.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        const std::string byteStr = hex.substr(i, 2);
        if (!std::isxdigit(static_cast<unsigned char>(byteStr[0])) ||
            !std::isxdigit(static_cast<unsigned char>(byteStr[1])))
            throw std::runtime_error("Invalid hex character at position " + std::to_string(i));
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
        data.push_back(byte);
    }
    return data;
}

} // namespace crypto
