#include "DirectoryWatcher.h"

#include <Windows.h>

FDirectoryWatcher::FDirectoryWatcher(const std::filesystem::path& InDirToWatch)
    : DirToWatch{ std::filesystem::absolute(std::filesystem::current_path() / InDirToWatch) }
{
    DirHandle = CreateFile(DirToWatch.string().c_str(),
                           FILE_LIST_DIRECTORY,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS, NULL);

    WorkerThread = std::thread([&]
    {
        DWORD BytesReturned;
        std::uint8_t Buffer[1024];
        while (bShouldWatch)
        {
            bIsWaiting = true;
            if (ReadDirectoryChangesW(DirHandle, Buffer, sizeof(Buffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, &BytesReturned, NULL, NULL) == TRUE)
            {
                std::uint32_t Offset = 0;
                do
                {
                    FILE_NOTIFY_INFORMATION* Info = (FILE_NOTIFY_INFORMATION*) &Buffer[Offset];

                    switch (Info->Action)
                    {
                        case FILE_ACTION_MODIFIED:
                        {
                            std::wstring FileNameW{ Info->FileName, Info->FileNameLength / sizeof(WCHAR) };
                            std::filesystem::path FilePath{ FileNameW };

                            {
                                std::lock_guard Lock{ FilesChangedMutex };
                                FilesChanged.insert((std::filesystem::current_path() / std::filesystem::path{ DirToWatch } / FilePath));
                            }

                            break;
                        }
                    }

                    Offset = Info->NextEntryOffset;
                }
                while (Offset != 0);
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