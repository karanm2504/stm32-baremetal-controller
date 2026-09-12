#include<stdio.h>
#include<stdint.h>

int main(void)
{
	volatile  uint8_t button_pressed = 0U;
	printf("Initial button presssed= %u\n",button_pressed);
	button_pressed = 1U;
	printf("Updated button pressed = %u\n",button_pressed);
	return 0;
}

