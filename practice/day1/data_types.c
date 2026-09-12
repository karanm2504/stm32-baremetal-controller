#include<stdio.h>
#include<stdint.h>

int main(void)
{
	uint8_t  value8 = 255U;
	uint16_t value16 = 65535U;
	uint32_t value32 = 100000U;
	
	int8_t temperature = -20;
	printf("uint8_t value= %u\n", value8);
	printf("uint8_t value= %u\n", value16);
	printf("uint8_t value= %u\n", value32);

	printf("Temperature = %d\n", temperature);
	
	printf("Size of uint8_t  = %zu byte\n", sizeof(value8));
	printf("Size of uint16_t  = %zu byte\n", sizeof(value16));
	printf("Size of uint32_t  = %zu byte\n", sizeof(value32));
	
	return 0;
}
