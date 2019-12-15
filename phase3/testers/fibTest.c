/*	Test of a CPU intensive recusive job */
#include "../../h/const.h"
#include "../../h/types.h"

#include "/usr/local/include/umps2/umps/libumps.e"

#include "h/tconst.h"
#include "print.e"


int fib (int i) {
	if ((i == 1) || (i ==2))
		return (1);

	return(fib(i-1)+fib(i-2));
}


void main() {
	int i, j, k;

	/*print(WRITETERMINAL, "Recursive Fibanaci Test starts\n");*/

	i = fib(7);

	/*print(WRITETERMINAL, "Recursion Concluded\n");*/

	/*
	if (i == 13) {
		print(WRITETERMINAL, "Recursion Concluded Successfully\n");
	}
	else
		print(WRITETERMINAL, "ERROR: Recursion problems\n");
		*/

	j = fib(7);
	k = fib(7);

	if(i == j && k == j){
		print(WRITETERMINAL, "Only the O will print here :(");
	}

	/* Terminate normally */
	SYSCALL(TERMINATE, 0, 0, 0);
}
