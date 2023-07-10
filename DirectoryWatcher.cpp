#include "DirectoryWatcher.h"

#include <iostream>
#include <filesystem>

#include <Windows.h>

FDirectoryWatcher::FDirectoryWatcher(const std::string& InDirToWatch)
    : DirToWatch{ InDirToWatch }
{
    DirHandle = CreateFile(InDirToWatch.c_str(),
                           FILE_LIST_DIRECTORY,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS, NULL);

    WorkerThread = std::thread([&]
    {
        DWORD BytesReturned;
        FILE_NOTIFY_INFORMATION Buffer[1024];
        while (bShouldWatch)
        {
            bIsWaiting = true;
            if (ReadDirectoryChangesW(DirHandle, Buffer, sizeof(Buffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, &BytesReturned, NULL, NULL) == TRUE)
            {
                FILE_NOTIFY_INFORMATION* Info = &Buffer[0];
                do
                {
                    Info = (FILE_NOTIFY_INFORMATION*) ((std::uint8_t*)&Buffer[0]) + Info->NextEntryOffset;

                    switch (Info->Action)
                    {
                        case FILE_ACTION_MODIFIED:
                        {
                            std::wstring FileNameW{ Info->FileName, Info->FileNameLength / sizeof(WCHAR) };
                            std::filesystem::path FilePath{ FileNameW };

                            {
                                std::lock_guard Lock{ FilesChangedMutex };
                                FilesChanged.insert((std::filesystem::path{ DirToWatch } / FilePath).string());
                            }

                            break;
                        }
                    }
                }
                while (Info->NextEntryOffset != 0);
            }
            bIsWaiting = false;
        }
    });
}

FDirectoryWatcher::~FDirectoryWatcher()
{
    bShouldWatch = false;
    while (bIsWaiting)
    {
        CancelIoEx(DirHandle, NULL);
    }
    WorkerThread.join();
}

std::set<std::string> FDirectoryWatcher::GetChangedFiles()
{
    std::lock_guard Lock{ FilesChangedMutex };
    std::set<std::string> ChangedFiles = FilesChanged;
    FilesChanged.clear();
    return ChangedFiles;
}