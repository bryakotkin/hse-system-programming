#include <iostream>
#include <filesystem>
#include <openssl/evp.h>
#include <cstring>
#include <openssl/rand.h>

const int KEY_SIZE = 16;  // 128 бит = 16 байт
const int IV_SIZE = 16;   // IV для режима CTR также 16 байт
const int ENCRYPTED_BYTES_SIZE = 8;

// Magic numbers
// https://gist.github.com/leommoore/f9e57ba2aa4bf197ebc5

// Пример использования openssl
// openssl enc -aes-128-ctr -pbkdf2 -in ./1.txt -out ./1_enc.txt -pass pass:1234 - enc
// openssl enc -aes-128-ctr -pbkdf2 -d -in ./1_enc.txt -out ./3.txt -pass pass:1234 - decrypt\

// https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption
// https://github.com/mohabouz/CPP-AES-OpenSSL-Encrypt/blob/master/utils.cpp

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

std::string encryptHeader(std::string headerText, unsigned char* key, unsigned char* iv) {
    unsigned char *headerTextBytes = (unsigned char *) headerText.c_str();
    unsigned int size = std::strlen((char *) headerTextBytes);
    unsigned char *cipher = (unsigned char *) malloc(sizeof(char) * size);

    int cipherSize = encrypt(
        headerTextBytes, 
        size, 
        key, 
        iv, 
        cipher
    );

    std::string cipherStr((char *) cipher);

    return cipherStr;
}

std::string decryptHeader(std::string ecryptedHeaderText, unsigned char* key, unsigned char* iv) {
    unsigned char *headerTextBytes = (unsigned char *) ecryptedHeaderText.c_str();
    unsigned int size = std::strlen((char *) headerTextBytes);
    unsigned char *decryptedText = (unsigned char *) malloc(sizeof(char) * size);

    int decryptedTextSize = decrypt(
        headerTextBytes, 
        size, 
        key, 
        iv,
        decryptedText
    );

    std::string resultText((char *) decryptedText);
    return resultText;
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
    if (argc != 2) {
        std::cout << "Use: " << argv[0] << " <path to file>" << std::endl;
        return -1;
    }
    const std::string userPath = argv[1];
    std::error_code error_code;
    if (!validatePath(userPath, error_code)) {
        std::cerr << "Validation failure with error: " << error_code.message() << std::endl;
        return -1;
    }

    unsigned char key[KEY_SIZE];
    unsigned char iv[IV_SIZE];
    unsigned char ciphertext[8];
    unsigned char decryptedtext[8];

    std::string plainText = "12345678";
    int plaintext_len = plainText.size();

    if (!RAND_bytes(key, KEY_SIZE) || !RAND_bytes(iv, IV_SIZE)) {
        return -1;
    }

    std::string encryptedText = encryptHeader(
        plainText,
        key,
        iv
    );

    std::string decryptedText = decryptHeader(
        encryptedText,
        key,
        iv
    );

    std::cout << encryptedText << " " << encryptedText.size() << std::endl;
    std::cout << decryptedText << std::endl;

    return 0;
}