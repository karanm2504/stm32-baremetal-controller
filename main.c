#include <stdio.h>

int should_turn_fan_on(float temperature, float threshold)
{
    return temperature >= threshold;
}

int main(void)
{
    float temperatures[] = {25.0f, 30.0f, 32.5f, 28.0f};
    float threshold = 30.0f;

    size_t count = sizeof(temperatures) / sizeof(temperatures[0]);

    for (size_t i = 0; i < count; i++)
    {
        int fan_on = should_turn_fan_on(temperatures[i], threshold);

        printf("Temperature: %.1f C | ", temperatures[i]);

        if (fan_on)
        {
            printf("Fan: ON\n");
        }
        else
        {
            printf("Fan: OFF\n");
        }
    }

    return 0;
}


