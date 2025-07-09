#include"recover.h"

void recover(const TSK_INUM_T inum, const fileSystem_t* fs, const TSK_TCHAR* path)
{
    if (path == NULL) {
        fprintf(stderr, "The path is NULL");
        return;
    }
    fileNode_t* node = fileNodebyInum(inum, fs->root);
    if (node->isDir) {
        recoverDirectory(inum, fs, path);
    }
    else {
        recoverSingleFile(inum, fs, path);
    }

}

void recoverSingleFile(const TSK_INUM_T inum, const fileSystem_t * fs, const TSK_TCHAR * path) {
    if (path == NULL) {
        fprintf(stderr, "The path is NULL");
        return;
    }

    fileNode_t* node = fileNodebyInum(inum, fs->root);
    TSK_FS_FILE* file = tsk_fs_file_open_meta(fs->fs, NULL,inum);

    if (file == NULL) {
        fprintf(stderr, "Couldn't open file\n%s\n",tsk_error_get());
        return;
    }

    TSK_TCHAR delim = TSTRCHR(path, '/')==NULL?'\\':'/';
    TSK_TCHAR filePath[MAX_PATH];

    if (TSNPRINTF(filePath, MAX_PATH, _TSK_T("%s%c%hs"), path, delim, node->name) == MAX_PATH) {
        fprintf(stderr, "max path overflow\n");
        return;
    }

    char buffer[2048];
    TSK_OFF_T offset = 0;
    ssize_t bytesRead; 
    FILE* out = _tfopen(filePath, _T("wb"));

    if (out == NULL) {
        fprintf(stderr, "couldn't open file for writing\n");
        return;
    }

    while ((bytesRead = tsk_fs_file_read(file, offset, buffer, sizeof(buffer), TSK_FS_FILE_READ_FLAG_NONE)) > 0) {
        fwrite(buffer, 1, bytesRead, out);
        offset += bytesRead;
    }
    fclose(out);
    tsk_fs_file_close(file);
}
void recoverDirectory(const TSK_INUM_T inum, const fileSystem_t* fs, const TSK_TCHAR* path) {
    if (path == NULL) {
        fprintf(stderr, "The path is NULL");
        return;
    }

    fileNode_t* node = fileNodebyInum(inum, fs->root);
    TSK_TCHAR delim = TSTRCHR(path, '/') == NULL ? '\\' : '/';
    TSK_TCHAR filePath[MAX_PATH];

    if (TSNPRINTF(filePath, MAX_PATH, _TSK_T("%s%c%hs"), path, delim, node->name) == MAX_PATH) {
        fprintf(stderr, "max path overflow\n");
        return;
    }

    if (node->isDir) {

        _tmkdir(filePath);

        for (size_t i = 0;i < node->len;i++) {
            recoverDirectory(node->children[i]->inum, fs, filePath);
        }

    }
    else {

        recoverSingleFile(inum, fs, path);
    
    }
}