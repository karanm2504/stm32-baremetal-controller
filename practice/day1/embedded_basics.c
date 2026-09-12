#include<stdio.h>
#include<stdint.h>
 
int main(void)
{
	const uint16_t adc_max = 4095;
	uint16_t adc_value = 3200U;
	volatile uint8_t data_ready = 0U;

	printf("ADC  maximum =  %u\n", adc_max);
	printf("ADC value = %u\n", adc_value);
	
	data_ready = 1U;
	
	if(data_ready)
	{
	printf("ADC data is ready\n");
	}
	
	if(adc_value > 3000U)
	{
	printf("ADC value is HIGH\n");
	}
	return  0 ;
}
