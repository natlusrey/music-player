#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>
#include <regex>
namespace fs = std::filesystem;
class AudioPlayer {
private:
    pid_t playerPid = -1;
    bool isPlaying = false;
    bool isPaused = false;
    std::string currentFile;
    std::thread statusThread;
    bool stopStatusThread = false;
public:
    ~AudioPlayer() {
        stop();
        if (statusThread.joinable()) {
            stopStatusThread = true;
            statusThread.join();
        }
    }
    bool play(const std::string& filepath) {
        stop();
        playerPid = fork();
        if (playerPid == 0) {
            execl("./ffmpeg-master-latest-linux64-gpl/bin/ffplay", "ffplay", "-nodisp", "-autoexit", "-loglevel", "quiet", filepath.c_str(), (char*)nullptr);
            _exit(127);
        } else if (playerPid > 0) {
            currentFile = filepath;
            isPlaying = true;
            isPaused = false;
            stopStatusThread = false;
            if (statusThread.joinable()) {
                statusThread.join();
            }
            statusThread = std::thread([this]() {
                int status;
                while(!stopStatusThread) {
                    pid_t result = waitpid(playerPid, &status, WNOHANG);
                    if (result == playerPid || result == -1) {
                        isPaused = false;
                        playerPid = -1;
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });
            return true;
        }
        return false;
    }
    void pause() {
        if (playerPid > 0 && isPlaying && !isPaused) {
            if (kill(playerPid, SIGSTOP) == 0) isPaused = true;
        }
    }
    void resume() {
        if (playerPid > 0 && isPlaying && isPaused) {
            if (kill(playerPid, SIGCONT) == 0) isPaused = false;
        }
    }
    void stop() {
        stopStatusThread = true;
        if (playerPid > 0) {
            kill(playerPid, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            int status;
            pid_t result = waitpid(playerPid, &status, WNOHANG);
            if (result == 0) {
                kill(playerPid, SIGKILL);
                waitpid(playerPid, &status, 0);
            }
            playerPid = -1;
        }
        if (statusThread.joinable()) {
            statusThread.join();
        }
        isPlaying = false;
        isPaused = false;
        currentFile.clear();
    }
    bool getIsPlaying() const { return isPlaying && !isPaused; }
    bool getIsPaused() const { return isPaused; }
    bool getIsActive() const { return isPlaying; }
    std::string getCurrentFile() const { return currentFile; }
    bool isProcessAlive() const {
        if (playerPid <= 0) return false;
        return kill(playerPid, 0) == 0;
    }
};
static bool has_mp3_ext(const fs::path& p) {
    auto ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
    return ext == ".mp3";
}
struct MusicPlayer : Fl_Window {
    Fl_Button   *chooseFolderBtn;
    Fl_Button   *refreshBtn;
    Fl_Input    *urlInput;
    Fl_Button   *downloadBtn;
    Fl_Browser  *fileList;
    Fl_Button   *playBtn;
    Fl_Button   *pauseBtn;
    Fl_Button   *stopBtn;
    Fl_Button   *prevBtn;
    Fl_Button   *nextBtn;
    Fl_Slider   *progressSlider;
    std::string  currentDir;
    std::vector<std::string> filePaths;
    int currentTrack = -1;
    AudioPlayer player;
    MusicPlayer(int W, int H, const char* title) : Fl_Window(W, H, title) {
        begin();
        chooseFolderBtn = new Fl_Button(10, 10, 120, 30, "Choose Folder");
        refreshBtn = new Fl_Button(140, 10, 80, 30, "Refresh");
        urlInput = new Fl_Input(10, 60, W-130, 30, "");
        urlInput->label("YouTube URL:");
        urlInput->align(FL_ALIGN_TOP_LEFT);
        downloadBtn = new Fl_Button(W-110, 60, 100, 30, "Download");
        fileList = new Fl_Browser(10, 120, W-20, H-220);
        fileList->type(FL_HOLD_BROWSER);
        int controlY = H - 90;
        playBtn = new Fl_Button(10, controlY, 60, 30, "▶ Play");
        pauseBtn = new Fl_Button(80, controlY, 60, 30, "Pause");
        stopBtn = new Fl_Button(150, controlY, 60, 30, "Stop");
        prevBtn = new Fl_Button(220, controlY, 60, 30, "Prev");
        nextBtn = new Fl_Button(290, controlY, 60, 30, "Next");
        progressSlider = new Fl_Slider(10, controlY + 40, W-20, 30);
        progressSlider->type(FL_HOR_NICE_SLIDER);
        progressSlider->bounds(0, 100);
        progressSlider->value(0);
        chooseFolderBtn->callback(+[](Fl_Widget*, void* v){ static_cast<MusicPlayer*>(v)->chooseFolder(); }, this);
        refreshBtn->callback(+[](Fl_Widget*, void* v){ static_cast<MusicPlayer*>(v)->scanAndFill(); }, this);
        downloadBtn->callback(+[](Fl_Widget*, void* v){ static_cast<MusicPlayer*>(v)->downloadFromUrl(); }, this);
        fileList->callback(+[](Fl_Widget* w, void* v){
            auto* ui = static_cast<MusicPlayer*>(v);
            if (Fl::event() == FL_PUSH && Fl::event_clicks() == 1) {
                int idx = static_cast<Fl_Browser*>(w)->value();
                if (idx > 0 && idx <= (int)ui->filePaths.size()) {
                    ui->playTrack(idx - 1);
                }
            }
        }, this);
        playBtn->callback(+[](Fl_Widget*, void* v){ static_cast<MusicPlayer*>(v)->playCurrentOrSelected(); }, this);
        pauseBtn->callback(+[](Fl_Widget*, void* v){ static_cast<MusicPlayer*>(v)->pausePlayback(); }, this);
        stopBtn->callback(+[](Fl_Widget*, void* v){ static_cast<MusicPlayer*>(v)->stopPlayback(); }, this);
        prevBtn->callback(+[](Fl_Widget*, void* v){ static_cast<MusicPlayer*>(v)->previousTrack(); }, this);
        nextBtn->callback(+[](Fl_Widget*, void* v){ static_cast<MusicPlayer*>(v)->nextTrack(); }, this);
        end();
        resizable(fileList);
        scanAndFill();
        Fl::add_timeout(1.0, updateUI_static, this);
    }
    void chooseFolder() {
        Fl_Native_File_Chooser ch;
        ch.title("Select a folder to scan");
        ch.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
        if (!currentDir.empty()) ch.directory(currentDir.c_str());
        if (ch.show() == 0) {
            currentDir = ch.filename();
            scanAndFill();
        }
    }
    void scanAndFill() {
        fileList->clear();
        filePaths.clear();
        currentTrack = -1;
        if (currentDir.empty()) {
            fileList->add("Select a folder…");
            return;
        }
        try {
            for (auto const& entry : fs::recursive_directory_iterator(currentDir)) {
                if (!entry.is_regular_file()) continue;
                const auto& p = entry.path();
                if (has_mp3_ext(p)) {
                    std::string rel;
                    try {
                        rel = fs::relative(p, currentDir).generic_string();
                    } catch (...) {
                        rel = p.filename().generic_string();
                    }
                    fileList->add(rel.c_str());
                    filePaths.emplace_back(p.string());
                }
            }
            if (filePaths.empty()) fileList->add("(No MP3 files found)");
        } catch (const std::exception& e) {
            fl_alert("Error scanning directory:\n%s", e.what());
        }
    }
    void downloadFromUrl() {
        std::string url = urlInput->value();
        if (url.empty()) {
            fl_alert("Please enter a URL!");
            return;
        }
        downloadBtn->deactivate();
        std::thread([this, url]() {
            try {
                std::string cmd = "./yt-dlp_linux --no-playlist --ffmpeg-location ./ffmpeg-master-latest-linux64-gpl/bin --extract-audio --audio-format mp3 --audio-quality 0 -o \"" + currentDir + "/%(title)s.%(ext)s\" \"" + url + "\"";
                int result = system(cmd.c_str());
                Fl::lock();
                if (result == 0) {
                    scanAndFill();
                    fl_message("Download completed successfully!");
                } else {
                    fl_alert("Download failed! Make sure the URL is valid.");
                }
                downloadBtn->activate();
                Fl::unlock();
            } catch (const std::exception& e) {
                Fl::lock();
                fl_alert("Download error: %s", e.what());
                downloadBtn->activate();
                Fl::unlock();
            }
        }).detach();
    }
    void playTrack(int index) {
        if (index < 0 || index >= (int)filePaths.size()) return;   
        currentTrack = index;
        fileList->select(index + 1);
        if (player.play(filePaths[index])) {
            std::string filename = fs::path(filePaths[index]).filename().string();
            this->label(("Playing: " + filename).c_str());
        } else {
            fl_alert("Failed to play file!");
        }
    }
    void playCurrentOrSelected() {
        int selected = fileList->value();
        if (selected > 0 && selected <= (int)filePaths.size()) {
            playTrack(selected - 1);
        } else if (currentTrack >= 0 && player.getIsPaused()) {
            player.resume();
            std::string filename = fs::path(filePaths[currentTrack]).filename().string();
            this->label(("Playing: " + filename).c_str());
        } else if (currentTrack >= 0) {
            playTrack(currentTrack);
        } else if (!filePaths.empty()) {
            playTrack(0);
        }
    }
    void pausePlayback() {
        if (player.getIsPlaying()) {
            player.pause();
            std::string filename = fs::path(filePaths[currentTrack]).filename().string();
            this->label(("Paused: " + filename).c_str());
        } else if (player.getIsPaused()) {
            player.resume();
            std::string filename = fs::path(filePaths[currentTrack]).filename().string();
            this->label(("Playing: " + filename).c_str());
        }
    }
    void stopPlayback() {
        player.stop();
        this->label("Music Player");
        progressSlider->value(0);
        currentTrack = -1;
    }
    void previousTrack() {
        if (currentTrack > 0) {
            playTrack(currentTrack - 1);
        } else if (!filePaths.empty()) {
            playTrack(filePaths.size() - 1);
        }
    }
    void nextTrack() {
        if (currentTrack >= 0 && currentTrack < (int)filePaths.size() - 1) {
            playTrack(currentTrack + 1);
        } else if (!filePaths.empty()) {
            playTrack(0);
        }
    }
    static void updateUI_static(void* v) {
        static_cast<MusicPlayer*>(v)->updateUI();
        Fl::repeat_timeout(1.0, updateUI_static, v);
    }

    void updateUI() {
        if (player.getIsActive() && !player.isProcessAlive()) {
            nextTrack();
            return;
        }
        if (player.getIsPlaying()) {
            playBtn->label("▶ Playing");
            pauseBtn->label("Pause");
            pauseBtn->activate();
            stopBtn->activate();
        } else if (player.getIsPaused()) {
            playBtn->label("▶ Resume");
            pauseBtn->label("Paused");
            pauseBtn->activate();
            stopBtn->activate();
        } else {
            playBtn->label("▶ Play");
            pauseBtn->label("Pause");
            pauseBtn->deactivate();
            stopBtn->deactivate();
        }
        if (filePaths.empty()) {
            prevBtn->deactivate();
            nextBtn->deactivate();
        } else {
            prevBtn->activate();
            nextBtn->activate();
        }

        redraw();
    }
};

int main(int argc, char** argv) {
    Fl::lock();
    MusicPlayer player(800, 600, "Music Player");
    player.show(argc, argv);
    if (argc > 1) {
        try {
            player.currentDir = fs::path(argv[1]).string();
            player.scanAndFill();
        } catch (...) {}
    }
    return Fl::run();
}
