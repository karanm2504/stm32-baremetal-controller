#include <stdio.h>
#include <stdint.h>

int main(void)
{
 	uint8_t value = 5U;
	printf("Decimal value : %u\n", value);
	printf("Hex value : 0x%02X\n", value);
	
	printf("Left shift : %u\n", value <<1 );
	printf("Bit 0 check: %u\n", value & 1U);
	printf("Set bit 3 : %u\n", value | (1U << 3));
	
	return 0;
}

