#ifndef REALMEMCARD_PS2_BLOCKCALCULATOR_H
#define REALMEMCARD_PS2_BLOCKCALCULATOR_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// La calculadora de bloques de PS2 (source/lib/ps2/ps2_blockcalculator.h en el .mdebug de la A124): cuenta los
// clusteres de 1 KB de la tabla del directorio, de los iconos y del fichero de relleno, y los de cada fichero de datos.
// La interfaz base es la de PS2 (SetDirectoryInfo y AddDataFile), no la de GameCube (SetFileInfo).

#include "../../../include/common/realmemcard/impl/memcard_interface_impl.h"

namespace Realmc {

extern const int NUM_CLUSTERS_PER_DIRECTORY_TABLE;
extern int gPs2DummyFileSize;

struct BlockCalculator {
    BlockCalculator() {}
    virtual void Clear() = 0;
    virtual unsigned int GetResult() = 0;
    virtual void SetDirectoryInfo(const CardID &cardID, const DirectoryInfo &directoryInfo) = 0;
    virtual void AddDataFile(const char *filename, unsigned int size) = 0;

  protected:
    virtual ~BlockCalculator() {}
};

struct BlockCalculatorImp : public BlockCalculator {
    BlockCalculatorImp();
    virtual ~BlockCalculatorImp() {}
    virtual void Clear();
    virtual void SetDirectoryInfo(const CardID &cardID, const DirectoryInfo &directoryInfo);
    virtual void AddDataFile(const char *filename, unsigned int size);
    virtual unsigned int GetResult();

    void SetBlockSize(unsigned int blockSize) {
        this->mBlockSize = blockSize;
    }

    unsigned int CalculateBlockSize(const CardID &cardID, const FileInfo &fileInfo);

    unsigned int CalculateBlockSize(unsigned int bytes) {
        return this->GetBlockCount(bytes);
    }

  private:
    unsigned int GetBlockCount(unsigned int bytes) {
        return (bytes + this->mBlockSize - 1) / this->mBlockSize;
    }

    CardID mCardID;
    DirectoryInfo mDirectoryInfo;
    unsigned int mBlockSize;
    unsigned int mBlockCount;
    unsigned int mFileCount;
    unsigned int mDefaultDummyFileBlockSize;
};

} // namespace Realmc

#endif
