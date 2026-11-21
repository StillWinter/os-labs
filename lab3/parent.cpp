#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

constexpr size_t MAX_LINE = 4096;

struct SharedChannel {
    size_t len;
    bool eof;
    char buf[MAX_LINE];
};

struct ChannelCtx {
    std::string shm_name;
    std::string sem_empty_name;
    std::string sem_full_name;
    std::string out_file;
    int shm_fd = -1;
    SharedChannel* shm = nullptr;
    sem_t* sem_empty = SEM_FAILED;
    sem_t* sem_full = SEM_FAILED;
    pid_t pid = -1;
};

static void die(const char* msg) {
    write(2, msg, strlen(msg));
    write(2, "\n", 1);
    _exit(1);
}

static std::string read_line_stdin() {
    std::string line;
    char c = '\0';
    ssize_t n = 0;
    while ((n = read(0, &c, 1)) > 0) {
        line.push_back(c);
        if (c == '\n') {break;}}
    if (n < 0) {die("read stdin");}
    return line;
}

static void init_channel(ChannelCtx& ctx, pid_t parent_pid, int idx) {
    char buf_shm[32];
    char buf_emp[32];
    char buf_ful[32];
    std::snprintf(buf_shm, sizeof(buf_shm), "/m_%d_%d", static_cast<int>(parent_pid), idx);
    std::snprintf(buf_emp, sizeof(buf_emp), "/e_%d_%d", static_cast<int>(parent_pid), idx);
    std::snprintf(buf_ful, sizeof(buf_ful), "/f_%d_%d", static_cast<int>(parent_pid), idx);

    ctx.shm_name = buf_shm;
    ctx.sem_empty_name = buf_emp;
    ctx.sem_full_name = buf_ful;

    shm_unlink(ctx.shm_name.c_str());
    sem_unlink(ctx.sem_empty_name.c_str());
    sem_unlink(ctx.sem_full_name.c_str());

    ctx.shm_fd = shm_open(ctx.shm_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
    if (ctx.shm_fd < 0) die("shm_open");

    if (ftruncate(ctx.shm_fd, static_cast<off_t>(sizeof(SharedChannel))) != 0) {
        die("ftruncate");
    }

    void* addr = mmap(nullptr, sizeof(SharedChannel), PROT_READ | PROT_WRITE, MAP_SHARED, ctx.shm_fd, 0);
    if (addr == MAP_FAILED) die("mmap");

    ctx.shm = static_cast<SharedChannel*>(addr);
    ctx.shm->len = 0;
    ctx.shm->eof = false;
    ctx.shm->buf[0] = '\0';

    ctx.sem_empty = sem_open(ctx.sem_empty_name.c_str(), O_CREAT | O_EXCL, 0600, 1);
    if (ctx.sem_empty == SEM_FAILED) die("sem_open empty");

    ctx.sem_full = sem_open(ctx.sem_full_name.c_str(), O_CREAT | O_EXCL, 0600, 0);
    if (ctx.sem_full == SEM_FAILED) die("sem_open full");
}

static void send_line(ChannelCtx& ctx, const std::string& line) {
    if (line.size() >= MAX_LINE) {
        die("line too long for shared buffer");
    }
    if (sem_wait(ctx.sem_empty) != 0) die("sem_wait empty");

    std::memcpy(ctx.shm->buf, line.data(), line.size());
    ctx.shm->len = line.size();
    ctx.shm->eof = false;

    if (sem_post(ctx.sem_full) != 0) die("sem_post full");
}

static void send_eof(ChannelCtx& ctx) {
    if (sem_wait(ctx.sem_empty) != 0) die("sem_wait empty on eof");

    ctx.shm->len = 0;
    ctx.shm->eof = true;

    if (sem_post(ctx.sem_full) != 0) die("sem_post full on eof");
}

static void cleanup_channel(ChannelCtx& ctx) {
    if (ctx.shm && ctx.shm != MAP_FAILED) {
        munmap(ctx.shm, sizeof(SharedChannel));
        ctx.shm = nullptr;
    }
    if (ctx.shm_fd >= 0) {
        close(ctx.shm_fd);
        ctx.shm_fd = -1;
    }
    if (ctx.sem_empty && ctx.sem_empty != SEM_FAILED) {
        sem_close(ctx.sem_empty);
        ctx.sem_empty = SEM_FAILED;
    }
    if (ctx.sem_full && ctx.sem_full != SEM_FAILED) {
        sem_close(ctx.sem_full);
        ctx.sem_full = SEM_FAILED;
    }
    if (!ctx.shm_name.empty()) shm_unlink(ctx.shm_name.c_str());
    if (!ctx.sem_empty_name.empty()) sem_unlink(ctx.sem_empty_name.c_str());
    if (!ctx.sem_full_name.empty()) sem_unlink(ctx.sem_full_name.c_str());
}

int main() {
    std::string file1 = read_line_stdin();
    if (!file1.empty() && file1.back() == '\n') file1.pop_back();

    std::string file2 = read_line_stdin();
    if (!file2.empty() && file2.back() == '\n') file2.pop_back();

    if (file1.empty() || file2.empty()) {
        die("two output filenames required");
    }

    pid_t pid = getpid();
    ChannelCtx ch[2];
    ch[0].out_file = file1;
    ch[1].out_file = file2;

    init_channel(ch[0], pid, 0);
    init_channel(ch[1], pid, 1);

    for (int i = 0; i < 2; ++i) {
        pid_t cpid = fork();
        if (cpid < 0) die("fork");
        if (cpid == 0) {
            execl("./child", "./child",
                  ch[i].shm_name.c_str(),
                  ch[i].sem_empty_name.c_str(),
                  ch[i].sem_full_name.c_str(),
                  ch[i].out_file.c_str(),
                  nullptr);
            die("execl child");
        }
        ch[i].pid = cpid;
    }

    srand(static_cast<unsigned int>(time(nullptr)));

    for (std::string line = read_line_stdin(); !line.empty(); line = read_line_stdin()) {
        int r = rand() % 100;
        int idx = (r < 80) ? 0 : 1;
        send_line(ch[idx], line);
    }

    send_eof(ch[0]);
    send_eof(ch[1]);

    waitpid(ch[0].pid, nullptr, 0);
    waitpid(ch[1].pid, nullptr, 0);

    cleanup_channel(ch[0]);
    cleanup_channel(ch[1]);
    return 0;
}