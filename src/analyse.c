#include"analyse.h"
volatile uint64_t total_file_sys = 0;
volatile uint64_t file_sys_analyzed = 0;
volatile bool done = false;
static bool* inodeTable = NULL;
void analyseImg(const char* a_img,
	TSK_IMG_TYPE_ENUM type,
	unsigned int a_ssize,volumeSystem_t** vol) {
	TSK_TCHAR a_image[MAX_PATH] = { 0 };
	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, a_img, -1, NULL, 0);
	if (sizeNeeded > MAX_PATH) {
		return ;
	}
	MultiByteToWideChar(CP_UTF8, 0, a_img, -1, a_image, sizeNeeded);
	TSK_IMG_INFO* img = tsk_img_open_sing(a_image, type, a_ssize);
	if (!img) {
		fprintf(stderr, "Failed trying to open image:\n %s", tsk_error_get());
		return ;
	}

	TSK_VS_INFO* vs = tsk_vs_open(img, 0, TSK_VS_TYPE_DETECT);
	if (!vs) {
		fprintf(stderr, "Failed trying to analyse volumes:\n %s", tsk_error_get());
		return ;
	}

	volumeSystem_t* ret = malloc(sizeof(volumeSystem_t));
	if (!ret) {
		fprintf(stderr, "Not enough memory! \n");
		return ;
	}
	ret->validFs = malloc(sizeof(uint16_t) * vs->part_count);
	if (!ret->validFs) {
		fprintf(stderr, "Not enough memory! \n");
		return ;
	}

	ret->fileSystems = malloc(sizeof(fileSystem_t*) * vs->part_count);
	if (!ret->fileSystems) {
		fprintf(stderr, "Not enough memory! \n");
		return ;
	}
	ret->size = 0;
	uint16_t top = -1;
	for (TSK_VS_PART_INFO* part = vs->part_list; part != NULL; part = part->next) {
		TSK_FS_INFO* fs = tsk_fs_open_img(img, part->start * img->sector_size, TSK_FS_TYPE_DETECT);

		if (fs != NULL) {
			total_file_sys ++;
			ret->validFs[++top] = 1;
		}
		else {
			ret->validFs[++top] = 0;
			ret->fileSystems[top] = NULL;
		}
			ret->size++;
	}
		top = -1;
		for (TSK_VS_PART_INFO* part = vs->part_list; part != NULL; part = part->next) {
			TSK_FS_INFO* fs = tsk_fs_open_img(img, part->start * img->sector_size, TSK_FS_TYPE_DETECT);

			if (ret->validFs[++top]) {
				ret->fileSystems[top] = analyseFileSystem(fs);
				file_sys_analyzed++;
			}
		}
		ret->vs = vs;
		*vol = ret;
		done = true;

}
static TSK_WALK_RET_ENUM countblocks (TSK_FS_FILE*
	a_fs_file, TSK_OFF_T a_off, TSK_DADDR_T a_addr, char* a_buf,
	size_t a_len, TSK_FS_BLOCK_FLAG_ENUM a_flags, void* a_ptr) {
	uint64_t* block = a_ptr;
	if (a_addr == 0) {
		return TSK_WALK_CONT;
	}
	if (a_flags & TSK_FS_BLOCK_FLAG_ALLOC) {
		block[1]++;
	}
	else if(a_flags & TSK_FS_BLOCK_FLAG_UNALLOC) {
		block[0]++;
	}
	return TSK_WALK_CONT;
}
static TSK_WALK_RET_ENUM add_children(TSK_FS_FILE* fs_file, const char* path, void* par) {
	if (par == NULL) {
		fprintf(stderr, "NULL pointer argument!\n");
		return TSK_WALK_ERROR;
	}
	fileNode_t* parent = par;
	if (!fs_file ||!fs_file->meta|| !fs_file->name || !fs_file->name->name)
		return TSK_WALK_CONT;

	// Ignore "." and ".."
	if (strcmp(fs_file->name->name, ".") == 0 || strcmp(fs_file->name->name, "..") == 0)
		return TSK_WALK_CONT;
	if((fs_file->meta)&&(fs_file->meta->type==TSK_FS_META_TYPE_LNK))
		return TSK_WALK_CONT;
	// Allocate and set up a new file node
	fileNode_t* node = calloc(1, sizeof(fileNode_t));
	if (!node) {
		fprintf(stderr, "Not enough memory!\n");
		return TSK_WALK_ERROR;
	}
	node->name = (char*)calloc(sizeof(char), fs_file->name->name_size);
	if (node->name == NULL){
		fprintf(stderr, "Not enough memory!\n");
		return TSK_WALK_ERROR;
	}
	memcpy(node->name,fs_file->name->name,fs_file->name->name_size);

	// Set metadata
	node->size = fs_file->meta->size;
	node->inum = fs_file->meta->addr;
	node->isDir = (fs_file->meta->type == TSK_FS_META_TYPE_DIR);
	node->deleted = !(fs_file->meta->flags & TSK_FS_META_FLAG_ALLOC) ||
		(parent && parent->deleted);
	node->parent = parent;

	if (!node->isDir && node->deleted) {
		double block[2] = {0,0};
		tsk_fs_file_walk(fs_file, TSK_FS_FILE_WALK_FLAG_AONLY | TSK_FS_FILE_WALK_FLAG_NOSPARSE, countblocks, block);
		if ((block[0] + block[1]) != 0) {
			node->deleted = (uint8_t)((block[1] / (block[0] + block[1]))*100 + 1);
		}
		block[0] = 0;block[1] = 0;
	}
	// Add to parent's children
	if (parent->capacity <= parent->len) {
		parent->capacity = parent->capacity ? parent->capacity * 2 : 2;
		fileNode_t** temp = realloc(parent->children, parent->capacity * sizeof(fileNode_t*));
		if (!temp) {
			fprintf(stderr, "Not enough memory for children array\n");
			free(node->name);
			free(node);
			return TSK_WALK_ERROR;
		}
		parent->children = temp;
	}

	parent->children[parent->len++] = node;

	// If directory, recurse
	if (node->isDir) {
		node->capacity = 2;
		node->children = malloc(node->capacity * sizeof(fileNode_t*));
		if (!node->children) {
			fprintf(stderr, "Not enough memory for directory children\n");
			return TSK_WALK_ERROR;
		}
		if (inodeTable[node->inum]) {
			return TSK_WALK_CONT;
		}
		inodeTable[node->inum] = true;
		if (tsk_fs_dir_walk(fs_file->fs_info, fs_file->meta->addr,
			TSK_FS_DIR_WALK_FLAG_ALLOC | TSK_FS_DIR_WALK_FLAG_UNALLOC,
			add_children, node) != 0) {
			fprintf(stderr, "Error walking directory: %s\n", tsk_error_get());
			return TSK_WALK_ERROR;
		}
	}

	return TSK_WALK_CONT;
}
void static processRecycleBinDir(fileNode_t* dir, const TSK_FS_INFO* fs) {
	if (!dir || !dir->isDir) return;

	for (size_t i = 0; i < dir->len; i++) {
		fileNode_t* node = dir->children[i];
		if (!node || !node->name) continue;

		// Recurse into subdirectories (for SID folders)
		if (node->isDir) {
			processRecycleBinDir(node, fs);
			// No 'continue' here because directories may also be $R* in FAT or NTFS
		}

		// Check for $R* or _R* names (NTFS & FAT)
		if ((node->name[0] == '$' || node->name[0] == '_') && node->name[1] == 'R') {
			const char* rSuffix = node->name + 2; // Skip $R or _R prefix

			// Find matching $I or _I sibling in the same directory
			for (size_t j = 0; j < dir->len; j++) {
				fileNode_t* sibling = dir->children[j];
				if (!sibling || !sibling->name) continue;

				if ((sibling->name[0] == '$' || sibling->name[0] == '_') &&
					sibling->name[1] == 'I' &&
					strcmp(sibling->name + 2, rSuffix) == 0) {

					// Found matching $I or _I file
					TSK_FS_FILE* iFile = tsk_fs_file_open_meta(fs, NULL, sibling->inum);
					if (!iFile || !iFile->meta || iFile->meta->size < 28) {
						if (iFile) tsk_fs_file_close(iFile);
						break;
					}

					uint8_t* buffer = (uint8_t*)malloc(iFile->meta->size);
					if (!buffer) {
						tsk_fs_file_close(iFile);
						break;
					}

					ssize_t bytesRead = tsk_fs_file_read(iFile, 0, buffer, iFile->meta->size, TSK_FS_FILE_READ_FLAG_NONE);
					if (bytesRead < 28) { // Must be at least 28 bytes to contain path
						free(buffer);
						tsk_fs_file_close(iFile);
						break;
					}

					// Original path is at offset 28 in Windows 10+ ($I file format)
					wchar_t* originalPathW = (wchar_t*)(buffer + 28);

					// Inline UTF-16 to UTF-8 conversion using Win32 API
					int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, originalPathW, -1, NULL, 0, NULL, NULL);
					if (sizeNeeded > 0) {
						char* utf8Path = (char*)malloc(sizeNeeded);
						if (utf8Path) {
							WideCharToMultiByte(CP_UTF8, 0, originalPathW, -1, utf8Path, sizeNeeded, NULL, NULL);
							if (node->recycledPath) free(node->recycledPath);
							node->recycledPath = utf8Path;
						}
					}

					free(buffer);
					tsk_fs_file_close(iFile);
					break; // Done with this $R file
				}
			}
		}
	}
}
fileSystem_t* analyseFileSystem(TSK_FS_INFO* fs) {
	if (fs->ftype == TSK_FS_TYPE_UNSUPP) {
		MessageBox(NULL, L"File System Type Not Supported", L"ERROR", MB_ICONERROR | MB_OK);
		exit(1);
	}

	if (fs == NULL) {
		return NULL;
	}

	fileSystem_t* fileSys = calloc(1,sizeof(fileSystem_t));
	if (fileSys == NULL) {
		fprintf(stderr, "Not enough memmory!\n");
		return NULL;
	}
	fileSys->root = calloc(1, sizeof(fileNode_t));
	if (fileSys->root == NULL) {
		fprintf(stderr, "Not Enough memory\n");
		free(fileSys);
		return NULL;
	}
	fileSys->root->inum = fs->root_inum;
	fileSys->root->name = _strdup("/");
	fileSys->root->parent = NULL;
	fileSys->root->capacity = 4;
	fileSys->root->children = malloc(sizeof(fileNode_t*) * fileSys->root->capacity);
	fileSys->root->len = 0;
	fileSys->root->deleted = false;
	fileSys->root->isDir = true;
	inodeTable = calloc(fs->last_inum, sizeof(bool));
	if (tsk_fs_dir_walk(fs, fs->root_inum,
		TSK_FS_DIR_WALK_FLAG_ALLOC | TSK_FS_DIR_WALK_FLAG_UNALLOC,
		add_children, fileSys->root) != 0) {
		fprintf(stderr, "Error walking file system: %s\n", tsk_error_get());
		freeFileSystem(fileSys);
		return NULL;
	}
	free(inodeTable);

	fileNode_t* recyclebin=findFileByUtf8Path(fileSys->root,"/$RECYCLE.BIN");
	if (recyclebin == NULL) {
		recyclebin = findFileByUtf8Path(fileSys->root,"/$Recycle.Bin");
	}
	if (recyclebin) {
		processRecycleBinDir(recyclebin,fs);
	}
	fileSys->fs = fs;
	return fileSys;
}


void freeFileNode(fileNode_t* node) {
	if (!node) return;

	for (uint32_t i = 0; i < node->len; i++) {
		if (node->children[i]) {
			freeFileNode(node->children[i]);
		}
	}
	if (node->recycledPath) {
		free(node->recycledPath);
	}
	free(node->name);
	free(node->children);
	free(node);
}

void freeFileSystem(fileSystem_t* fileSys) {
	if (!fileSys) return;
	tsk_fs_close(fileSys->fs);

	freeFileNode(fileSys->root);
	free(fileSys);
}

void freeVolumeSystem(volumeSystem_t* volSys) {
	if (!volSys) return;

	for (int i = 0; i < volSys->size; i++) {

		freeFileSystem(volSys->fileSystems[i]);
	}

	free(volSys->vs);
	free(volSys->validFs);
	free(volSys->fileSystems);
	free(volSys);
}

fileNode_t* findFileByUtf8Path(fileNode_t* root, const char* utf8Path) {
	if (!root || !utf8Path) return NULL;

	// Make a writable copy of the path
	char* working = _strdup(utf8Path);
	if (!working) return NULL;

	// We'll split on both '/' and '\' (both are ASCII, so safe in UTF‑8)
	const char delimiters[] = "/\\";
	char* token = strtok(working, delimiters);
	fileNode_t* current = root;

	while (token) {
		bool found = false;

		// Search children for a name exactly matching this UTF‑8 token
		for (uint32_t i = 0; i < current->len; ++i) {
			if (current->children[i]->name
				&& strcmp(current->children[i]->name, token) == 0)
			{
				current = current->children[i];
				found = true;
				break;
			}
		}

		if (!found) {
			// No matching child; cleanup and bail out
			free(working);
			return NULL;
		}

		// Next path component
		token = strtok(NULL, delimiters);
	}

	free(working);
	return current;
}
fileNode_t* fileNodebyInum(TSK_INUM_T inum, fileNode_t* root) {
	if (root == NULL) {
		return NULL;
	}
	if (root->inum == inum) {
		return root;
	}

	for (int i = 0; i < root->len; i++) {
		fileNode_t* child = root->children[i];
		if (child->inum == inum) {
			return child;
		}
		if (child->isDir) {
			fileNode_t* result = fileNodebyInum(inum, child);
			if (result != NULL) {
				return result;
			}
		}
	}

	return NULL;
}

static void printHelper(const fileNode_t* fn,uint16_t tabs) {
	if (!fn) {
		return;
	}
	if (fn->isDir) {
		for (int i = 0;i < tabs;i++) {
			putchar('\t');
		}
		if (fn->deleted) {
			_tprintf(L""COLOR_RED"(inode:%"PRIuINUM") % ls\n"COLOR_RESET,fn->inum, fn->name);
		}
		else {
			_tcprintf(L"(inode: % "PRIuINUM") %ls\n",fn->inum, fn->name);
		}
		for (size_t i = 0;i < fn->len;i++) {
			printHelper(fn->children[i], tabs + 1);
		}
	}
	else {
		for (int i = 0;i < tabs;i++) {
			putchar('\t');
		}
		if (fn->deleted) {
			_tprintf(L""COLOR_RED"(inode:%"PRIuINUM") % ls %"PRIu8"\n"COLOR_RESET,fn->inum, fn->name,fn->deleted-1);
		}
		else {
			_tcprintf(L"(inode: % "PRIuINUM") %ls\n",fn->inum, fn->name);
		}
	}

}
void printFileSys(const fileSystem_t* fn) {
	printHelper(fn->root, 0);
}
