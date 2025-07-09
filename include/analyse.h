#pragma once
#include<libtsk.h>
#ifdef __cplusplus
extern "C" {
#endif
#include<stdbool.h>
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
	extern volatile uint64_t total_file_sys;
	extern volatile uint64_t file_sys_analyzed;
	extern volatile bool done;
	typedef struct fileNode {
		char *name;
		ssize_t size;
		struct fileNode* parent;
		struct fileNode** children;
		uint32_t len; //No.of children
		uint32_t capacity; // capacity of children
		TSK_INUM_T inum;
		bool isDir;
		uint8_t deleted;
		char* recycledPath;
	} fileNode_t;
	typedef struct fileSystem {
		fileNode_t* root;
		TSK_FS_INFO* fs;
	}fileSystem_t;
	typedef struct volumeSystem {
		TSK_VS_INFO* vs;
		uint16_t* validFs;
		uint16_t size;
		fileSystem_t** fileSystems;
	}volumeSystem_t;

	void analyseImg(const char* a_img,
		TSK_IMG_TYPE_ENUM type,
		unsigned int a_ssize, volumeSystem_t** ret);
	fileSystem_t* analyseFileSystem(TSK_FS_INFO* fs);
	fileNode_t* findFileByUtf8Path(fileNode_t* root, const char* path);
	void freeFileSystem(fileSystem_t* fileSys);
	void freeVolumeSystem(volumeSystem_t* volSys);
	void freeFileNode(fileNode_t* fN);
	void printFileSys(const fileSystem_t* fn);
	fileNode_t* fileNodebyInum(TSK_INUM_T, fileNode_t* root);
	TSK_WALK_RET_ENUM initbitmap(const TSK_FS_BLOCK*
		a_block, void* a_ptr);
#ifdef __cplusplus
}
#endif
