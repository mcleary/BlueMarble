#pragma once

#include <set>
#include <thread>
#include <mutex>
#include <filesystem>

class FDirectoryWatcher
{
public:

    FDirectoryWatcher(const std::filesystem::path& InDirToWatch);
    ~FDirectoryWatcher();

    std::set<std::filesystem::path> GetChangedFiles();

private:

    std::filesystem::path DirToWatch;
    void* DirHandle;
    std::thread WorkerThread;
    std::atomic<bool> bIsWaiting{ false };
    std::atomic<bool> bShouldWatch{ true };

    std::mutex FilesChangedMutex;
    std::set<std::filesystem::path> FilesChanged;
};
