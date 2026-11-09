// ============================================================================
// DangoPlayer Switch — File Utilities
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <cstdint>

enum class FileType {
    Directory,
    ParentDir,
    Video,
    Audio,
    Playlist,
    Other
};

enum class FileFilter {
    All,
    MediaOnly,      // Video + Audio + Playlist
    VideoOnly,
    PlaylistOnly
};

struct FileItem {
    std::string name;
    std::string fullPath;
    FileType type = FileType::Other;
    uint64_t size = 0;
    bool isDirectory = false;
};

namespace FileUtils {
    /// Check if path exists.
    bool Exists(const std::string& path);

    /// Check if path is a directory.
    bool IsDirectory(const std::string& path);

    /// Get file size in bytes.
    uint64_t GetFileSize(const std::string& path);

    /// Classify a file extension into FileType.
    FileType GetFileType(const std::string& filename);

    /// Scan a directory and return sorted entries (folders first, then alphabetical).
    std::vector<FileItem> ScanDirectory(const std::string& path, FileFilter filter = FileFilter::All);

    /// Check if an extension is a supported video format.
    bool IsVideoFile(const std::string& path);

    /// Check if an extension is a supported audio format.
    bool IsAudioFile(const std::string& path);

    /// Check if an extension is a supported playlist format.
    bool IsPlaylistFile(const std::string& path);
}
