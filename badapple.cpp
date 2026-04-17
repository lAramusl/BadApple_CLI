#include <opencv2/opencv.hpp>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <vector>
#include <string>
#include <iostream>

using namespace cv;
using namespace std;

struct TermSize {
    int rows;
    int cols;
};

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

int main() {
    VideoCapture cap("bad_apple.mp4");
    if (!cap.isOpened()) {
        cerr << "Failed to open video\n";
        return 1;
    }

    double fps = cap.get(CAP_PROP_FPS);
    double frame_time = 1.0 / fps;

    Mat frame, gray, resized, thresh;

    string buffer;
    buffer.reserve(200000); // уменьшает realloc

    printf("\033[2J"); // очистка один раз

    double start = now_sec();
    int frame_idx = 0;

    while (true) {
        if (!cap.read(frame)) break;

        // --- динамический размер терминала ---
        TermSize ts = get_term_size();

        float aspect = 4.0f / 3.0f;

        int height = ts.rows;
        int width = (int)(height * aspect);

        // --- символы ---
        const char* white = "##";
        const char* black = "  ";
        int char_width = 2;

        if (width * char_width > ts.cols) {
            char_width = 1;
            white = "#";
            black = " ";
        }

        int render_width = width;
        int render_height = height;

        resize(frame, resized, Size(render_width, render_height));
        cvtColor(resized, gray, COLOR_BGR2GRAY);
        threshold(gray, thresh, 50, 255, THRESH_BINARY);

        int offset = (ts.cols - render_width * char_width) / 2;
        if (offset < 0) offset = 0;

        // --- буферизация ---
        buffer.clear();
        buffer += "\033[H"; // курсор в начало

        for (int i = 0; i < thresh.rows; i++) {
            buffer.append(offset, ' ');

            const uchar* row = thresh.ptr<uchar>(i);

            for (int j = 0; j < thresh.cols; j++) {
                buffer += (row[j] == 255) ? white : black;
            }
            buffer += '\n';
        }

        // --- вывод одним куском ---
        fwrite(buffer.c_str(), 1, buffer.size(), stdout);

        // --- синхронизация ---
        double elapsed = now_sec() - start;
        double target = frame_idx * frame_time;
        double sleep_time = target - elapsed;

        if (sleep_time > 0) {
            usleep((useconds_t)(sleep_time * 1e6));
        }

        frame_idx++;
    }

    printf("\033[2J\033[H"); // очистка в конце
    cap.release();
    return 0;
}