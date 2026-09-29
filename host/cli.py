import serial
import sys
import time
import subprocess


def get_wsl_host_ip():
    result = subprocess.run(
        ["ip", "route", "show", "default"],
        capture_output=True,
        text=True
    )

    parts = result.stdout.split()

    if "via" in parts:
        return parts[parts.index("via") + 1]

    return "127.0.0.1"


def open_serial(port):
    if port.startswith("rfc2217://"):
        return serial.serial_for_url(
            port,
            baudrate=115200,
            timeout=0.2
        )

    return serial.Serial(
        port=port,
        baudrate=115200,
        timeout=0.2
    )


def send_command(port, command):
    for attempt in range(2):
        ser = open_serial(port)

        # Allow RFC2217 / serial connection to settle
        time.sleep(0.8)

        ser.reset_input_buffer()

        ser.write((command + "\r\n").encode())
        ser.flush()

        response = ""
        start_time = time.time()

        while (time.time() - start_time) < 2.0:
            data = ser.read(128)

            if data:
                response += data.decode(errors="ignore")

                # Allow remaining bytes to arrive
                time.sleep(0.1)

                while ser.in_waiting > 0:
                    response += ser.read(
                        ser.in_waiting
                    ).decode(errors="ignore")

                break

        ser.close()

        if response:
            print(response, end="")
            return

        # Retry once if no reply arrived
        time.sleep(0.5)

    print("No response from STM32")


def print_usage():
    print("Usage:")
    print("python cli.py <command>")
    print()
    print("Available examples:")
    print("python cli.py status")
    print("python cli.py start")
    print("python cli.py stop")
    print("python cli.py help")
    print('python cli.py "led on"')
    print('python cli.py "led off"')


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print_usage()
        sys.exit(1)

    host_ip = get_wsl_host_ip()

    port = f"rfc2217://{host_ip}:4000"

    command = " ".join(sys.argv[1:])

    try:
        send_command(port, command)

    except serial.SerialException as error:
        print("Serial connection error:")
        print(error)
        sys.exit(1)

    except KeyboardInterrupt:
        print("\nStopped.")
        sys.exit(0)