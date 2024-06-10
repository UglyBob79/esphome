# ESPHome Projects

This repository contains ESPHome configurations for my home automation projects. Currently, it includes the following projects:
- Pool Pump Controller
- Irrigation Controller

## Projects

### Pool Pump Controller

**Description**: The Pool Pump Controller manages the pool pump's operation based on a simple schedule or manual control. It ensures efficient energy usage and optimal pool maintenance. Currently the scheduling is done by setting start and stop time values (hour/minute), at which time the controllers relay will open and close respectively. The Pool Pump Controller is based on a Sonoff Basic R2, but it can be easily modified to run on any relay board.

**Features**:
- Scheduled operation
- Manual override via Home Assistant
- Status monitoring

### Irrigation Controller

**Description**: The Irrigation Controller automates the watering schedule for your garden or lawn. It supports multiple zones and allows for flexible scheduling using Home Assistant. Currently the functionality is very simple and relies on Home Assistant to do the actual scheduling. The device itself exposes a service where a zone can be triggered open for a duration, then it will automatically close. This implementation choice was made to prevent any valve being left open in case Home Assistant goes down. The irrigation controller is based on a Sonoff 4CH Pro, but it can be easily modified to run on any relay board. 

**Features**:
- Multi-zone control
- Home Assistant controlled watering schedules
- Integration with weather data for smart watering (TODO)

## Installation

### Prerequisites

1. **ESPHome**: Ensure you have ESPHome installed. Follow the [installation guide](https://esphome.io/guides/installing_esphome.html) if needed.
2. **Home Assistant**: These projects integrate with Home Assistant. Ensure Home Assistant is set up and running.

### Cloning the Repository

Clone this repository to your local machine:

```bash
git clone https://github.com/UglyBob79/esphome.git
cd esphome
```


