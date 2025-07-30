# LED Service Hw Test

LED tests help verify that the LEDs on the switch are working correctly. Our LED tests trigger different sequences of LED blinking / color changes based on the test, config, and BSP mapping.

## Steps to Run LED Tests

1. Compile the `led_service_hw_test` binary by following instructions on the [build page](/docs/build/building_fboss_on_docker_containers/).

2. Copy the binary onto the switch.

3. Create a file named `/tmp/led.conf` on your switch, with your platform name (`PLATFORM_NAME`) as follows:

    ```json
    {
      "defaultCommandLineArgs": {
        "mode": "PLATFORM_NAME"
      },
      "ledControlledThroughService": true
    }
    ```

4. Run the LED color change test:

    ```bash
    ./led_service_hw_test --gtest_filter=LedServiceTest.checkLedColorChange --led-config /tmp/led.conf --visual_delay_sec 1
    ```

    This test is successful if the following sequence occurs:
    - Both LEDs turn on.
    - Both LEDs turn off.
    - Second LED turns on, then turns blue, then turns yellow.

5. Run the LED blinking test:

    ```bash
    ./led_service_hw_test --gtest_filter=LedServiceTest.checkLedBlinking --led-config /tmp/led.conf --visual_delay_sec 1
    ```

    This test is successful if the LEDs are blinking blue and yellow.


## Expected Tests to Pass
| Test Name                     | Basic Link Tests                                                      |
|------------------------------|----------------------------------------------------------------------|
| LedServiceTest.checkLedColorChange | Checks that LEDs can turn on/off, change color between blue and yellow. |
| LedServiceTest.checkLedBlinking    | Checks that LEDs can blink between different colors.                |
