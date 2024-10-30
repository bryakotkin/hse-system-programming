#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

const std::string PATH_TO_HIDDEN_DIRECTORY = "./my-hidden-directory";

class MoveFileController {
private:
    std::string const pathToFile;
    std::string const pathToDirectory;
    std::string const fileName;

public:
    MoveFileController(const std::string& pathToFile, const std::string& pathToDirectory)
        : pathToFile(pathToFile), pathToDirectory(pathToDirectory),
          fileName([&]() -> std::string {
              std::size_t found = pathToFile.find_last_of("/");
              if (found == std::string::npos) {
                  return pathToFile;
              } else {
                  return pathToFile.substr(found + 1);
              }
          }()) {}

    int moveFileToDirectory(std::string& resultPathFile) {
        std::string resultPath = pathToDirectory + "/" + fileName;
        if (rename(pathToFile.c_str(), resultPath.c_str()) != 0) {
            std::perror("File moving ended with an error");
            return -1;
        }
        resultPathFile = resultPath;
        return 0;
    }
};

bool createDirectoryIfNotExists(const std::string& directoryPath) {
    struct stat dirStat;

    if (stat(directoryPath.c_str(), &dirStat) == -1) {
        if (errno != ENOENT) {
            std::cerr << "Error accessing directory" << std::endl;
            return false;
        }
        if (mkdir(directoryPath.c_str(), S_IWUSR | S_IXUSR) != 0) {
            std::cerr << "Failed to create directory" << std::endl;
            return false;
        }

        return true;
    }

    if (!S_ISDIR(dirStat.st_mode)) {
        return false;
    }

    if (dirStat.st_mode & S_IRUSR || dirStat.st_mode & S_IRGRP || dirStat.st_mode & S_IROTH) {
        if (chmod(directoryPath.c_str(), dirStat.st_mode & ~(S_IRUSR | S_IRGRP | S_IROTH)) != 0) {
            std::cerr << "Failed to modify directory permissions: " << strerror(errno) << std::endl;
            return false;
        }
    }

    return true;
}

bool validatePath(const std::string& userPath) {
    struct stat fileStat;
    if (stat(userPath.c_str(), &fileStat) != 0) {
        std::cerr << "Error accessing file: " << strerror(errno) << std::endl;
        return false;
    }

    return S_ISREG(fileStat.st_mode);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <path to file>" << std::endl;
        return -1;
    }
    const std::string userPath = argv[1];
    if (!validatePath(userPath)) {
        std::cout << "Path is a directory" << std::endl;
        return -1;
    }

    if (!createDirectoryIfNotExists(PATH_TO_HIDDEN_DIRECTORY)) {
        return -1;
    }

    MoveFileController file(userPath, PATH_TO_HIDDEN_DIRECTORY);

    std::string resultPathFile;
    if (file.moveFileToDirectory(resultPathFile) != 0) {
        return -1;
    }

    std::cout << resultPathFile << std::endl;
    return 0;
}