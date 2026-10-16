#ifndef REALMEMCARD_PS2_INTERFACE_H
#define REALMEMCARD_PS2_INTERFACE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// La interfaz de la capa baja de PS2 (source/lib/ps2/ps2_interface.h en el .mdebug de la A124): la tarjeta de memoria
// de libmc, con una tarea por operacion y el gestor de tareas como segunda base (en +0x1C). El layout sale de los stabs
// (3924 B). Las tareas viven en ps2_task.h, el mensaje en ps2_message.h y la calculadora en ps2_blockcalculator.h.

#include <libmc.h>

#include "../../../include/common/realmemcard/impl/interfaceimp.h"
#include "ps2_blockcalculator.h"
#include "ps2_message.h"
#include "ps2_task.h"

namespace Realmc {

extern const char *PARENT_DIRECTORY_NAME;
extern const char *CURRENT_DIRECTORY_NAME;
extern const char *ROOT_DIRECTORY_NAME;
extern const int MAX_FILES_PER_DIRECTORY;
extern const int PS2_CLUSTER_SIZE;
// El icon.sys y el fichero de relleno de cada directorio de partida (ps2_trctasks.cpp).
extern const char *gPs2IconSysFileName;

// El multitap se carga aparte (mtapman): el juego rellena estos punteros si lo quiere usar.
extern int (*gMCMTapInit)();
extern int (*gMCMTapPortOpen)(int);
extern int (*gMCMTapPortClose)(int);
extern int (*gMCMTapGetConnection)(int);
extern bool gMCMTapUsePort[2];

struct PS2Interface : public InterfaceImp, public TaskManager {
    static void *operator new(unsigned int size) {
        return AllocateMemSize(0, size, 0, 0, 0);
    }
    static void operator delete(void *ptr, unsigned int size) {
        FreeMemSize(ptr, size);
    }
    static void *operator new[](unsigned int size) {
        return AllocateMemSize(0, size, 0, 0, 0);
    }
    static void operator delete[](void *ptr, unsigned int size) {
        FreeMemSize(ptr, size);
    }
    static void *operator new(unsigned int, void *ptr) {
        return ptr;
    }
    static void operator delete(void *, void *) {}
    static void *operator new[](unsigned int, void *ptr) {
        return ptr;
    }
    static void operator delete[](void *, void *) {}

    PS2Interface(const SystemInterface &iSystem);
    virtual ~PS2Interface();
    virtual void TrcStartGame(const StartGameInfo &info);
    virtual void TrcCardExists(const CardID &cardID);
    virtual void TrcGetCardInfo(const CardID &cardID);
    virtual void TrcLoadFile(const CardID &cardID, const FileInfo &fileInfo);
    virtual void TrcSaveFile(const CardID &cardID, const FileInfo &fileInfo, SaveTaskType saveTaskType,
                             unsigned int nBlocksNeeded, unsigned int nFilesNeeded);
    virtual void TrcDeleteFile(const CardID &cardID, const FileInfo &fileInfo);
    virtual void TrcListFiles(const CardID &cardID, const FileInfo &fileInfo, ListTaskType listTaskType);
    virtual void CardExists(const CardID &cardID);
    virtual void GetCardInfo(const CardID &cardID);
    virtual void OpenFile(const CardID &cardID, const FileInfo &fileInfo, FileOpenMode mode);
    virtual void CloseFile(OpenFileDescriptor *fileHandle);
    virtual void DeleteFile(const CardID &cardID, const char *directoryName, const char *fileName);
    virtual void Read(OpenFileDescriptor *fileHandle, void *buffer, int bufsize);
    virtual void Write(OpenFileDescriptor *fileHandle, void *buffer, int bufsize);
    virtual void Seek(OpenFileDescriptor *fileHandle, int offset, SeekFrom from);
    virtual void Flush(OpenFileDescriptor *fileHandle);
    virtual void SetFileAttribute(const CardID &cardID, const char *directoryName, const char *fileName,
                                  FileAttribute attribute);
    virtual void FindFile(const CardID &cardID, const char *directoryName, const char *fileName);
    virtual void CreateDirectory(const CardID &cardID, const char *directoryName);
    virtual void DeleteDirectory(const CardID &cardID, const char *directoryName);
    virtual void Format(const CardID &cardID);
    virtual const Message *GetMessage(int elapsedTime);
    virtual void SendMessage(UserMessage msg, int param);
    // Un mensaje vacio (LMSG_NONE) para la siguiente consulta: _ProcessDelete de la capa alta espera asi la respuesta
    // del usuario (ps2_interface.h:84; sus dos stores en ps2_memcard_interface_impl de la A124).
    void SetDummyMessage() {
        this->mNewMsg = true;
        this->mMsg.mMsg = LMSG_NONE;
    }
    virtual bool IsBusy() {
        return this->GetCurrentTask() != nullptr;
    }
    virtual const unsigned short *GetCardName(const CardID &cardID);
    virtual Sjis *GetSjisInterface() {
        return &this->mSjis;
    }
    virtual unsigned int GetBlockSize(const CardID &) {
        return PS2_CLUSTER_SIZE;
    }
    virtual void *GetBlockCalculator() {
        return &this->mBlockCalculator;
    }
    void SetGameTitle(wchar_t *gameTitle) {
        this->mGameTitle = gameTitle;
    }
    void SetAutosaveEnabled(bool isAutosaveEnabled) {
        this->mIsAutosaveEnabled = isAutosaveEnabled;
    }
    void SetBootFlow(bool isBootFlow) {
        this->mIsBootFlow = isBootFlow;
    }
    virtual void ClearTask();
    virtual void UpdateCurrentTask();

  private:
    CardStatus SceResultToStatus(int sceFunction, int sceResult);
    char *PortStr(CardID cardID);
    char *PortStrAscii(CardID cardID);
    void CheckName(const char *str);
    void CheckFileInfo(const FileInfo *fileInfo);
    void CheckIopModule(char *moduleName);
    void CheckSceResult(int sceFunctionResult);
    bool CheckSyncResult();
    CardStatus CheckCard(CardID cardID, int *type, int *freeSpace, int *format);
    bool MultitapConnected(int port);
    bool CheckCurrentTask(const char *taskName);
    void SetFileTime(FileTime *fileTime, sceMcTblGetDir *entry);
    Ps2FileDescriptor *SceOpenFile(const CardID &cardID, const char *path, int mode);
    void SceCloseFile(Ps2FileDescriptor *fileDescriptor);
    Ps2FileDescriptor *FindFreeFileDescriptor();
    bool GetNextCardId(CardID &cardID, bool checkAllSlots);
    void UpdateTaskTrcStartGame();
    void UpdateTaskTrcGetCardInfo();
    void UpdateTaskTrcList();
    void UpdateTaskTrcListEntries();
    void UpdateTaskTrcListFiles();
    void UpdateTaskTrcSaveCheck();
    void UpdateTaskTrcSaveFile();
    void UpdateTaskTrcLoadFile();
    void UpdateTaskTrcDeleteFile();
    void UpdateTaskStartGameCommon();
    void UpdateTaskGetCardInfo();
    void UpdateTaskCheckSpace();
    void UpdateTaskSaveFile();
    void UpdateTaskFind();
    void UpdateTaskWrite();
    void UpdateTaskRead();
    void UpdateTaskCreateDirectory();
    void UpdateTaskChangeDirectory();
    void UpdateTaskOpen();
    void UpdateTaskClose();
    void UpdateTaskFlush();
    void UpdateTaskSeek();
    void UpdateTaskSetAttribute();
    void UpdateTaskDelete();
    void UpdateTaskFormat();
    void UpdateTaskCheckDirectory();

    TaskTrcStartGame mTaskTrcStartGame;
    TaskTrcGetCardInfo mTaskTrcGetCardInfo;
    TaskTrcSaveFile mTaskTrcSaveFile;
    TaskTrcLoadFile mTaskTrcLoadFile;
    TaskTrcDeleteFile mTaskTrcDeleteFile;
    TaskTrcList mTaskTrcList;
    TaskTrcListEntries mTaskTrcListEntries;
    TaskTrcListFiles mTaskTrcListFiles;
    TaskStartGameCommon mTaskStartGameCommon;
    TaskGetCardInfo mTaskGetCardInfo;
    TaskCreateDirectory mTaskCreateDirectory;
    TaskChangeDirectory mTaskChangeDirectory;
    TaskOpen mTaskOpen;
    TaskClose mTaskClose;
    TaskDelete mTaskDelete;
    TaskSeek mTaskSeek;
    TaskRead mTaskRead;
    TaskWrite mTaskWrite;
    TaskFlush mTaskFlush;
    TaskSetAttribute mTaskSetAttribute;
    TaskFind mTaskFind;
    TaskFormat mTaskFormat;
    TaskCheckSpace mTaskCheckSpace;
    TaskSaveFile mTaskSaveFile;
    TaskCheckDirectory mTaskCheckDirectory;
    static sceMcTblGetDir mPs2FileTableEntry[7];
    Ps2FileDescriptor mFileDescriptors[3];
    Ps2FileDescriptor *mCurrentFileDescriptor;
    Sjis mSjis;
    UserMessage mUserMsg;
    PS2Message mMsg;
    bool mNewMsg;
    PS2Message mResultMsg;
    BlockCalculatorImp mBlockCalculator;
    MessageTimer mMsgTimer;
    int mSceAsyncResult;
    CardStatus mCardStatus;
    int mNumFilesFound;
    bool mNoCheckingCardWarning;
    wchar_t *mGameTitle;
    bool mIsAutosaveEnabled;
    bool mIsBootFlow;
};

} // namespace Realmc

#endif
