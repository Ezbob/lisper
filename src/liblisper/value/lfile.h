
#ifndef _HEADER_FILE_file_20240217143452_
#define _HEADER_FILE_file_20240217143452_

struct lvalue;

struct lfile {
  struct lvalue *path;
  struct lvalue *mode;
  void *fp;
};

struct lfile *lfile_new(struct lvalue *path, struct lvalue *mode, void *fp);

#endif