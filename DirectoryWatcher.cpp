#include "DirectoryWatcher.h"

#include <Windows.h>

#include <iostream>
#include <array>

FDirectoryWatcher::FDirectoryWatcher(const std::filesystem::path& InDirToWatch)
    : DirToWatch{ std::filesystem::absolute(InDirToWatch) }
{
    DirHandle = CreateFile(DirToWatch.string().c_str(),
                           FILE_LIST_DIRECTORY,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS, NULL);

    WorkerThread = std::thread([&]
    {
        std::vector<UCHAR> Buffer(64 * 1024, 0);

        DWORD BytesReturned;
        while (bShouldWatch)
        {
            bIsWaiting = true;
            constexpr DWORD NotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;
            if (ReadDirectoryChangesW(DirHandle, Buffer.data(), Buffer.size() * sizeof(UCHAR), FALSE, NotifyFilter, &BytesReturned, NULL, NULL) == TRUE)
            {
                DWORD Offset = 0;
                FILE_NOTIFY_INFORMATION* Info;
                do
                {
                    Info = (FILE_NOTIFY_INFORMATION*) &Buffer[Offset];

                    #if 0
                    switch (Info->Action)
                    {
                        case FILE_ACTION_ADDED:
                            std::cout << "Add: ";
                            break;

                        case FILE_ACTION_MODIFIED:
                            std::cout << "Modified: ";
                            break;

                        case FILE_ACTION_REMOVED:
                            std::cout << "Removed: ";
                            break;

                        case FILE_ACTION_RENAMED_NEW_NAME:
                            std::cout << "Renamed New Name: ";
                            break;

                        case FILE_ACTION_RENAMED_OLD_NAME:
                            std::cout << "Renamed Old Name: ";
                            break;
                    }

                    std::wstring FileNameW{ Info->FileName, Info->FileNameLength / sizeof(WCHAR) };
                    std::filesystem::path FilePath{ FileNameW };

                    std::cout << FilePath << std::endl;
                    #endif

                    switch (Info->Action)
                    {
                        case FILE_ACTION_MODIFIED:
                        case FILE_ACTION_RENAMED_NEW_NAME:
                        {
                            const std::wstring FileNameW{ Info->FileName, Info->FileNameLength / sizeof(WCHAR) };
                            const std::filesystem::path FilePath{ FileNameW };

                            {
                                std::lock_guard Lock{ FilesChangedMutex };
                                FilesChanged.insert(DirToWatch / FilePath);
                            }

                            break;
                        }
                    }

                    Offset += Info->NextEntryOffset;
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
    CloseHandle(DirHandle);
}

std::set<std::filesystem::path> FDirectoryWatcher::GetChangedFiles()
{
    std::lock_guard Lock{ FilesChangedMutex };
    std::set<std::filesystem::path> ChangedFiles = FilesChanged;
    FilesChanged.clear();
    return ChangedFiles;
}