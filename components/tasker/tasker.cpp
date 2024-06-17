#include "tasker.h"

#include "esphome/core/log.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace tasker {

static const char *TAG = "tasker";

// Define a mapping from day strings to numerical values
const std::unordered_map<std::string, int> day_mapping = {
    {"MON", 0}, {"TUE", 1}, {"WED", 2}, {"THU", 3}, {"FRI", 4}, {"SAT", 5}, {"SUN", 6}, {"ODD", -1}, {"EVEN", -2}
};

void populate_days(Schedule& schedule, const std::vector<int> &day_values) {
    schedule.even = false;
    schedule.odd = false;
    schedule.days.raw = 0;

    if (day_values[0] == -1) {
        schedule.odd = true;
    } else if (day_values[0] == -2) {
        schedule.even = true;
    } else {
        for (int day : day_values) {
            // Set the corresponding bit
            schedule.days.raw |= (1 << day); 
        }        
    }
}

void Schedule::setup() {
    // Default to 1, to make the next loop trigger at the second before, which is 0
    last_sec_ = 1;
}

void Schedule::loop() {
    ESPTime time = this->parent_->time_->now();
    uint8_t curr_sec = time.second;
    
    if (curr_sec >= 0 && last_sec_ > curr_sec) {
        // if (schedules_.size() > 0) {
        //     schedules_[0]->dump();
        // }

        // ESP_LOGD(TAG, "timestamp: %d", time.timestamp);
        // ESP_LOGD(TAG, "time: %02d:%02d:%02d", time.hour, time.minute, time.second);
        
        check_trigger(time.hour, time.minute);
    }
    last_sec_ = curr_sec;
}

void Schedule::dump_config() {
    ESP_LOGCONFIG(TAG, "dump_config()");
}

void Schedule::dump() const {
    //ESP_LOGD("schedule", "Schedule ID: %s", get_id());
    ESP_LOGD("schedule", "Days of Week: %s", days_of_week_text->state.c_str());
    ESP_LOGD("schedule", "Odd: %d", odd);
    ESP_LOGD("schedule", "Even: %d", even);
    // TODO loop and map
    ESP_LOGD("schedule", "Mon: %d", days.day.mon);
    ESP_LOGD("schedule", "Tue: %d", days.day.tue);
    ESP_LOGD("schedule", "Wed: %d", days.day.wed);
    ESP_LOGD("schedule", "Thu: %d", days.day.thu);
    ESP_LOGD("schedule", "Fri: %d", days.day.fri);
    ESP_LOGD("schedule", "Sat: %d", days.day.sat);
    ESP_LOGD("schedule", "Sun: %d", days.day.sun);
    ESP_LOGD("schedule", "Times: %s", times_text->state.c_str());
    for (int i = 0; i < time_cnt; i++) {
         ESP_LOGD("schedule", "Time[%d]: %02d:%02d", i, time[i].hour, time[i].minute);
    }
}

void Schedule::check_trigger(uint8_t hour, uint8_t minute) {
    for (Time& t : time) {
        // Check for matching trigger time
        if (t.hour == hour && t.minute == minute) {
             ESP_LOGD(TAG, "Schedule has triggered!");
             trigger_->trigger();
        }
    }
}

void Schedule::on_days_of_week_state_changed(const std::string& days_of_week) {
    ESP_LOGD(TAG, "Schedule days of week changed: %s", days_of_week.c_str());

    // parse the string
    std::vector<int> days;
    bool success = true;
    size_t pos = 0;
    while (pos < days_of_week.size()) {
        // TODO Check for illegal characters
        size_t end_pos = days_of_week.find_first_of(',', pos);
        std::string token = days_of_week.substr(pos, end_pos - pos);
        pos = (end_pos == std::string::npos) ? days_of_week.size() : end_pos + 1;

        size_t hyphen_pos = token.find('-');
        if (hyphen_pos != std::string::npos) {  // Token contains a range
            std::string start_day = token.substr(0, hyphen_pos);
            std::string end_day = token.substr(hyphen_pos + 1);
            auto start_it = day_mapping.find(start_day);
            auto end_it = day_mapping.find(end_day);
            if (start_it != day_mapping.end() && end_it != day_mapping.end()) {
                int start_value = start_it->second;
                int end_value = end_it->second;
                if (start_value < 0 || end_value < 0) {
                    success = false; // ODD or EVEN can't be in ranges
                    break;
                } else if (start_value < end_value) {
                    for (int day = start_value; day <= end_value; ++day) {
                        ESP_LOGD("schedule", "Found day: %d", day);
                        days.push_back(day);
                    }
                } else {
                    success = false; // Start day is same or after end day
                    break;
                }
            } else {
                success = false; // Invalid day names in the range
                break;
            }
        } else {  // Single day token
            auto it = day_mapping.find(token);
            if (it != day_mapping.end()) {
                ESP_LOGD("schedule", "Found day: %d", it->second);
                days.push_back(it->second);
            } else {
                success = false; // Invalid day name
                break;
            }
        }
    }  

    if (days.size() > 1 && days[0] < 0) {
         success = false; // ODD or EVEN can not be combined with other options
    }

    if (success) {
        populate_days(*this, days);
    } else {
        ESP_LOGD("schedule", "Invalid format of days of week");
        return;
    } 
}

void Schedule::on_times_state_changed(const std::string& times) {
    ESP_LOGD(TAG, "Schedule times changed: %s", times.c_str());

    // parse the string
    std::vector<Time> time_list;
    bool success = true;
    size_t pos = 0;
    while (pos < times.size()) {
        size_t end_pos = times.find_first_of(',', pos);
        std::string token = times.substr(pos, end_pos - pos);
        pos = (end_pos == std::string::npos) ? times.size() : end_pos + 1;

        size_t colon_pos = token.find(':');
        if (colon_pos == std::string::npos) {
            success = false; // Could not find : in time
            break;
        }

        int hour = std::stoi(token.substr(0, colon_pos));
        int minute = std::stoi(token.substr(colon_pos + 1, 2));

        if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60) {
            ESP_LOGD("schedule", "Found time: %02d:%02d", hour, minute);
            time_list.push_back(Time(hour, minute));
        } else {
            success = false; // Wrong time format
            break;
        }
    }   

    if (success) {
        if (time_list.size() > TASKER_MAX_TIMES_CNT) {
            ESP_LOGD("schedule", "Time list is too long, using first %d entries", TASKER_MAX_TIMES_CNT);
            // TODO Report to user?
        }

        std::sort(time_list.begin(), time_list.end(), [](const Time& a, const Time& b) {
            if (a.hour == b.hour) {
                return a.minute < b.minute;
            }
            return a.hour < b.hour;
        });

        this->time_cnt = 0;

        for (int i = 0; i < TASKER_MAX_TIMES_CNT; i++) {
            if (i < time_list.size()) {
                this->time[i].hour = time_list[i].hour;
                this->time[i].minute = time_list[i].minute;
                this->time_cnt++;
            }
        }
    } else {
        ESP_LOGD("schedule", "Invalid format of days of week");
        // TODO Report to user
        return;
    }  
}

void Schedule::set_days_of_week_text(Text* days_of_week_text) {
    // Set up the state change callback for days_of_week_text
    days_of_week_text->add_on_state_callback([this](std::string state) {
        this->on_days_of_week_state_changed(state);
    });
}

void Schedule::set_times_text(Text* times_text) {
    // Set up the state change callback for times_text
    times_text->add_on_state_callback([this](std::string state) {
        this->on_times_state_changed(state);
    });
}

void TaskerText::setup() {
    this->pref_ = global_preferences->make_preference<char[TASKER_MAX_TEXT_LEN]>(this->get_object_id_hash(), true);

    // Need a buffer as the preferences expects a fixed size
    char buffer[TASKER_MAX_TEXT_LEN];

    if (this->pref_.load(&buffer)) {
        state.assign(buffer);
        this->publish_state(state);
    }
}

void TaskerText::control(const std::string& state) {
    ESP_LOGD("control", "New state: %s", state.c_str());

    this->publish_state(state);

    // TODO Handle string too long
    // Need a buffer as the preferences expects a fixed size
    char buffer[TASKER_MAX_TEXT_LEN];
    strncpy(buffer, state.c_str(), sizeof(buffer));

    if (!this->pref_.save(&buffer)) {
        ESP_LOGD("control", "Could not save state");
    }
}

}  // namespace tasker
}  // namespace esphome
