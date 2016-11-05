#include	<linux/kernel.h>
#include	<linux/linkage.h>
asmlinkage long	sys_simple_add(int num1, int num2, int* result)
{
	printk(KERN_ALERT "num1 = %d", num1);
	printk(KERN_ALERT "num2 = %d", num2);
	
	*result = num1 + num2;
	
	printk(KERN_ALERT "sum = %d", *result);
	return 0;
}
