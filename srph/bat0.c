#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#define BAT_ENN_PATH "/sys/class/power_supply/BAT0/energy_now"
#define BAT_ENF_PATH "/sys/class/power_supply/BAT0/energy_full"
#define BAT_ENFD_PATH "/sys/class/power_supply/BAT0/energy_full_design"
#define BAT_PWN_PATH "/sys/class/power_supply/BAT0/power_now"
#define BAT_STAT_PATH "/sys/class/power_supply/BAT0/status"

int read_value(const char *file_path, char *buffer, size_t buffer_size) {
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Failed to open file");
        return -1;
    }

    if (fgets(buffer, buffer_size, file) == NULL) {
        perror("Failed to read value from file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

int convert_to_int(const char *str) {
    return atoi(str);
}

double convert_to_watts(int micro_watts) {
    return micro_watts / 1000000.0;
}

volatile sig_atomic_t keep_running = 1;
void handle_sigint(int sig) {
    keep_running = 0;
}

int main() {
    char buffer[64];
    signal(SIGINT, handle_sigint);

    while (keep_running) {
        system("clear"); // Clear

        // Read remaining energy
        int energy_now = 0;
        if (read_value(BAT_ENN_PATH, buffer, sizeof(buffer)) == 0) {
            energy_now = convert_to_int(buffer);
            double energy_now_wh = convert_to_watts(energy_now); 
            printf("Remaining battery energy: %.2f Wh\n", energy_now_wh);
        }

        // Read last full capacity
        int energy_full = 0;
        if (read_value(BAT_ENF_PATH, buffer, sizeof(buffer)) == 0) {
            energy_full = convert_to_int(buffer);
            double energy_full_wh = convert_to_watts(energy_full); 
            printf("Last full capacity: %.2f Wh\n", energy_full_wh);
        }

        // Read design capacity
        int energy_full_design = 0;
        if (read_value(BAT_ENFD_PATH, buffer, sizeof(buffer)) == 0) {
            energy_full_design = convert_to_int(buffer);
            double energy_full_design_wh = convert_to_watts(energy_full_design); 
            printf("Design capacity: %.2f Wh\n", energy_full_design_wh);
        }

        // Current output/input
        int power_now = 0;
        if (read_value(BAT_PWN_PATH, buffer, sizeof(buffer)) == 0) {
            power_now = convert_to_int(buffer);
            double power_now_w = convert_to_watts(power_now); 
            printf("Current I/O: %.2f W\n", power_now_w);
        }

        // Read battery status
        char battery_status[64];
        int is_charging = 0;
        if (read_value(BAT_STAT_PATH, battery_status, sizeof(battery_status)) == 0) {
            printf("Battery status: %s\n", battery_status);
            if (strstr(battery_status, "Charging") != NULL) {
                is_charging = 1;
            }
        }

        // Measurement Interval
        sleep(1);
    }

    printf("Program terminated.\n");
    return EXIT_SUCCESS;
}