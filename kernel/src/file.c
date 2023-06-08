#include "klib.h"
#include "file.h"

#define TOTAL_FILE 128

file_t files[TOTAL_FILE];

static file_t *falloc() {
  // Lab3-1: find a file whose ref==0, init it, inc ref and return it, return NULL if none
  //TODO();
  for(int i = 0; i < TOTAL_FILE; i++){
    if(files[i].ref == 0){
      files[i].type = TYPE_NONE;
      files[i].ref++;
      return &files[i];
    }
  }
  return NULL;
}

file_t *fopen(const char *path, int mode) {
  file_t *fp = falloc();
  inode_t *ip = NULL;
  if (!fp) goto bad;
  // TODO: Lab3-2, determine type according to mode
  // iopen in Lab3-2: if file exist, open and return it
  //       if file not exist and type==TYPE_NONE, return NULL
  //       if file not exist and type!=TYPE_NONE, create the file as type
  // you can ignore this in Lab3-1
  int open_type = 114514;
  if(mode & O_CREATE){
    if(mode & O_DIR) open_type = TYPE_DIR;
    else open_type = TYPE_FILE;
  }else{
    open_type = TYPE_NONE;
  }
  ip = iopen(path, open_type);
  if (!ip) goto bad;
  int type = itype(ip);
  if (type == TYPE_FILE || type == TYPE_DIR) {
    // TODO: Lab3-2, if type is not DIR, go bad if mode&O_DIR
    if(type != TYPE_DIR && mode&O_DIR){
      goto bad;
    }
    // TODO: Lab3-2, if type is DIR, go bad if mode WRITE or TRUNC
    if(type == TYPE_DIR && (mode&O_WRONLY || mode&O_TRUNC || mode&O_RDWR)){
      goto bad;
    }
    // TODO: Lab3-2, if mode&O_TRUNC, trunc the file
    if(mode&O_TRUNC){
      itrunc(ip);
    }
    fp->type = TYPE_FILE; // file_t don't and needn't distingush between file and dir
    fp->inode = ip;
    fp->offset = 0;
  } else if (type == TYPE_DEV) {
    fp->type = TYPE_DEV;
    fp->dev_op = dev_get(idevid(ip));
    iclose(ip);
    ip = NULL;
  } else assert(0);
  fp->readable = !(mode & O_WRONLY);
  fp->writable = (mode & O_WRONLY) || (mode & O_RDWR);
  return fp;
bad:
  if (fp) fclose(fp);
  if (ip) iclose(ip);
  return NULL;
}

int fread(file_t *file, void *buf, uint32_t size) {
  // Lab3-1, distribute read operation by file's type
  // remember to add offset if type is FILE (check if iread return value >= 0!)
  if (!file->readable) return -1;
  //TODO();
  //printf("here in fread\n");
  int offadd = -1;
  if(file->type == TYPE_FILE){
    offadd = iread(file->inode, file->offset, buf, size);
    if(offadd != -1) file->offset += offadd;
  }else if(file->type == TYPE_DEV){
    offadd = file->dev_op->read(buf, size);
  }
  //printf("here after fread\n");
  return offadd;
}

int fwrite(file_t *file, const void *buf, uint32_t size) {
  // Lab3-1, distribute write operation by file's type
  // remember to add offset if type is FILE (check if iwrite return value >= 0!)
  if (!file->writable) return -1;
  //TODO();
  //printf("here in fwrite\n");
  int offadd = -1;
  if(file->type == TYPE_FILE || file->type == TYPE_DIR){
    offadd = iwrite(file->inode, file->offset, buf, size);
    if(offadd != -1) file->offset += offadd;
  }else if(file->type == TYPE_DEV){
    offadd = file->dev_op->write(buf, size);
  }
  //printf("here after fwrite\n");
  return offadd;
}

uint32_t fseek(file_t *file, uint32_t off, int whence) {
  // Lab3-1, change file's offset, do not let it cross file's size
  if (file->type == TYPE_FILE || file->type == TYPE_DIR) {
    //TODO();
    if(whence == SEEK_SET){
      file->offset = off;
    }else if(whence == SEEK_CUR){
      file->offset += off;
    }else if(whence == SEEK_END){
      file->offset = isize(file->inode) + off;
    }
    return file->offset;
  }
  return -1;
}

file_t *fdup(file_t *file) {
  // Lab3-1, inc file's ref, then return itself
  //TODO();
  file->ref++;
  return file;
}

void fclose(file_t *file) {
  // Lab3-1, dec file's ref, if ref==0 and it's a file, call iclose
  //TODO();
  file->ref--;
  if(file->ref == 0 && (file->type == TYPE_FILE || file->type == TYPE_DIR)){
    iclose(file->inode);
  }
}
