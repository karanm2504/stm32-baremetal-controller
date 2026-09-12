#include<stdio.h>
#include<stdint.h>

int main(void)
{
	uint8_t value = 13U;

	printf("Orignal Value : %u\n", value);
	
	value &= ~(1U << 3);
	printf("Clear bit 3  :  %u\n", value);

	value ^= (1U << 2);
	printf("Toggle bit 2  :  %u\n", value);
	
	if( value  & (1U <<0))
	{
	printf("Bit 0 is Set\n");
	}
	else
	{
	printf("Bit 0 is Clear\n");
	}
	return 0;
}

