### **Documentation for the HeaterUart Code**

---

### **Overview**
The `HeaterUart` class is a custom ESPHome component for parsing data from a UART-connected heater. It interprets protocol frames transmitted and received over UART, extracts meaningful sensor data, and maps specific states and error codes into human-readable descriptions.

---

### **Class Members**

#### **Sensors**
- `set_temp`: Current set temperature from the heater.
- `fan_speed`: Current fan speed in RPM.
- `supply_voltage`: Voltage supplied to the heater.
- `heat_exchanger_temp`: Temperature of the heat exchanger.
- `glow_plug_voltage`: Voltage of the glow plug.
- `glow_plug_current`: Current drawn by the glow plug.
- `pump_frequency`: Pump frequency in Hz.
- `desired_temp`: Desired temperature as set by the user or controller.
- `fan_voltage`: Voltage supplied to the fan.
- `run_state_text`: Descriptive text for the heater's current state (e.g., "Running").
- `error_code_text`: Descriptive text for the last error reported by the heater.

#### **Binary Sensor**
- `on_off_state`: Indicates whether the heater is on or off.

---

### **Constructor**
```cpp
HeaterUart(UARTComponent *parent, Sensor *set_temp = nullptr, Sensor *fan_speed = nullptr,
           Sensor *supply_voltage = nullptr, Sensor *heat_exchanger_temp = nullptr,
           Sensor *glow_plug_voltage = nullptr, Sensor *glow_plug_current = nullptr,
           Sensor *pump_frequency = nullptr, Sensor *desired_temp = nullptr,
           BinarySensor *on_off_state = nullptr, Sensor *fan_voltage = nullptr,
           TextSensor *run_state_text = nullptr, TextSensor *error_code_text = nullptr)
```
- Initializes the UART component with optional sensor arguments.
- Each sensor can be linked to specific ESPHome `Sensor` or `BinarySensor` objects.

---

### **Methods**

#### `void setup()`
- Called during ESPHome setup.
- Logs that the component has been initialized.

#### `void loop()`
- Main loop that processes incoming UART data.
- Uses `frame` to store the current frame of 48 bytes.
- Ensures proper synchronization by waiting for the start byte (`0x76`).
- Verifies frame structure:
  - Checks the **Transmit Frame** (first 24 bytes).
  - Validates the **Receive Frame** (last 24 bytes).
- Calls `parse_frame()` on a valid frame.

#### `void update()`
- Publishes parsed data to ESPHome sensors.
- Checks if each sensor exists before publishing to avoid null pointer issues.

#### `void print_frame(const uint8_t *frame, size_t length, const char *label)`
- Logs the entire frame in hexadecimal format with a descriptive label.

#### `void parse_frame(const uint8_t *frame, size_t length)`
- Extracts and processes data from a valid 48-byte frame.
- **Command Frame (Bytes 0–23):**
  - `set_temperature_value`: Set temperature.
  - `desired_temperature`: Desired temperature from the controller.
- **Response Frame (Bytes 24–47):**
  - `fan_speed_value`: Fan speed in RPM.
  - `measured_voltage`: Voltage supplied to the heater.
  - `heat_exchanger_temp_value`: Heat exchanger temperature.
  - `glow_plug_voltage_value`: Glow plug voltage.
  - `glow_plug_current_value`: Glow plug current.
  - `pump_frequency_value`: Pump frequency in Hz.
  - `error_code_value`: Error code reported by the heater.
  - `run_state_value`: Current operational state of the heater.
  - `on_off_value`: Whether the heater is on (true) or off (false).
  - `fan_voltage_value`: Voltage supplied to the fan.
- Maps `run_state_value` and `error_code_value` to descriptive text using lookup tables.

#### `void reset_frame()`
- Resets the `frame_index` and waits for the next start byte (`0x76`).

---

### **Private Members**
- **Data Storage:**
  - `float` and `int` variables to store parsed sensor data.
  - `std::string` variables for state and error descriptions.
- **Frame Handling:**
  - `uint8_t frame[48]`: Buffer to hold the current 48-byte frame.
  - `int frame_index`: Tracks the current position in the frame.
  - `bool waiting_for_start`: Indicates whether the loop is waiting for the start byte (`0x76`).
- **Lookup Tables:**
  - `std::map<int, std::string> run_state_map`: Maps run state codes to descriptions.
  - `std::map<int, std::string> error_code_map`: Maps error codes to descriptions.

---

### **Data Parsing**

#### **Command Frame (Bytes 0–23)**
- **Set Temperature** (`Byte 3`): Current set temperature.
- **Desired Temperature** (`Byte 4`): Temperature set by the controller.

#### **Response Frame (Bytes 24–47)**
- **Fan Speed** (`Bytes 6–7`): RPM of the fan (16-bit value).
- **Supply Voltage** (`Bytes 4–5`): Voltage measured in volts (16-bit value, scaled by 0.1).
- **Heat Exchanger Temperature** (`Bytes 10–11`): Heat exchanger temperature.
- **Glow Plug Voltage** (`Bytes 12–13`): Glow plug voltage in volts (scaled by 0.1).
- **Glow Plug Current** (`Bytes 14–15`): Glow plug current in amperes (scaled by 0.01).
- **Pump Frequency** (`Byte 16`): Pump frequency in Hz.
- **Error Code** (`Byte 17`): Error code reported by the heater.
- **Run State** (`Byte 2`): Operational state of the heater.
- **On/Off State** (`Byte 3`): Indicates whether the heater is on (1) or off (0).
- **Fan Voltage** (`Bytes 8–9`): Voltage supplied to the fan.

---

### **Logging**
- **Start of Frame**: Logs when the start byte (`0x76`) is detected.
- **Invalid Frames**: Logs warnings for invalid transmit or receive frames.
- **Parsed Data**: Logs extracted sensor values and descriptions.

---

### **Example Usage**
- Ensure the YAML configuration links the correct ESPHome sensors to the `HeaterUart` component.
- Sensors will automatically update with parsed values and descriptions.
