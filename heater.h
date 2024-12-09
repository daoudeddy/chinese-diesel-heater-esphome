#include "esphome.h"
#include <map>
#include <string>

/**
 * Custom Heater UART Component for ESPHome.
 * Parses a custom heater protocol via UART and maps error codes and run states.
 */
class HeaterUart : public PollingComponent, public UARTDevice {
public:
    // Template Sensors
    Sensor *set_temp;                      // Current set temperature
    Sensor *fan_speed;                     // Fan speed in RPM
    Sensor *supply_voltage;                // Voltage supplied to the heater
    Sensor *heat_exchanger_temp;           // Heat exchanger temperature
    Sensor *glow_plug_voltage;             // Glow plug voltage
    Sensor *glow_plug_current;             // Glow plug current
    Sensor *pump_frequency;                // Pump frequency in Hz
    Sensor *desired_temp;                  // Desired temperature from the controller
    Sensor *fan_voltage;                   // Voltage supplied to the fan
    TextSensor *run_state_text;            // Descriptive text for the current run state
    TextSensor *error_code_text;           // Descriptive text for the error code
    BinarySensor *on_off_state;            // Binary sensor for heater on/off state

    /**
     * Constructor with optional sensor arguments.
     */
    HeaterUart(UARTComponent *parent, Sensor *set_temp = nullptr, Sensor *fan_speed = nullptr,
               Sensor *supply_voltage = nullptr, Sensor *heat_exchanger_temp = nullptr,
               Sensor *glow_plug_voltage = nullptr, Sensor *glow_plug_current = nullptr,
               Sensor *pump_frequency = nullptr, Sensor *desired_temp = nullptr,
               BinarySensor *on_off_state = nullptr, Sensor *fan_voltage = nullptr,
               TextSensor *run_state_text = nullptr, TextSensor *error_code_text = nullptr)
        : PollingComponent(5000), UARTDevice(parent) {
        // Initialize sensors
        this->set_temp = set_temp;
        this->fan_speed = fan_speed;
        this->supply_voltage = supply_voltage;
        this->heat_exchanger_temp = heat_exchanger_temp;
        this->glow_plug_voltage = glow_plug_voltage;
        this->glow_plug_current = glow_plug_current;
        this->pump_frequency = pump_frequency;
        this->desired_temp = desired_temp;
        this->on_off_state = on_off_state;
        this->fan_voltage = fan_voltage;
        this->run_state_text = run_state_text;
        this->error_code_text = error_code_text;
    }

    /**
     * Setup function for the component.
     */
    void setup() override {
        ESP_LOGD("heater_uart", "Heater UART setup complete");
    }

    /**
     * Logs the contents of a frame as a single line of hex values.
     * @param frame - The frame array to log.
     * @param length - Length of the frame.
     * @param label - A descriptive label for the frame.
     */
    void print_frame(const uint8_t *frame, size_t length, const char *label) {
        std::string frame_data;
        frame_data.reserve(length * 3);  // Reserve space for hex representation

        for (size_t i = 0; i < length; i++) {
            char byte_str[4];
            sprintf(byte_str, "%02X ", frame[i]);  // Convert byte to hex
            frame_data.append(byte_str);
        }

        ESP_LOGD("heater_uart", "%s: [%s]", label, frame_data.c_str());
    }

    /**
     * Main loop function to process incoming UART data.
     * Reads bytes, validates frames, and parses valid frames.
     */
    void loop() override {
        const int FRAME_SIZE = 48;                // Total frame size (Tx + Rx)
        const int TX_FRAME_END_INDEX = 23;       // End index of the Transmit frame
        const int RX_FRAME_START_INDEX = 24;    // Start index of the Receive frame
        const uint8_t END_OF_FRAME_MARKER = 0x00; // Expected end of frame marker

        while (available()) {
            uint8_t byte = read();  // Read a byte from UART

            if (waiting_for_start) {
                // Check for the start of a new frame
                if (byte == 0x76) {
                    frame[frame_index++] = byte;  // Store the start byte
                    waiting_for_start = false;   // Stop waiting
                }
            } else {
                frame[frame_index++] = byte;  // Store subsequent bytes

                // Validate Transmit Frame end marker
                if (frame_index == TX_FRAME_END_INDEX + 1) {
                    if (frame[21] == END_OF_FRAME_MARKER) {
                        ESP_LOGW("heater_uart", "Invalid Transmit Packet. Resetting frame.");
                        reset_frame();
                        return;
                    }
                }

                // Check if the full frame is received
                if (frame_index == FRAME_SIZE) {
                    // Validate Receive Frame start byte and end marker
                    if (frame[45] == END_OF_FRAME_MARKER && frame[RX_FRAME_START_INDEX] == 0x76) {
                        parse_frame(frame, FRAME_SIZE);  // Parse the valid frame
                    } else {
                        ESP_LOGW("heater_uart", "Invalid Receive Packet or incorrect order. Resetting frame.");
                    }
                    reset_frame();  // Prepare for the next frame
                }
            }
        }
    }

    /**
     * Publishes parsed data to linked ESPHome sensors.
     */
    void update() override {
        if (set_temp) set_temp->publish_state(set_temperature_value);
        if (fan_speed) fan_speed->publish_state(fan_speed_value);
        if (supply_voltage) supply_voltage->publish_state(measured_voltage);
        if (heat_exchanger_temp) heat_exchanger_temp->publish_state(heat_exchanger_temp_value);
        if (glow_plug_voltage) glow_plug_voltage->publish_state(glow_plug_voltage_value);
        if (glow_plug_current) glow_plug_current->publish_state(glow_plug_current_value);
        if (pump_frequency) pump_frequency->publish_state(pump_frequency_value);
        if (fan_voltage) fan_voltage->publish_state(fan_voltage_value);
        if (run_state_text) run_state_text->publish_state(run_state_description);
        if (error_code_text) error_code_text->publish_state(error_code_description);
        if (on_off_state) on_off_state->publish_state(on_off_value);
        if (desired_temp) desired_temp->publish_state(desired_temperature_value);
    }

private:
    // Parsed data values
    float set_temperature_value = 0;
    int desired_temperature_value = 0;
    int fan_speed_value = 0;
    float measured_voltage = 0;
    float heat_exchanger_temp_value = 0;
    float glow_plug_voltage_value = 0;
    float glow_plug_current_value = 0;
    float pump_frequency_value = 0;
    int error_code_value = 0;
    int run_state_value = 0;
    int on_off_value = 0;
    float fan_voltage_value = 0;
    std::string run_state_description = "Unknown";
    std::string error_code_description = "Unknown";

    // Frame handling
    uint8_t frame[48];          // Buffer to store the current frame
    int frame_index = 0;        // Current index in the frame
    bool waiting_for_start = true; // Indicates whether waiting for the start byte

    // Maps for state and error descriptions
    const std::map<int, std::string> run_state_map = {
        {0, "Off / Standby"}, {1, "Start Acknowledge"}, {2, "Glow plug pre-heat"},
        {3, "Failed ignition - pausing for retry"}, {4, "Ignited – heating to full temp phase"},
        {5, "Running"}, {6, "Skipped – stop acknowledge?"},
        {7, "Stopping - Post run glow re-heat"}, {8, "Cooldown"}
    };

    const std::map<int, std::string> error_code_map = {
        {0, "No Error"}, {1, "No Error, but started"}, {2, "Voltage too low"},
        {3, "Voltage too high"}, {4, "Ignition plug failure"},
        {5, "Pump Failure – over current"}, {6, "Too hot"}, {7, "Motor Failure"},
        {8, "Serial connection lost"}, {9, "Fire is extinguished"}, {10, "Temperature sensor failure"}
    };

    /**
     * Parses a valid frame and extracts sensor data.
     * @param frame - The 48-byte frame to parse.
     * @param length - Length of the frame.
     */
    void parse_frame(const uint8_t *frame, size_t length) {
        if (length != 48) {
            ESP_LOGW("heater_uart", "Invalid frame length: %d bytes (expected 48)", length);
            return;
        }

        const uint8_t *command_frame = &frame[0];
        const uint8_t *response_frame = &frame[24];

        // Parse Command Frame
        set_temperature_value = command_frame[3];
        desired_temperature_value = command_frame[4];

        // Parse Response Frame
        fan_speed_value = (response_frame[6] << 8) | response_frame[7];
        measured_voltage = ((response_frame[4] << 8) | response_frame[5]) * 0.1;
        heat_exchanger_temp_value = ((response_frame[10] << 8) | response_frame[11]);
        glow_plug_voltage_value = ((response_frame[12] << 8) | response_frame[13]) * 0.1;
        glow_plug_current_value = ((response_frame[14] << 8) | response_frame[15]) * 0.01;
        pump_frequency_value = response_frame[16] * 0.1;
        error_code_value = response_frame[17];
        run_state_value = response_frame[2];
        on_off_value = response_frame[3] == 1;
        fan_voltage_value = ((response_frame[8] << 8) | response_frame[9]) * 0.1;

        // Map Run State and Error Code
        run_state_description = run_state_map.count(run_state_value)
                                    ? run_state_map.at(run_state_value)
                                    : "Unknown Run State";
        error_code_description = error_code_map.count(error_code_value)
                                     ? error_code_map.at(error_code_value)
                                     : "Unknown Error Code";
    }

    /**
     * Resets the frame index and prepares for the next frame.
     */
    void reset_frame() {
        frame_index = 0;
        waiting_for_start = true;
    }
};
