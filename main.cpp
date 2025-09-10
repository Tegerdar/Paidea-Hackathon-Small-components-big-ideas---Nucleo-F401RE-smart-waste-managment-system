#include "mbed.h"

// Ultrasonic sensor pins
DigitalOut trig(D2);
DigitalIn echo(D3);

// Timer pins
Timer timer;
Timer timeout;

// Flame sensor pin (digital output)
DigitalIn flame(D4);

// Digital brightness sensor (connected to D5)
DigitalIn brightness_sensor(D5);

// Tilt sensor (connected to D6) - detects if container is open
DigitalIn tilt_sensor(D6);

// Infrared detector (connected to D7)
DigitalIn ir_detector(D7);

// LED outputs
DigitalOut led1(D13);
DigitalOut led2(D11);
DigitalOut led3(D12);

// Bin setup
const float BIN_HEIGHT_CM = 150.0f;   // Height of the bin (from sensor to bottom)

// Measure distance to surface in cm
// Returns -1 if no echo is detected
float measure_distance_cm() {
    // Trigger pulse
    trig = 0;
    wait_us(2);
    trig = 1;
    wait_us(10);   // 10 µs trigger pulse
    trig = 0;

    // Wait for echo to go HIGH (with ~25 ms timeout)
    timeout.reset();
    timeout.start();
    while (echo == 0) {
        if (timeout.read_ms() > 25) {
            timeout.stop();
            return -1.0f; // No echo detected
        }
    }
    timeout.stop();

    // Measure echo HIGH duration
    timer.reset();
    timer.start();

    timeout.reset();
    timeout.start();
    while (echo == 1) {
        if (timeout.read_ms() > 25) {
            break; // Limit ~400 cm
        }
    }
    timer.stop();
    timeout.stop();

    // Convert to distance (cm)
    float distance_cm = timer.read_us() / 58.0f; // µs -> cm
    return distance_cm;
}

// Take multiple samples and average them
float get_filtered_distance(int samples = 5) {
    float sum = 0;
    int valid = 0;
    for (int i = 0; i < samples; i++) {
        float d = measure_distance_cm();
        if (d > 0) {
            sum += d;
            valid++;
        }
        wait_ms(60);
    }
    if (valid == 0) return -1;
    return sum / valid;
}

// Read digital brightness sensor
// Returns true if bright, false if dark
bool read_brightness() {
    return (brightness_sensor.read() == 0);
}

// Read tilt sensor
// Returns true if container is open, false if closed
bool is_container_open() {
    return (tilt_sensor.read() == 0); // 0 means open/tilted
}

// Read infrared detector - single read without debouncing
bool read_ir_detector() {
    return (ir_detector.read() == 0); // Assuming 0 means object detected im whenever getting that objerct detected or no if i change 0 and 1
}

// Control LEDs based on darkness
void control_leds(bool is_dark) {
    if (is_dark) {
        led1 = 1;  // Turn on LED1
        led2 = 1;  // Turn on LED2
        led3 = 1;  // Turn on LED3
        printf("LIGHTS ON\n");
    } else {
        led1 = 0;  // Turn off LED1
        led2 = 0;  // Turn off LED2
        led3 = 0;  // Turn off LED3
    }
}

int main() {
    printf("SYSTEM START\n");
    printf("BIN HEIGHT %.1f cm\n\n", BIN_HEIGHT_CM);

    // Test sensors at startup
    printf("TEST\n");
    bool test_bright = read_brightness();
    bool test_open = is_container_open();
    bool test_ir = read_ir_detector();
    printf("LIGHT: %s | CONTAINER STATUS: %s | IR: %s\n", 
           test_bright ? "BRIGHT" : "DARK",
           test_open ? "OPEN" : "CLOSED",
           test_ir ? "OBJECT DETECTED" : "NO OBJECT");
    printf("\n");

    wait(2);

    while (true) {
        // Check if container is open first
        bool container_open = is_container_open();
        
        if (container_open) {
            printf("CONTAINER IS OPEN MEASUREMENTS PAUSED\n");
        } else {
            // Measure bin fill (only if container is closed)
            float distance = get_filtered_distance();

            if (distance < 0) {
                printf("NO ECHO DETECTED\n");
            } else {
                float h = BIN_HEIGHT_CM - distance;
                if (h < 0) h = 0;
                if (h > BIN_HEIGHT_CM) h = BIN_HEIGHT_CM;

                float fill_percent = (h / BIN_HEIGHT_CM) * 100.0f;

                // output
                printf("FILL LEVEL: %5.1f%% | ", fill_percent);

                // Visual fill indicator
                int bars = static_cast<int>((fill_percent / 100.0f) * 20);
                printf("[");
                for (int i = 0; i < 20; i++) {
                    if (i < bars) printf("#");
                    else printf(" ");
                }
                printf("]\n");
            }
        }

        // Check flame sensor
        if (flame.read() == 0) {  
            // Flame detected
            printf("ALERT FLAME\n");
        } else {
            printf("NO FLAME\n");
        }

        // Read brightness sensor
        bool is_bright = read_brightness();
        printf("LIGHT: %s\n", is_bright ? "BRIGHT" : "DARK");

        // Read infrared detector
        bool object_detected = read_ir_detector();
        printf("IR: %s\n", object_detected ? "OBJECT DETECTED" : "NO OBJECT");

        // Control LEDs based on darkness and IR detection
        if (object_detected == 1) {
            control_leds(!is_bright);  // Turn on LEDs when it's dark
        }
        
        // Display container status
        printf("CONTAINER STATUS: %s\n", container_open ? "OPEN" : "CLOSED");

        printf("----------------------------------------------------------------------------\n");
        wait_ms(10000); // 10 second delay
    }
}