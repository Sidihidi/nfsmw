#include <string.h>

#include "ps2_blockcalculator.h"

namespace Realmc {

BlockCalculatorImp::BlockCalculatorImp() {
    this->mBlockSize = 0;
    this->Clear();
}

void BlockCalculatorImp::Clear() {
    this->mBlockCount = 0;
    this->mFileCount = 0;
    this->mDefaultDummyFileBlockSize = 0;
}

// El directorio ocupa su tabla, el icon.sys, los tres iconos (si los hay) y el fichero de relleno.
void BlockCalculatorImp::SetDirectoryInfo(const CardID &cardID, const DirectoryInfo &directoryInfo) {
    this->mCardID = cardID;
    this->mDirectoryInfo = directoryInfo;

    this->mBlockCount += NUM_CLUSTERS_PER_DIRECTORY_TABLE;

    if (directoryInfo.iconSysDataSize > 0) {
        this->mFileCount++;
        this->mBlockCount += this->GetBlockCount(directoryInfo.iconSysDataSize);
    }

    if (directoryInfo.staticIconDataSize > 0) {
        this->mFileCount++;
        this->mBlockCount += this->GetBlockCount(directoryInfo.staticIconDataSize);
    }

    if (directoryInfo.copyIconDataSize > 0) {
        this->mFileCount++;
        this->mBlockCount += this->GetBlockCount(directoryInfo.copyIconDataSize);
    }

    if (directoryInfo.deleteIconDataSize > 0) {
        this->mFileCount++;
        this->mBlockCount += this->GetBlockCount(directoryInfo.deleteIconDataSize);
    }

    this->mFileCount++;
    this->mDefaultDummyFileBlockSize = this->GetBlockCount(gPs2DummyFileSize);
    this->mBlockCount += this->mDefaultDummyFileBlockSize;
}

// Un fichero de datos con el nombre del directorio sustituye al de relleno.
void BlockCalculatorImp::AddDataFile(const char *filename, unsigned int size) {
    this->mBlockCount += this->GetBlockCount(size);
    this->mFileCount++;

    if (strcasecmp(filename, this->mDirectoryInfo.directoryName) == 0) {
        this->mFileCount--;
        this->mBlockCount -= this->mDefaultDummyFileBlockSize;
        this->mDefaultDummyFileBlockSize = 0;
    }
}

unsigned int BlockCalculatorImp::CalculateBlockSize(const CardID &cardID, const FileInfo &fileInfo) {
    this->Clear();

    if (fileInfo.directoryInfo != nullptr) {
        this->SetDirectoryInfo(cardID, *fileInfo.directoryInfo);
    }

    this->AddDataFile(fileInfo.fileName, fileInfo.fileByteSize);

    return this->GetResult();
}

// Cada entrada de directorio ocupa media unidad: dos ficheros por cluster.
unsigned int BlockCalculatorImp::GetResult() {
    return this->mBlockCount + (this->mFileCount + 1) / 2;
}

} // namespace Realmc
