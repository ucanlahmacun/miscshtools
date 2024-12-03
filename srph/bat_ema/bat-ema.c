#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <lua.h>         // Include the Lua header
#include <lauxlib.h>     // Include the auxiliary library header
#include <lualib.h>      // Include the standard Lua libraries header


#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

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

double calculate_ema_with_lua(lua_State *L, double current_sample) {
    lua_getglobal(L, "calculate_ema");
    lua_pushnumber(L, current_sample);
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        fprintf(stderr, "Failed to call Lua function: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);  // remove error message
        return -1;
    }
    double ema = lua_tonumber(L, -1);
    lua_pop(L, 1);  // remove result
    return ema;
}

int main() {
    char buffer[64];
    signal(SIGINT, handle_sigint);

    // Initialize Lua
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    if (luaL_dofile(L, "ema.lua") != LUA_OK) {
        fprintf(stderr, "Failed to load Lua script: %s\n", lua_tostring(L, -1));
        return EXIT_FAILURE;
    }

    double ema = 0.0;

    while (keep_running) {
        system("clear"); // Clear

        // Read remaining energy
        int energy_now = 0;
        if (read_value(BAT_ENN_PATH, buffer, sizeof(buffer)) == 0) {
            energy_now = convert_to_int(buffer);
            double energy_now_wh = convert_to_watts(energy_now); 
            printf(YELLOW "Remaining battery energy: %.2f Wh\n", energy_now_wh);
        }

        // Read last full capacity
        int energy_full = 0;
        if (read_value(BAT_ENF_PATH, buffer, sizeof(buffer)) == 0) {
            energy_full = convert_to_int(buffer);
            double energy_full_wh = convert_to_watts(energy_full); 
            printf(GREEN "Last full capacity: %.2f Wh\n", energy_full_wh);
        }

        // Read design capacity
        int energy_full_design = 0;
        if (read_value(BAT_ENFD_PATH, buffer, sizeof(buffer)) == 0) {
            energy_full_design = convert_to_int(buffer);
            double energy_full_design_wh = convert_to_watts(energy_full_design); 
            printf(CYAN "Design capacity: %.2f Wh\n", energy_full_design_wh);
        }

        // Current output/input
        int power_now = 0;
        if (read_value(BAT_PWN_PATH, buffer, sizeof(buffer)) == 0) {
            power_now = convert_to_int(buffer);
            double power_now_w = convert_to_watts(power_now); 
            printf(MAGENTA "Current I/O: %.2f W\n", power_now_w);
            ema = calculate_ema_with_lua(L, power_now_w);  // Calculate EMA
            printf(WHITE "EMA of power usage: %.2f W\n", ema);
        }

        // Read battery status
        char battery_status[64];
        int is_charging = 0;
        if (read_value(BAT_STAT_PATH, battery_status, sizeof(battery_status)) == 0) {
            printf(WHITE "Battery status: %s\n", battery_status);
            if (strstr(battery_status, "Charging") != NULL) {
                is_charging = 1;
            }
        }

        // Use EMA to predict battery level reaching 10% or 90%
        double target_energy;
        double current_energy = convert_to_watts(energy_now);
        double remaining_energy;
        double time_to_target;

        if (is_charging) {
            target_energy = 0.9 * convert_to_watts(energy_full);
            remaining_energy = target_energy - current_energy;
        } else {
            target_energy = 0.1 * convert_to_watts(energy_full);
            remaining_energy = current_energy - target_energy;
        }

        if (ema != 0) { // Avoid division by zero
            time_to_target = remaining_energy / ema; // Time in hours
            int time_to_target_seconds = (int)(time_to_target * 3600);
            int hours = time_to_target_seconds / 3600;
            int minutes = (time_to_target_seconds % 3600) / 60;
            int seconds = time_to_target_seconds % 60;

            if (is_charging) {
                printf(GREEN "Battery will reach 90%% in approximately %02d:%02d:%02d\n", hours, minutes, seconds);
            } else {
                printf(RED "Battery will reach 10%% in approximately %02d:%02d:%02d\n", hours, minutes, seconds);
            }
        }

        // Measurement Interval
        sleep(1);
    }

    lua_close(L);
    printf("Program terminated.\n");
    return EXIT_SUCCESS;
}