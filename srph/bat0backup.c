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

#define EMA_PERIOD 10  // Number of data points to consider for EMA

// Function to read a value from a file
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

// Function to convert a string to an integer
int convert_to_int(const char *str) {
    return atoi(str);
}

// Function to convert micro-watts to watts
double convert_to_watts(int micro_watts) {
    return micro_watts / 1000000.0;
}

// Function to calculate Exponential Moving Average (EMA)
double calculate_ema(double *data, int data_length, int period) {
    if (data_length < period) {
        return -1; // Not enough data points
    }

    double multiplier = 2.0 / (period + 1);
    double ema = data[data_length - period]; // Starting point

    for (int i = data_length - period + 1; i < data_length; i++) {
        ema = ((data[i] - ema) * multiplier) + ema;
    }
    return ema;
}

// Function to predict time to reach a target percentage
double predict_time(double current_energy, double target_energy, double power) {
    if (power == 0) {
        return -1; // Avoid division by zero
    }
    return (target_energy - current_energy) / power;
}

// Function to format time in HH:MM
void format_time(double hours, char *buffer, size_t buffer_size) {
    int total_minutes = (int)(hours * 60);
    int hh = total_minutes / 60;
    int mm = total_minutes % 60;
    snprintf(buffer, buffer_size, "%02d:%02d", hh, mm);
}

// Flag to indicate if the program should keep running
volatile sig_atomic_t keep_running = 1;

// Signal handler to set the flag to 0 when an interrupt signal is received
void handle_sigint(int sig) {
    keep_running = 0;
}

int main() {
    char buffer[64]; // Buffer to hold the value read from the file
    double power_data[EMA_PERIOD] = {0}; // Array to store power_now values
    int data_count = 0; // Number of data points collected

    // Register the signal handler for interrupt signal (Ctrl+C)
    signal(SIGINT, handle_sigint);

    while (keep_running) {
        system("clear"); // Clear the screen

        // Read remaining battery energy
        int energy_now = 0;
        if (read_value(BAT_ENN_PATH, buffer, sizeof(buffer)) == 0) {
            energy_now = convert_to_int(buffer);
            double energy_now_wh = convert_to_watts(energy_now); // Assuming energy is in µWh
            printf("Remaining battery energy: %.2f Wh\n", energy_now_wh);
        }

        // Read full battery energy
        int energy_full = 0;
        if (read_value(BAT_ENF_PATH, buffer, sizeof(buffer)) == 0) {
            energy_full = convert_to_int(buffer);
            double energy_full_wh = convert_to_watts(energy_full); // Assuming energy is in µWh
            printf("Full battery energy: %.2f Wh\n", energy_full_wh);
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

        // Read current power usage/input
        int power_now = 0;
        if (read_value(BAT_PWN_PATH, buffer, sizeof(buffer)) == 0) {
            power_now = convert_to_int(buffer);
            double power_now_w = convert_to_watts(power_now); // Assuming power is in µW

            // Store the power_now value for EMA calculation
            if (data_count < EMA_PERIOD) {
                power_data[data_count++] = power_now_w;
            } else {
                // Shift the array to make space for the new value
                memmove(power_data, power_data + 1, (EMA_PERIOD - 1) * sizeof(double));
                power_data[EMA_PERIOD - 1] = power_now_w;
            }

            // Calculate EMA of power_now
            double ema_power = calculate_ema(power_data, data_count, EMA_PERIOD);
            printf("Exponential Moving Average (EMA) of power: %.2f W\n", ema_power);

            if (is_charging) {
                printf("Current power input: %.2f W\n", power_now_w);
            } else {
                printf("Current power usage: %.2f W\n", power_now_w);
            }

            // Predict time to reach target energy levels
            char time_buffer[64];
            if (is_charging) {
                double target_energy = 0.9 * energy_full;
                if (energy_now < target_energy) {
                    double time_to_90_percent = predict_time(energy_now, target_energy, ema_power);
                    format_time(time_to_90_percent, time_buffer, sizeof(time_buffer));
                    printf("Time to reach 90%%: %s\n", time_buffer);
                } else {
                    printf("Battery is already above 90%%\n");
                }
            } else {
                double target_energy = 0.1 * energy_full;
                if (energy_now > target_energy) {
                    double time_to_10_percent = predict_time(energy_now, target_energy, -ema_power); // Use negative power for discharging
                    format_time(time_to_10_percent, time_buffer, sizeof(time_buffer));
                    printf("Time to reach 10%%: %s\n", time_buffer);
                } else {
                    printf("Battery is already below 10%%\n");
                }
            }
        }

        // Sleep for 1 second before reading the values again
        sleep(1);
    }

    printf("Program terminated.\n");
    return EXIT_SUCCESS;
}