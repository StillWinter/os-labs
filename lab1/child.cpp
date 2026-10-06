#include <unistd.h>
#include <cstring>
#include <string>
#include <cstdlib>

void error(const char *msg)
{
    write(2, msg, strlen(msg));
    write(2, "\n", 1);
    exit(1);
}

int main()
{
    std::string line;
    char c;

    while (true)
    {
        ssize_t n = read(0, &c, 1);
        if (n < 0)
            error("read error");
        if (n == 0)
            break;

        if (c == '\n')
        {
            std::string rev(line.rbegin(), line.rend());
            rev.push_back('\n');
            ssize_t written = write(1, rev.c_str(), rev.size());
            if (written < 0)
                error("write error");
            line.clear();
        }
        else
        {
            line.push_back(c);
        }
    }

    if (!line.empty())
    {
        std::string rev(line.rbegin(), line.rend());
        rev.push_back('\n');
        write(1, rev.c_str(), rev.size());
    }

    return 0;
}