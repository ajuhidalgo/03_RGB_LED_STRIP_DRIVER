
// RGB LED STRIP DRIVER FIRMWARE.

// Libraries.
#include <Arduino.h>
#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProLight.h>
#include <map>

// Global constants.
const char wifi_ssid[]  = "YOUR-WIFI-SSID";
const char wifi_pass[]  = "YOUR-WIFI-PASS";
const char app_key[]    = "YOUR-APPKEY";
const char app_secret[] = "YOUR-APPSECRET";
const char light_id[]   = "YOUR-DEVICEID";
const int baud_rate     = 115200;           // Baudrate to serial log
const int blue_pin      = 20;               // PIN for BLUE Mosfet
const int red_pin       = 22;               // PIN for RED Mosfet
const int green_pin     = 21;               // PIN for GREEN Mosfet
SinricProLight& myLight = SinricPro[light_id];  // SinricProLight device.

// Global variables.
struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

using ColorTemperatures = std::map<uint16_t, Color>; // Colortemperature lookup table.
ColorTemperatures colorTemperatures{
    //   {Temperature value, {color r, g, b}}
    {2000, {255, 138, 18}},
    {2200, {255, 147, 44}},
    {2700, {255, 169, 87}},
    {3000, {255, 180, 107}},
    {4000, {255, 209, 163}},
    {5000, {255, 228, 206}},
    {5500, {255, 236, 224}},
    {6000, {255, 243, 239}},
    {6500, {255, 249, 253}},
    {7000, {245, 243, 255}},
    {7500, {235, 238, 255}},
    {9000, {214, 225, 255}}};

struct DeviceState {                                   // Stores current device state with following initial values:
    bool  powerState       = false;                    // initial state is off
    Color color            = colorTemperatures[9000];  // color is set to white (9000k)
    int   colorTemperature = 9000;                     // color temperature is set to 9000k
    int   brightness       = 100;                      // brightness is set to 100
} device_state;


// Functions.
void setStripe() {
    int rValue = map(device_state.color.r * device_state.brightness, 0, 255 * 100, 0, 1023);  // Calculate red value and map between 0 and 1023 for analogWrite.
    int gValue = map(device_state.color.g * device_state.brightness, 0, 255 * 100, 0, 1023);  // Calculate green value and map between 0 and 1023 for analogWrite.
    int bValue = map(device_state.color.b * device_state.brightness, 0, 255 * 100, 0, 1023);  // Calculate blue value and map between 0 and 1023 for analogWrite.

    if (device_state.powerState == false) {  // Turn off?.
        digitalWrite(red_pin, LOW);          // Set.
        digitalWrite(green_pin, LOW);        // Mosfets.
        digitalWrite(blue_pin, LOW);         // Low.
    } 
    else {
        analogWrite(red_pin, rValue);       // Write red value to pin.
        analogWrite(green_pin, gValue);     // Write green value to pin.
        analogWrite(blue_pin, bValue);      // Write blue value to pin.
    }
}

bool onPowerState(const String& deviceId, bool& state) {
    device_state.powerState = state;  // Store the new power state.
    setStripe();                      // Update the mosfets.
    return true;
}

bool onBrightness(const String& deviceId, int& brightness) {
    device_state.brightness = brightness;  // Store new brightness level.
    setStripe();                           // Spdate the mosfets.
    return true;
}

bool onAdjustBrightness(const String& deviceId, int& brightnessDelta) {
    device_state.brightness += brightnessDelta;  // Calculate and store new absolute brightness.
    brightnessDelta = device_state.brightness;   // Return absolute brightness.
    setStripe();                                 // Update the mosfets.
    return true;
}

bool onColor(const String& deviceId, byte& r, byte& g, byte& b) {
    device_state.color.r = r;  // Store new red value.
    device_state.color.g = g;  // Store new green value.
    device_state.color.b = b;  // Store new blue value.
    setStripe();               // Update the mosfets.
    return true;
}

bool onColorTemperature(const String& deviceId, int& colorTemperature) {
    device_state.color            = colorTemperatures[colorTemperature];  // Set rgb values from corresponding colortemperauture.
    device_state.colorTemperature = colorTemperature;                     // Store the current color temperature.
    setStripe();                                                          // Update the mosfets.
    return true;
}

bool onIncreaseColorTemperature(const String& devceId, int& colorTemperature) {
    auto current = colorTemperatures.find(device_state.colorTemperature);  // Get current entry from colorTemperature map.
    auto next    = std::next(current);                                     // Get next element.
    if (next == colorTemperatures.end()) next = current;                   // Prevent past last element.
    device_state.color            = next->second;                          // Set color.
    device_state.colorTemperature = next->first;                           // Set colorTemperature.
    colorTemperature              = device_state.colorTemperature;         // Return new colorTemperature.
    setStripe();
    return true;
}

bool onDecreaseColorTemperature(const String& devceId, int& colorTemperature) {
    auto current = colorTemperatures.find(device_state.colorTemperature);  // Get current entry from colorTemperature map.
    auto next    = std::prev(current);                                     // Get previous element.
    if (next == colorTemperatures.end()) next = current;                   // Prevent before first element.
    device_state.color            = next->second;                          // Set color.
    device_state.colorTemperature = next->first;                           // Set colorTemperature.
    colorTemperature              = device_state.colorTemperature;         // Return new colorTemperature.
    setStripe();
    return true;
}

void setupWiFi() {
    Serial.printf("WiFi: connecting ...");
    WiFi.setSleep(false); 
    WiFi.setAutoReconnect(true);
    WiFi.begin(wifi_ssid,wifi_pass); 
    while (WiFi.status() != WL_CONNECTED) {
        Serial.printf(".");
        delay(250);
    }
    Serial.printf("connected\r\nIP is %s\r\n", WiFi.localIP().toString().c_str());
}

void setupSinricPro() {
    myLight.onPowerState(onPowerState);                              // Assign onPowerState callback.
    myLight.onBrightness(onBrightness);                              // Assign onBrightness callback.
    myLight.onAdjustBrightness(onAdjustBrightness);                  // Assign onAdjustBrightness callback.
    myLight.onColor(onColor);                                        // Assign onColor callback.
    myLight.onColorTemperature(onColorTemperature);                  // Assign onColorTemperature callback.
    myLight.onDecreaseColorTemperature(onDecreaseColorTemperature);  // Assign onDecreaseColorTemperature callback.
    myLight.onIncreaseColorTemperature(onIncreaseColorTemperature);  // Assign onIncreaseColorTemperature callback.

    SinricPro.begin(app_key,app_secret);  // start SinricPro
}


// Configuration.
void setup() {

    // UART configuration.
    Serial.begin(baud_rate);
    Serial.println("Serial communication started ...");
    
    // Pin configuration.
    pinMode(red_pin,OUTPUT);    // Set red-mosfet pin as output.
    pinMode(green_pin,OUTPUT);  // Set green-mosfet pin as output.
    pinMode(blue_pin,OUTPUT);   // Set blue-mosfet pin as output.

    // Wi-Fi connection.
    setupWiFi();

    // Sinric Pro configuration.
    setupSinricPro();
}

// Loop.
void loop() {
    SinricPro.handle();
}