#pragma once

#include <set>
#include <string>
#include <thread>
#include <mutex>

class FDirectoryWatcher
{
public:

    FDirectoryWatcher(const std::string& InDirToWatch);
    ~FDirectoryWatcher();

    std::set<std::string> GetChangedFiles();

private:

    std::string DirToWatch;
    void* DirHandle;
    std::thread WorkerThread;
    std::atomic<bool> bIsWaiting{ false };
    std::atomic<bool> bShouldWatch{ true };

    std::mutex FilesChangedMutex;
    std::set<std::string> FilesChanged;
};
