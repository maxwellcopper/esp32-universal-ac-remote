# AC Universal Remote

## Overview

AC Universal Remote is an ESP32-based device that allows a cloud server to monitor and control an air conditioner through infrared (IR) communication.

The device can:

* Control various AC brands using IR commands.
* Receive control commands from a remote server.
* Report AC status to the server.
* Measure electrical current and power consumption using an SCT013 current sensor.
* Automatically reconnect to WiFi.
* Provide a WiFi configuration portal when network configuration is unavailable.
* Learn and store IR remote data during the remote scan process.

The system is designed for IoT applications where an air conditioner can be monitored and controlled remotely through a central server.

### Main Features

* Universal AC IR control
* WiFi connectivity using ESP32
* Remote command execution via HTTP
* Periodic device status reporting
* Current and power monitoring
* WiFi Manager configuration portal
* Automatic device recovery and reconnect mechanism
* LED status indication for troubleshooting

### System Workflow

1. The device connects to a configured WiFi network.
2. Every 5 seconds, the device sends its status to the server.
3. Every 2 seconds, the device sends current and power measurements.
4. Every 10 seconds, the device requests new commands from the server.
5. If a valid command is received, the device transmits the corresponding IR signal to the air conditioner.
6. If WiFi configuration is missing or WiFi connection fails, the device starts WiFi Manager mode and creates an Access Point for configuration.
7. During the first startup or after a scan reset command, the device enters Remote Scan mode to learn and store compatible IR codes.

---

## Timing Diagram

The device communicates with the server using the following intervals:

| Function     | Interval         |
| ------------ | ---------------- |
| POST Status  | Every 5 seconds  |
| POST Current | Every 2 seconds  |
| GET Command  | Every 10 seconds |

---

## LED Blink Indications

The onboard LED indicates the current device status.

| Blink Delay         | Status                                                                        |
| ------------------- | ----------------------------------------------------------------------------- |
| 100 ms              | Remote scan is running                                                        |
| 250 ms              | WiFi Manager is active (ESP32 works as an Access Point)                       |
| 500 ms              | Normal operation (WiFi connected and HTTP POST/GET handlers running normally) |
| Greater than 500 ms | WiFi connection problem or HTTP communication problem                         |

---

## Current Sensor Task

The current sensor task continuously measures current consumption.

* Sampling interval: **1 ms**
* Sampling window: **200 ms**
* These values can be changed using the macros defined in `sct013.h`.

---

## WiFi Manager Task

The WiFi Manager becomes active in the following situations:

1. The serial command `wifi reset` is executed.
2. WiFi connection fails during device startup.

### Access Point Information

When WiFi Manager is active, the ESP32 creates an Access Point with:

| Parameter          | Value                |
| ------------------ | -------------------- |
| SSID               | `ControlAC-Setup`    |
| Configuration Page | `http://192.168.4.1` |

---

## Remote Scan Task

The remote scan process becomes active in the following situations:

1. Serial command `reset` followed by `scan`.
2. First boot when no remote data is stored in flash memory.

---

## HTTP POST Status

The device sends status information to the server every 5 seconds.

### Posted Data

| Field     | Type    | Description                   |
| --------- | ------- | ----------------------------- |
| id        | String  | Device identifier             |
| lastCmdId | String  | Last executed command ID      |
| ip        | String  | Current local IP address      |
| uptimeMs  | Integer | Device uptime in milliseconds |
| fwVer     | String  | Firmware version              |
| freeHeap  | Integer | Available heap memory         |
| protocol  | String  | Detected AC protocol name     |
| P         | Boolean | AC power state                |
| temp      | Integer | AC temperature setting        |
| mode      | Integer | AC operating mode             |
| fan       | Integer | AC fan speed                  |
| swing     | Integer | AC vertical swing setting     |

Example JSON:

```json
{
  "id": "AC001",
  "lastCmdId": "123",
  "ip": "192.168.1.100",
  "uptimeMs": 123456,
  "fwVer": "1.0.0",
  "freeHeap": 180000,
  "protocol": "DAIKIN",
  "P": true,
  "temp": 24,
  "mode": 1,
  "fan": 2,
  "swing": 0
}
```

---

## HTTP POST Current

The device sends electrical measurements every 2 seconds.

### Posted Data

| Field        | Type    | Description                          |
| ------------ | ------- | ------------------------------------ |
| id           | String  | Device identifier                    |
| current      | Float   | RMS current value (A)                |
| power        | Float   | RMS power value (W)                  |
| timestamp_ms | Integer | Timestamp in milliseconds since boot |

Example JSON:

```json
{
  "id": "AC001",
  "current": 1.25,
  "power": 275.5,
  "timestamp_ms": 123456
}
```

---

## HTTP GET Command

The device retrieves AC commands from the server every 10 seconds.

### Command Fields

| Field  | Type    | Description            |
| ------ | ------- | ---------------------- |
| power  | Boolean | AC power state         |
| temp   | Integer | Temperature setting    |
| mode   | Integer | Operating mode         |
| fan    | Integer | Fan speed              |
| swingv | Integer | Vertical swing setting |

Example JSON:

```json
{
  "power": true,
  "temp": 24,
  "mode": 1,
  "fan": 2,
  "swingv": 0
}
```

---

## AC State Definitions

The device uses the common A/C state definitions provided by the IRremoteESP8266 library (`stdAc`). These values are used when sending or receiving AC commands through the HTTP API.

### Operating Mode (`mode`)

| Value | Enum  | Description           |
| ----- | ----- | --------------------- |
| -1    | kOff  | AC is turned off      |
| 0     | kAuto | Automatic mode        |
| 1     | kCool | Cooling mode          |
| 2     | kHeat | Heating mode          |
| 3     | kDry  | Dehumidification mode |
| 4     | kFan  | Fan only mode         |

Example:

```json
{
  "mode": 1
}
```

The example above sets the AC to **Cool Mode**.

---

### Fan Speed (`fan`)

| Value | Enum        | Description         |
| ----- | ----------- | ------------------- |
| 0     | kAuto       | Automatic fan speed |
| 1     | kMin        | Minimum speed       |
| 2     | kLow        | Low speed           |
| 3     | kMedium     | Medium speed        |
| 4     | kHigh       | High speed          |
| 5     | kMax        | Maximum speed       |
| 6     | kMediumHigh | Medium-High speed   |

Example:

```json
{
  "fan": 4
}
```

The example above sets the fan speed to **High**.

---

### Vertical Swing (`swingv`)

| Value | Enum         | Description           |
| ----- | ------------ | --------------------- |
| -1    | kOff         | Swing disabled        |
| 0     | kAuto        | Automatic swing       |
| 1     | kHighest     | Highest position      |
| 2     | kHigh        | High position         |
| 3     | kMiddle      | Middle position       |
| 4     | kLow         | Low position          |
| 5     | kLowest      | Lowest position       |
| 6     | kUpperMiddle | Upper-middle position |

Example:

```json
{
  "swingv": 0
}
```

The example above enables **Automatic Vertical Swing**.

---

### Complete Command Example

```json
{
  "power": true,
  "temp": 24,
  "mode": 1,
  "fan": 3,
  "swingv": 0
}
```

This command will:

* Turn the AC ON
* Set temperature to 24°C
* Set operating mode to Cool
* Set fan speed to Medium
* Enable automatic vertical swing

**Note :** Actual supported modes, fan speeds, and swing positions depend on the AC brand and protocol. Unsupported values may be ignored or automatically mapped to the closest supported setting by the IR library.