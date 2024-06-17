#pragma once

#include "esphome.h"

using esphome::time::RealTimeClock;

namespace Irrigation {

    // Define a map to convert day names to indexes
    std::unordered_map<std::string, size_t> day_map = {
        {"mon", 0},
        {"tue", 1},
        {"wed", 2},
        {"thur", 3},
        {"fri", 4},
        {"sat", 5},
        {"sun", 6}
    };

    void clear_days_schedule() {
        bool *days = id(schedule_days);

        for (int i = 0; i < 7; i++) {
            days[i] = false;
        }
    }

    std::vector<std::string> split_to_tokens(std::string str) {
        std::vector<std::string> tokens;
        char *token;

        // Split the list into an array
        token = strtok(&str[0], ", ");

        while (token != NULL) {
            // Skip empty tokens
            if (strlen(token) > 0) {
                // Convert to lower case before saving it to the list
                std::string token_str(token);
                std::transform(token_str.begin(), token_str.end(), token_str.begin(), ::tolower);
                tokens.push_back(token_str);
            }
                  
            token = strtok(NULL, ", ");
        }

        return tokens;
    }

    int16_t time_str_to_minutes(std::string time) {
        // Split the hours and minutes
        int16_t hours = atoi(time.substr(0,2).c_str());
        int16_t minutes = atoi(time.substr(3,2).c_str());

        if (hours > 23 || minutes > 59) {
            return -1;
        }

        // Return the number of minutes from midnight
        return hours * 60 + minutes;
    }

    void update_schedule_day(const std::string& day) {
        ESP_LOGD("update_schedule_day", "Looking for day: %s", day.c_str());

        // Find the day in the map
        auto it = day_map.find(day);

        // Check if day found and get index
        size_t index;
        if (it != day_map.end()) {
            index = it->second;
            bool *days = id(schedule_days);

            ESP_LOGD("update_schedule_day", "Marking day: %d", index);

            // update schedule
            days[index] = true;
        } else {
            // TODO Handle error
            ESP_LOGD("update_schedule_day", "Did not find day: %s", day);
        }        
    }

    bool set_days_config(std::string days_str) {
        ESP_LOGD("set_days_config", "Update days with new schedule: %s", days_str.c_str());

        std::vector<std::string> days = split_to_tokens(days_str);

        for (std::string day : days) {
            ESP_LOGD("set_days_config", "day found: %s", day.c_str());
        }

        bool &odd = id(schedule_odd);
        bool &even = id(schedule_even);

        if (days.size() == 1) {
            if (days.front() == "odd") {
                ESP_LOGD("set_days_config", "Odd found");

                odd = true;
                even = false;
                clear_days_schedule();
            } else if (days.front() == "even") {
                ESP_LOGD("set_days_config", "Even found");

                odd = false;
                even = true;
                clear_days_schedule();
            } else {
                odd = false;
                even = false;
            }
        } else {
            odd = false;
            even = false;
        }

        if (!odd && !even) {
            for (std::string day : days) {
                update_schedule_day(day);
            }
        }

        return true;
    }

    bool set_zone_times_config(int zone, std::string times_str) {
        ESP_LOGD("set_zone_times_config", "Update zone %d with new schedule: %s", zone, times_str.c_str());
        
        std::vector<std::string> times = split_to_tokens(times_str);

        // TODO: 5 as a constant
        int16_t (*zone_times)[5] = id(schedule_zone_times);
        
        // We allow max 5 time values per zone
        for (int i = 0; i < 5; i++) {
            uint16_t mins = -1;

            if (i < times.size()) {
                mins = time_str_to_minutes(times[i]);
                ESP_LOGD("set_zone_times_config", "Time found: %s", times[i].c_str());
                ESP_LOGD("set_zone_times_config", "Time in minutes: %d", mins);

                if (mins < 0) {
                    ESP_LOGD("set_zone_times_config", "Invalid time format: %s", times[i].c_str());
                }
            }

            ESP_LOGD("set_zone_times_config", "Setting zone %d time index %d: %d", zone, i, mins);
            zone_times[zone - 1][i] = mins;
        }
        
        return true;
    }

    bool check_days_schedule(RealTimeClock *time) {
        // Get the current time
        auto now = id(time).now();

        // Check if the time is valid
        if (!now.is_valid()) {
            ESP_LOGD("check_days_schedule", "Time is not valid");
            return false;
        }

        // someone was stupid enough to both have Sunday as the first day of the week _AND_ start index from 1
        int day_of_week = (now.day_of_week + 5) % 7;

        ESP_LOGD("check_days_schedule", "Day of week today: %d", day_of_week);

        bool &odd = id(schedule_odd);
        bool &even = id(schedule_even);
        bool *days = id(schedule_days);

        return (odd && day_of_week % 2 == 1) || 
            (even && day_of_week % 2 == 0) ||
            days[day_of_week];
    }

    bool check_zone_schedule(RealTimeClock *time, int zone) {
        // Get the current time
        auto now = id(time).now();

        // Check if the time is valid
        if (!now.is_valid()) {
            ESP_LOGD("check_zone_schedule", "Time is not valid");
            return false;
        }

        int now_h = now.hour;
        int now_m = now.minute;

        ESP_LOGD("check_zone_schedule", "Zone %d Current time: %02d:%02d", zone, now_h, now_m);

        int time_in_mins = now_h * 60 + now_m;

        ESP_LOGD("check_zone_schedule", "Zone %d Time in minutes: %d", zone, time_in_mins);

        int16_t (*zone_times)[5] = id(schedule_zone_times);

        for (int i = 0; i < 5; i++) {
            if (zone_times[zone - 1][i] == time_in_mins) {
                ESP_LOGD("check_zone_schedule", "Zone %d schedule match!", zone);
                return true;
            }
        }

        return false;
    }
}