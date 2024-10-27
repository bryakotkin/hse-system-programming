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

const std::string fileTypeDescription(unsigned int ifmp) {
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
            description = "REGULAR FILE";
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

    std::map<unsigned int, int> filestat;
    for (const auto& file : list) 
    {
        // if (file == "." || file == "..") {
        //     continue;
        // }
        struct stat buf;
        if (stat(file.c_str(), &buf) != 0) {
            std::cerr << "Error getting stats for file: " << file << std::endl;
            continue;
        }
        int ifmp = buf.st_mode & S_IFMT;

        filestat[ifmp] += 1;
    }

    for (auto const& stat : filestat)
    {
        std::string fileDescription = fileTypeDescription(stat.first);
        std::cout << fileDescription << ": " << stat.second << std::endl;
    }

    return 0;
}