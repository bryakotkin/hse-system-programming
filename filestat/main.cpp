#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <map>
 
void readDirectory(const std::string& name, std::vector<std::string>& v)
{
    DIR* dirp = opendir(name.c_str());
    if (!dirp) {
        std::cerr << "Error opening directory: " << name << std::endl;
        return;
    }
    struct dirent *dp;
    while ((dp = readdir(dirp)) != NULL) {
        v.push_back(dp->d_name);
    }
    closedir(dirp);
}

const std::string fileTypeDescription(unsigned int ifmp, mode_t mode) {
    std::string description;
    switch (ifmp) {
        case S_IFDIR:
            description = "DIRECTORY";
            break;
        case S_IFCHR:
            description = "CHARACTER DEVICE";
            break;
        case S_IFBLK:
            description = "BLOCK DEVICE";
            break;
        case S_IFREG:
            description = (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ? "EXECUTABLE FILE" : "REGULAR FILE";
            break;
        case S_IFIFO:
            description = "FIFO";
            break;
        case S_IFLNK:
            description = "SYMBOLIC LINK";
            break;
        case S_IFSOCK:
            description = "SOCKET";
            break;
        default:
            description = "UNKNOWN";
            break;
    }

    return description;
}

int main(int argc, char *argv[]) 
{
    std::vector<std::string> list;
    readDirectory("./", list);

    std::map<std::string, int> filestat;
    for (const auto& file : list) 
    {
        if (file == "." || file == "..") {
            continue;
        }
        struct stat buf;
        if (lstat(file.c_str(), &buf) != 0) {
            std::cerr << "Error getting stats for file: " << file << std::endl;
            continue;
        }
        int ifmp = buf.st_mode & S_IFMT;
        std::string fileDescription = fileTypeDescription(ifmp, buf.st_mode);

        filestat[fileDescription] += 1;
    }

    for (auto const& stat : filestat)
    {
        std::cout << stat.first << ": " << stat.second << std::endl;
    }

    return 0;
}