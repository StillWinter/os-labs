#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cstring>

void error(const char *msg)
{
    write(2, msg, strlen(msg));
    write(2, "\n", 1);
    exit(1);
}

std::string read_line(int fd)
{
    std::string line;
    char c;
    while (true)
    {
        ssize_t n = read(fd, &c, 1);
        if (n < 0)
            error("read error");
        if (n == 0)
            break;
        line.push_back(c);
        if (c == '\n')
            break;
    }
    return line;
}

int main()
{
    std::string file1 = read_line(0);
    if (!file1.empty() && file1.back() == '\n')
        file1.pop_back();

    std::string file2 = read_line(0);
    if (!file2.empty() && file2.back() == '\n')
        file2.pop_back();

    int pipe1[2], pipe2[2];
    if (pipe(pipe1) < 0)
        error("pipe1");
    if (pipe(pipe2) < 0)
        error("pipe2");

    int fd1 = open(file1.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd1 < 0)
        error("open file1");
    int fd2 = open(file2.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd2 < 0)
        error("open file2");

    pid_t pid1 = fork();
    if (pid1 < 0)
        error("fork1");
    if (pid1 == 0)
    {
        if (dup2(pipe1[0], 0) < 0)
            error("dup2 pipe1 read");
        if (dup2(fd1, 1) < 0)
            error("dup2 fd1");
        close(pipe1[1]);
        close(pipe1[0]);
        close(pipe2[0]);
        close(pipe2[1]);
        close(fd1);
        close(fd2);
        execlp("./child", "./child", nullptr);
        error("exec child1");
    }

    pid_t pid2 = fork();
    if (pid2 < 0)
        error("fork2");
    if (pid2 == 0)
    {
        if (dup2(pipe2[0], 0) < 0)
            error("dup2 pipe2 read");
        if (dup2(fd2, 1) < 0)
            error("dup2 fd2");
        close(pipe1[1]);
        close(pipe1[0]);
        close(pipe2[0]);
        close(pipe2[1]);
        close(fd1);
        close(fd2);
        execlp("./child", "./child", nullptr);
        error("exec child2");
    }

    close(pipe1[0]);
    close(pipe2[0]);
    close(fd1);
    close(fd2);

    srand(time(NULL));

    while (true)
    {
        std::string line = read_line(0);
        if (line.empty())
            break;

        int r = rand() % 100;
        int fd = (r < 80) ? pipe1[1] : pipe2[1];

        ssize_t written = write(fd, line.c_str(), line.size());
        if (written < 0)
            error("write to pipe");
    }

    close(pipe1[1]);
    close(pipe2[1]);

    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);

    return 0;
}