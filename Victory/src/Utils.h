#include <stdio.h>
#include <vector>
#include <string>

typedef unsigned int uint32_t;

namespace Utils {
    static std::vector<char> ReadFile(std::string&& path_) {
        FILE* file = fopen(path_.c_str(), "rb");

        if (!file) {
            printf("\nFile not readed: %s", path_.c_str());
            return {};
        }

        fseek(file, 0, SEEK_END);
        uint32_t length = static_cast<uint32_t>(ftell(file));
        fseek(file, 0, SEEK_SET);

        std::vector<char> buffer(length);
        fread(buffer.data(), 1, length, file);
        fclose(file);

        return buffer;
    }
} // namespace Utils