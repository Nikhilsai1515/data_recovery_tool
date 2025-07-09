#pragma once
#include"analyse.h"
#include<libtsk.h>
#ifdef __cplusplus
extern "C" {
#endif
void recover(const TSK_INUM_T inum, const fileSystem_t* fs, const TSK_TCHAR* path);
void recoverSingleFile(const TSK_INUM_T inum, const fileSystem_t* fs, const TSK_TCHAR* path);
void recoverDirectory(const TSK_INUM_T inum, const fileSystem_t* fs, const TSK_TCHAR* path);
#ifdef __cplusplus
}
#endif