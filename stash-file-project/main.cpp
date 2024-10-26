#include <iostream>
#include <filesystem>
#include <openssl/evp.h>
#include <cstring>
#include <openssl/rand.h>
#include <fstream>
#include <string.h>
#include <map>

// Magic numbers
// https://gist.github.com/leommoore/f9e57ba2aa4bf197ebc5

// Пример использования openssl
// openssl enc -aes-128-ctr -pbkdf2 -in ./1.txt -out ./1_enc.txt -pass pass:1234 - enc
// openssl enc -aes-128-ctr -pbkdf2 -d -in ./1_enc.txt -out ./3.txt -pass pass:1234 - decrypt\

// https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption
// https://github.com/mohabouz/CPP-AES-OpenSSL-Encrypt/blob/master/utils.cpp

const int KEY_SIZE = 16;  // 128 бит = 16 байт
const int IV_SIZE = 16;   // IV для режима CTR также 16 байт
const int ENCRYPTED_BYTES_SIZE = 8;

enum Mode {
    ENCRYPT,
    DECRYPT
};

int encrypt(
    const unsigned char* inputData, 
    int inputLen, 
    unsigned char* key,
    unsigned char* iv, 
    unsigned char* outputData
) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int outputLen;

    if (!(ctx = EVP_CIPHER_CTX_new())) { return -1; }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv) != 1) {
        return -1;
    }

    if (EVP_EncryptUpdate(ctx, outputData, &len, inputData, inputLen) != 1) {
        return -1;
    }

    outputLen = len;
    if (EVP_EncryptFinal_ex(ctx, outputData + len, &len) != 1) {
        return -1;
    }
    outputLen += len;
    EVP_CIPHER_CTX_free(ctx);

    return outputLen;
}

int decrypt(
    const unsigned char* inputData, 
    int inputLen,
    unsigned char* key,
    unsigned char* iv, 
    unsigned char* outputData
) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int outputLen;

    if (!(ctx = EVP_CIPHER_CTX_new())) { return -1; }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv) != 1) {
        return -1;
    }

    if (EVP_DecryptUpdate(ctx, outputData, &len, inputData, inputLen) != 1) {
        return -1;
    }

    outputLen = len;
    if (EVP_DecryptFinal_ex(ctx, outputData + len, &len) != 1) {
        return -1;
    }
    outputLen += len;
    EVP_CIPHER_CTX_free(ctx);

    return outputLen;
}

unsigned char* encryptHeader(unsigned char* headerText, unsigned char* key, unsigned char* iv) {
    unsigned int size = std::strlen((char *) headerText);
    unsigned char* cipher = (unsigned char *) malloc(size);

    encrypt(headerText, size, key, iv, cipher);

    return cipher;
}

unsigned char* decryptHeader(unsigned char* ecryptedHeaderText, unsigned char* key, unsigned char* iv) {
    unsigned int size = std::strlen((char *) ecryptedHeaderText);
    unsigned char* decryptedText = (unsigned char *) malloc(size);

    decrypt(ecryptedHeaderText, size, key, iv, decryptedText);

    return decryptedText;
}

bool validatePath(const std::string& userPath, std::error_code& error_code) {
    const std::filesystem::path path(userPath);
    if (std::filesystem::is_directory(path, error_code)) {
        error_code = std::make_error_code(std::errc::is_a_directory);
        return false;
    }
    if (error_code) {
        return false;
    }
    if (std::filesystem::is_regular_file(path, error_code) && !error_code) {
        return true;
    }

    return false;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Use: " << argv[0] << " <path to file>" << std::endl;
        return -1;
    }
    std::error_code error_code;
    if (!validatePath(argv[2], error_code)) {
        std::cerr << "Validation failure with error: " << error_code.message() << std::endl;
        return -1;
    }
    
    // oh no cringe
    unsigned char key[KEY_SIZE] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35 };
    unsigned char iv[IV_SIZE] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35 };

    std::fstream file(argv[2], std::ios::in | std::ios::out | std::ios::binary); 
    if (!file) { 
        std::cerr << "Error opening file!" << std::endl; 
        return -1; 
    }

    std::map <std::string, Mode> mapping;
    mapping["decrypt"] = DECRYPT;
    mapping["encrypt"] = ENCRYPT;

    unsigned char* encrypted = nullptr;
    unsigned char* decrypted = nullptr;
    char* headerTextToEncrypt = (char *) malloc(ENCRYPTED_BYTES_SIZE + 1);
    char* headerTextToDecrypt = (char *) malloc(ENCRYPTED_BYTES_SIZE + 1);
    switch (mapping[argv[1]]) {
        case ENCRYPT:
            file.read(headerTextToEncrypt, ENCRYPTED_BYTES_SIZE);
            encrypted = encryptHeader((unsigned char *) headerTextToEncrypt, key, iv);
            file.seekp(0);
            file.write((char *) encrypted, ENCRYPTED_BYTES_SIZE);
            file.close();
            break;
        case DECRYPT:
            file.read(headerTextToDecrypt, ENCRYPTED_BYTES_SIZE);
            decrypted = decryptHeader((unsigned char *) headerTextToDecrypt, key, iv);
            file.seekp(0);
            file.write((char *) decrypted, ENCRYPTED_BYTES_SIZE);
            file.close();
            break;
        default:
            file.close();
            return -1;
    }
    
    return 0;
}