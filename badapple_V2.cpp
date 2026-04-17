#include <opencv2/opencv.hpp>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string>
#include <iostream>
#include <cstring>
#include <atomic>
#include <sys/wait.h>

static const std::string ASCII = " .:-=+*#%@";

struct TermSize {
    int rows;
    int cols;
};

static std::atomic<bool> g_stop(false);
static std::atomic<bool> g_resized(true);

static pid_t g_audio_pid = -1;

void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        g_stop = true;
    } else if (sig == SIGWINCH) {
        g_resized = true;
    }
}

void setup_signals() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGWINCH, &sa, NULL);
}

TermSize get_term_size() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return {w.ws_row, w.ws_col};
}

double now_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

inline char pixel_to_char(unsigned char p) {
    int idx = (p * (ASCII.size() - 1)) / 255;
    return ASCII[idx];
}

// loop_audio == true -> ffplay -loop 0
void start_audio(bool loop_audio) {
    pid_t pid = fork();

    if (pid == 0) {
        if (loop_audio) {
            execlp(
                "ffplay", "ffplay",
                "-nodisp",
                "-loglevel", "quiet",
                "-loop", "0",
                "bad_apple.wav",
                (char*)NULL
            );
        } else {
            execlp(
                "ffplay", "ffplay",
                "-nodisp",
                "-autoexit",
                "-loglevel", "quiet",
                "bad_apple.wav",
                (char*)NULL
            );
        }

        _exit(1);
    } else if (pid > 0) {
        g_audio_pid = pid;
    }
}

void stop_audio() {
    if (g_audio_pid > 0) {
        kill(g_audio_pid, SIGTERM);
        waitpid(g_audio_pid, NULL, 0);
        g_audio_pid = -1;
    }
}

void cleanup() {
    const char* msg = "\033[2J\033[Hbye bye\n";
    write(STDOUT_FILENO, msg, std::strlen(msg));
}

int main(int argc, char** argv) {
    setup_signals();

    bool play_sound = false;
    bool forever = false;
    bool help = false;
    
    for (int i = 1; i < argc; i++) {
        if (
            std::strcmp(argv[i], "--help") == 0
        ) {
            help = true;
        }
        if (
            std::strcmp(argv[i], "-s") == 0 ||
            std::strcmp(argv[i], "--song") == 0
        ) {
            play_sound = true;
        } else if (
            std::strcmp(argv[i], "-f") == 0 ||
            std::strcmp(argv[i], "--forever") == 0
        ) {
            forever = true;
        }
    }

    if(help)
    {
        std::cout << "usage: badapple [-s | --song] [-f | --forever]\nPixelated view of badApple";
        exit(0);
    }

    if (play_sound) {
        start_audio(forever);
    }

    cv::VideoCapture cap("bad_apple.mp4");
    if (!cap.isOpened()) {
        std::cerr << "Failed to open video\n";
        return 1;
    }

    double fps = cap.get(cv::CAP_PROP_FPS);
    double frame_time = 1.0 / fps;

    cv::Mat frame, gray, resized;

    std::string buffer;
    buffer.reserve(200000);

    std::printf("\033[2J");

    TermSize ts = get_term_size();

    double start = now_sec();
    int frame_idx = 0;

    while (!g_stop.load()) {
        if (!cap.read(frame)) {
            if (forever) {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                start = now_sec();
                frame_idx = 0;
                continue;
            } else {
                break;
            }
        }

        if (g_resized.exchange(false)) {
            ts = get_term_size();
        }

        float aspect = 4.0f / 3.0f;

        int height = ts.rows;
        int width = static_cast<int>(height * aspect);

        int char_width = 2;
        if (width * char_width > ts.cols) {
            char_width = 1;
        }

        cv::resize(frame, resized, cv::Size(width, height));
        cv::cvtColor(resized, gray, cv::COLOR_BGR2GRAY);

        int offset = (ts.cols - width * char_width) / 2;
        if (offset < 0) offset = 0;

        buffer.clear();
        buffer += "\033[H";

        for (int i = 0; i < gray.rows; i++) {
            buffer.append(offset, ' ');

            const unsigned char* row = gray.ptr<unsigned char>(i);

            for (int j = 0; j < gray.cols; j++) {
                char c = pixel_to_char(row[j]);

                if (char_width == 2) {
                    buffer += c;
                    buffer += c;
                } else {
                    buffer += c;
                }
            }

            buffer += '\n';
        }

        fwrite(buffer.c_str(), 1, buffer.size(), stdout);

        double elapsed = now_sec() - start;
        double target = frame_idx * frame_time;
        double sleep_time = target - elapsed;

        if (sleep_time > 0) {
            usleep((useconds_t)(sleep_time * 1e6));
        }

        frame_idx++;
    }

    stop_audio();
    cleanup();
    cap.release();

    return 0;
}