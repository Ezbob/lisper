
#ifndef _HEADER_FILE_cells_20240217143232_
#define _HEADER_FILE_cells_20240217143232_

struct lvalue;

struct lcells {
  unsigned long long count;
  struct lvalue **cells;
};

#endif