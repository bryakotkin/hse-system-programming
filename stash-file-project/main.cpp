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
// openssl enc -aes-128-ctr -pbkdf2 -in ./1.txt -out ./1_enc.txt -pass pass:1234 - encrypt
// openssl enc -aes-128-ctr -pbkdf2 -d -in ./1_enc.txt -out ./3.txt -pass pass:1234 - decrypt

// https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption
// https://github.com/mohabouz/CPP-AES-OpenSSL-Encrypt/blob/master/utils.cpp

const int KEY_SIZE = 16;
const int IV_SIZE = 16;
const int ENCRYPTED_BYTES_SIZE = 8;

unsigned char KEY[KEY_SIZE] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35 };
unsigned char IV[IV_SIZE] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35 };

enum Mode {
    ENCRYPT,
    DECRYPT,
    INVALID_MODE
};

struct ModeHandler {
public:
    Mode map(const std::string& modeString) {
        if (modeString == "encrypt") {
            return ENCRYPT;
        } else if (modeString == "decrypt") {
            return DECRYPT;
        } else {
            return INVALID_MODE;
        }
    }
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
        std::cout << "Use: " << argv[0] << "<mode> <path to file>" << std::endl;
        return -1;
    }
    std::error_code error_code;
    if (!validatePath(argv[2], error_code)) {
        std::cerr << "Validation failure with error: " << error_code.message() << std::endl;
        return -1;
    }

    std::fstream file(argv[2], std::ios::in | std::ios::out | std::ios::binary); 
    if (!file) { 
        std::cerr << "Error opening file!" << std::endl; 
        return -1; 
    }

    ModeHandler modeHandler;
    Mode mode = modeHandler.map(argv[1]);
    
    const int size = ENCRYPTED_BYTES_SIZE;
    char* headerText = (char *) malloc(size + 1);
    unsigned char* result = (unsigned char *) malloc(size + 1);

    switch (mode) {
        case ENCRYPT:
            file.read(headerText, size);
            encrypt((unsigned char*) headerText, size, KEY, IV, result);
            file.seekp(0);
            file.write((char *) result, size);
            break;
        case DECRYPT:
            file.read(headerText, size);
            decrypt((unsigned char *) headerText, size, KEY, IV, result);
            file.seekp(0);
            file.write((char *) result, size);
            break;
        default:
            std::cerr << "Invalid mode!" << std::endl;
            free(headerText);
            free(result);
            file.close();
            return -1;
    }

    free(headerText);
    free(result);
    file.close();
    
    return 0;
}