// Shane Sarnac
// CSCI 3753- Operating Systems
// Fall 2016
// Homework 1

#ifndef HOMEWORK1_C
#define HOMEWORK1_C

#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>

int main() {
	int sum = 0;
	int num1 = 5;
	int num2 = 6;
	int* result = &sum;
	
	syscall(326); // 326 => sys_helloworld()
	syscall(327, num1, num2, result); // 327 => sys_simple_add
	
	printf("sum = %d", *result);
	
	
	return 0;
}

#endif
