#include "malloc.c"

int main(){
  int* n = malloc(sizeof(int));
  *n = 456;
	printf("%s %p\n", "Pointer N", &n);
	printf("N is %d\n", *n);
	int* m = malloc(sizeof(int));
  *m = 100;
	printf("%s %p\n", "Pointer M", &m);
	printf("M is %d\n", *m);
	int* o = malloc(sizeof(int));
  o = m;
	printf("%s %p\n", "Pointer O", &o);
	printf("O is %d\n", *o);
}