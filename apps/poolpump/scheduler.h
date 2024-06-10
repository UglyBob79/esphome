#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "esphome.h"

#include "esphome/components/number/number.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/time/real_time_clock.h"

using esphome::number::Number;
using esphome::switch_::Switch;
using esphome::time::RealTimeClock;

void check_schedule(RealTimeClock *time, Number *on_hour, Number *on_min, Number *off_hour, Number *off_min, Switch *relay) {
    ESP_LOGD("main", "Checking schedule...");

    auto now = id(time).now();

    int now_h = now.hour;
    int now_m = now.minute;

    ESP_LOGD("main", "Current time: %02d:%02d", now_h, now_m);

    int on_h = on_hour->state;
    int on_m = on_min->state;
    int off_h = off_hour->state;
    int off_m = off_min->state;

    ESP_LOGD("main", "ON scheduled at: %02d:%02d", on_h, on_m);
    ESP_LOGD("main", "OFF scheduled at: %02d:%02d", off_h, off_m);

    // If same time is set for on and off, we do nothing
    if (on_h == off_h && on_m == off_m) {
        return;
    }

    // If the pump is on, just check if it's time to turn it off and vice versa
    // Only switch state on exact time match, because we want to allow manual override and it's the simplest way to do it
    // Otherwise, the schedule would override any manual change every minute
    if (relay->state) {
        if (now_h == off_h && now_m == off_m) {
            ESP_LOGD("main", "Scheduled Pump State: OFF");
            relay->turn_off();
        }
    } else {
        if (now_h == on_h && now_m == on_m) {
            ESP_LOGD("main", "Scheduled Pump State: ON");
            relay->turn_on();
        }
    }
}

#endif // SCHEDULER_H
