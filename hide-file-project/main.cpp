#include <iostream>
#include <filesystem>

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
        if (std::rename(pathToFile.c_str(), resultPath.c_str())) {
            std::perror("File moving end with error");
            return -1;
        }
        resultPathFile = resultPath;

        return 0;
    }
};

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

    MoveFileController file(userPath, PATH_TO_HIDDEN_DIRECTORY);

    std::string resultPathFile;
    if (file.moveFileToDirectory(resultPathFile) != 0) {
        return -1;
    }

    std::cout << resultPathFile << std::endl;
    return 0;
}