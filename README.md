# Embedded Sensor Controller

A C project using simulated temperature readings to control
a printed fan status.

## Features
- Processes multiple readings using an array and a loop.
- Uses a function to decide the fan state.
- Turns the fan ON at or above 30 degrees Celsius.

## Build and run
```bash
gcc -std=c11 -Wall -Wextra main.c -o controller
./controller
```

## Expected results
- 25.0 C: OFF
- 30.0 C: ON
- 32.5 C: ON
- 28.0 C: OFF

## Limitations
Uses fixed sample readings. No physical hardware is connected.
