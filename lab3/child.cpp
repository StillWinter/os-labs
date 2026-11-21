#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <string>

constexpr size_t MAX_LINE = 4096;

struct SharedChannel {
    size_t len;
    bool eof;
    char buf[MAX_LINE];
};

static void die(const char* msg) {
    write(2, msg, strlen(msg));
    write(2, "\n", 1);
    _exit(1);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        die("usage: child <shm_name> <sem_empty_name> <sem_full_name> <out_file>");
    }

    const char* shm_name = argv[1];
    const char* sem_empty_name = argv[2];
    const char* sem_full_name = argv[3];
    const char* out_path = argv[4];

    int out_fd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) die("open out_file");

    int shm_fd = shm_open(shm_name, O_RDWR, 0600);
    if (shm_fd < 0) die("shm_open in child");

    void* addr = mmap(nullptr, sizeof(SharedChannel), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (addr == MAP_FAILED) die("mmap in child");
    close(shm_fd);

    auto* ch = static_cast<SharedChannel*>(addr);

    sem_t* sem_empty = sem_open(sem_empty_name, 0);
    if (sem_empty == SEM_FAILED) die("sem_open empty in child");

    sem_t* sem_full = sem_open(sem_full_name, 0);
    if (sem_full == SEM_FAILED) die("sem_open full in child");

    bool running = true;
    while (running) {
        if (sem_wait(sem_full) != 0) die("sem_wait full");

        if (ch->eof) {
            if (sem_post(sem_empty) != 0) die("sem_post empty on eof");
            running = false;
        } else {
            std::string s(ch->buf, ch->len);
            bool has_nl = (!s.empty() && s.back() == '\n');
            if (has_nl) s.pop_back();
            std::reverse(s.begin(), s.end());
            if (has_nl) s.push_back('\n');
            size_t total = 0;
            while (total < s.size()) {
                ssize_t w = write(out_fd, s.data() + total, s.size() - total);
                if (w < 0) die("write out_file");
                total += static_cast<size_t>(w);
            }

            if (sem_post(sem_empty) != 0) die("sem_post empty");
        }
    }

    munmap(addr, sizeof(SharedChannel));
    close(out_fd);
    sem_close(sem_empty);
    sem_close(sem_full);
    return 0;
}