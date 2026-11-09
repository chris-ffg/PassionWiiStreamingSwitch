// ============================================================================
// DangoPlayer Switch — File Utilities Implementation
// ============================================================================

#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include "utils/logger.h"
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <cstring>

namespace FileUtils {

static const std::vector<std::string> VIDEO_EXTS = {
    ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv",
    ".webm", ".ts", ".m2ts", ".vob", ".3gp", ".m4v", ".mpg", ".mpeg"
};

static const std::vector<std::string> AUDIO_EXTS = {
    ".mp3", ".flac", ".wav", ".aac", ".ogg", ".m4a", ".opus", ".wma"
};

static const std::vector<std::string> PLAYLIST_EXTS = {
    ".m3u", ".m3u8", ".pls"
};

bool Exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool IsDirectory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return false;
}

uint64_t GetFileSize(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return (uint64_t)st.st_size;
    }
    return 0;
}

bool IsVideoFile(const std::string& path) {
    std::string ext = StringUtils::GetExtension(path);
    return std::find(VIDEO_EXTS.begin(), VIDEO_EXTS.end(), ext) != VIDEO_EXTS.end();
}

bool IsAudioFile(const std::string& path) {
    std::string ext = StringUtils::GetExtension(path);
    return std::find(AUDIO_EXTS.begin(), AUDIO_EXTS.end(), ext) != AUDIO_EXTS.end();
}

bool IsPlaylistFile(const std::string& path) {
    std::string ext = StringUtils::GetExtension(path);
    return std::find(PLAYLIST_EXTS.begin(), PLAYLIST_EXTS.end(), ext) != PLAYLIST_EXTS.end();
}

FileType GetFileType(const std::string& filename) {
    if (IsVideoFile(filename)) return FileType::Video;
    if (IsAudioFile(filename)) return FileType::Audio;
    if (IsPlaylistFile(filename)) return FileType::Playlist;
    return FileType::Other;
}

std::vector<FileItem> ScanDirectory(const std::string& path, FileFilter filter) {
    std::vector<FileItem> dirs;
    std::vector<FileItem> files;

    DIR* dir = opendir(path.c_str());
    if (!dir) {
        Logger::Warn("Cannot open directory: %s", path.c_str());
        return {};
    }

    // Add parent directory item if not at root
    bool isRoot = (path == "sdmc:/" || path == "sdmc:" || path == "/");
    if (!isRoot) {
        FileItem parent;
        parent.name = ".. (Carpeta anterior)";
        parent.fullPath = StringUtils::GetParentDirectory(path);
        parent.type = FileType::ParentDir;
        parent.isDirectory = true;
        dirs.push_back(parent);
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Skip hidden files/directories and "." / ".."
        if (entry->d_name[0] == '.') {
            continue;
        }

        std::string fullPath = path;
        if (fullPath.back() != '/' && fullPath.back() != '\\') {
            fullPath += "/";
        }
        fullPath += entry->d_name;

        struct stat st;
        if (stat(fullPath.c_str(), &st) != 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            FileItem item;
            item.name = entry->d_name;
            item.fullPath = fullPath;
            item.type = FileType::Directory;
            item.isDirectory = true;
            item.size = 0;
            dirs.push_back(item);
        } else {
            FileType type = GetFileType(entry->d_name);

            // Apply filter
            if (filter == FileFilter::MediaOnly) {
                if (type != FileType::Video && type != FileType::Audio && type != FileType::Playlist) {
                    continue;
                }
            } else if (filter == FileFilter::VideoOnly) {
                if (type != FileType::Video) continue;
            } else if (filter == FileFilter::PlaylistOnly) {
                if (type != FileType::Playlist) continue;
            }

            FileItem item;
            item.name = entry->d_name;
            item.fullPath = fullPath;
            item.type = type;
            item.isDirectory = false;
            item.size = (uint64_t)st.st_size;
            files.push_back(item);
        }
    }
    closedir(dir);

    // Sort folders alphabetically (case-insensitive, preserving ".." at top)
    auto sortAlpha = [](const FileItem& a, const FileItem& b) {
        if (a.type == FileType::ParentDir) return true;
        if (b.type == FileType::ParentDir) return false;
        return StringUtils::ToLower(a.name) < StringUtils::ToLower(b.name);
    };

    std::sort(dirs.begin(), dirs.end(), sortAlpha);
    std::sort(files.begin(), files.end(), sortAlpha);

    // Merge: directories first, then files
    std::vector<FileItem> result;
    result.reserve(dirs.size() + files.size());
    result.insert(result.end(), dirs.begin(), dirs.end());
    result.insert(result.end(), files.begin(), files.end());

    return result;
}

}  // namespace FileUtils
