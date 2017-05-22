#include <stdio.h>
#include <stdlib.h>
void print_size(int x, int y, short int ar[y][x]) {
  int c;
  unsigned long r;
  r = sizeof(ar)/sizeof(ar[0]);
  c = (sizeof(ar[0])/sizeof(ar[0][0])-1);
  printf("r = %lu,  c = %d\n",r,c );
  ar[0][0] = 7;
}

int main(int argc, char const *argv[]) {
  short int grid[140][218];
  grid[0][0] = 2;
  printf("%d\n",grid[0][0]);
  int c;
  int r;
  r = sizeof(grid)/sizeof(grid[0]);
  c = sizeof(grid[0])/sizeof(grid[0][0]);

  print_size(r,c, grid );
  /* code */
  printf("%d\n",grid[0][0]);


  printf("r = %d,  c = %d\n",r,c );

  return 0;
}
