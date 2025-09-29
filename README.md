# C++ FLTK Music Player

A simple and lightweight music player built with C++ and the FLTK library. It allows you to browse and play local MP3 files and download audio from YouTube URLs.

## Features

- **Local Playback**: Browse a directory on your filesystem and recursively find all `.mp3` files.
- **Playback Controls**: Standard play, pause, stop, next, and previous track controls.
- **YouTube Downloader**: Integrated downloader using `yt-dlp` to fetch audio from YouTube links and save it as an MP3.
- **Cross-platform GUI**: Built with the fast and light FLTK toolkit.

## Dependencies

To build and run this project, you will need both build-time and run-time dependencies.

### Build-Time Dependencies

- A C++ compiler that supports C++17 (e.g., `g++` or `clang++`).
- `make`
- The **FLTK library (version 1.3 or higher)**. You can typically install this with your system's package manager.

```bash
# On Debian/Ubuntu
sudo apt-get update
sudo apt-get install build-essential libfltk1.3-dev

# On Fedora
sudo dnf install gcc-c++ make fltk-devel
```

### Run-Time Dependencies

The application relies on two external, statically-linked executables that must be placed in the project's root directory.

1.  **yt-dlp**: A powerful command-line program to download video/audio from YouTube and other sites.
    - Download the `yt-dlp_linux` binary from the [official releases page](https://github.com/yt-dlp/yt-dlp/releases).
    - Place it in the root of the project directory and name it `yt-dlp_linux`.

2.  **FFmpeg/FFplay**: A complete, cross-platform solution to record, convert and stream audio and video.
    - Download a **static GPL build** of FFmpeg for Linux from a source like [John Van Sickle](https://johnvansickle.com/ffmpeg/) or the [official site](https://ffmpeg.org/download.html).
    - Extract the archive and place the entire folder in the project root. The application expects the `ffplay` executable to be at the path `./ffmpeg-master-latest-linux64-gpl/bin/ffplay`.

Your final project directory structure should look like this:

```
.
├── ffmpeg-master-latest-linux64-gpl/
│   └── bin/
│       ├── ffmpeg
│       └── ffplay  <-- Required for playback
├── Makefile
├── main.cpp        <-- Your source code
└── yt-dlp_linux    <-- Required for downloading
```

## How to Build

1.  **Clone the repository:**

    ```bash
    git clone https://github.com/your-username/your-repo-name.git
    cd your-repo-name
    ```

2.  **Install dependencies:**
    Make sure you have installed the build-time dependencies (`g++`, `make`, `fltk-devel`) and placed the run-time dependencies (`yt-dlp_linux`, `ffmpeg` folder) in the correct locations as described above.

3.  **Compile the project:**
    Simply run the `make` command in the project's root directory.
    ```bash
    make
    ```
    This will create an executable file (e.g., `music-player`) in the same directory.

## How to Run

Execute the compiled binary from your terminal:

```bash
./music-player
```

You can also provide a starting directory as a command-line argument:

```bash
./music-player /path/to/my/music
```

## Roadmap & TODO

This is a list of planned features and improvements for the future. Contributions are welcome!

- [ ] **Song Deletion**: Add functionality to delete a selected song from the list (and optionally from the filesystem).
- [ ] **Playlist Management**: Implement a system to create, save, and load playlists.
- [ ] **Functional Progress Bar**: Make the progress slider actively track the current song's playback time and allow seeking.
- [ ] **Keyboard Shortcuts**: Add hotkeys for common actions (e.g., Spacebar for play/pause, arrow keys for next/previous).
- [ ] **UI Enhancements**:
  - [ ] Make the song list resizable and display more metadata (e.g., full titles, artist, album).
  - [ ] Implement double-click to play a song from the list.
- [ ] **Improved File Chooser**: Update the "Choose Folder" dialog to be more intuitive, possibly with file filtering.
