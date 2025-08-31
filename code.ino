#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "Arduino.h"
#include "Base64.h"

// Camera Pin Definitions for ESP32-CAM AI-Thinker
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

// Traffic Light Pins
const int redLight = 14;
const int yellowLight = 12;
const int greenLight = 13;

// Ultrasonic Sensor Pins
const int trigPin = 15;
const int echoPin = 4;

// WiFi credentials
const char* ssid = "Realme Gt Neo 2";
const char* password = "12345678";

// Web server on port 80
WebServer server(80);

int lightState = 0; // 0=Green, 1=Yellow, 2=Red

void setup() {
  Serial.begin(115200);
  
  // Configure traffic light pins
  pinMode(redLight, OUTPUT);
  pinMode(yellowLight, OUTPUT);
  pinMode(greenLight, OUTPUT);

  // Configure ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Start with green light
  digitalWrite(greenLight, HIGH);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Start server and define routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/capture", HTTP_GET, captureAndServeImage);
  server.begin();
}

void loop() {
  manageTrafficLights();
  int distance = getDistance();
  if (distance > 0 && distance < 20 && (lightState == 1 || lightState == 2)) {
    captureAndServeImage();
  }
  server.handleClient();
  delay(1000);
}

void manageTrafficLights() {
  switch (lightState) {
    case 0: // Green Light
      digitalWrite(greenLight, HIGH);
      delay(10000);
      digitalWrite(greenLight, LOW);
      lightState = 2;
      break;
    case 1: // Yellow Light
      digitalWrite(yellowLight, HIGH);
      delay(6000);
      digitalWrite(yellowLight, LOW);
      lightState = 0;
      break;
    case 2: // Red Light
      digitalWrite(redLight, HIGH);
      delay(10000);
      digitalWrite(redLight, LOW);
      lightState = 1;
      break;
  }
}

int getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2;
}

void captureAndServeImage() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  // Encode image as base64
  String imageBase64 = base64::encode((uint8_t*)fb->buf, fb->len);
  esp_camera_fb_return(fb);

  // Create HTML page with embedded image
  String html = "<html><body style='background-color:powderblue;'>";
  html += "<h1>Captured Image</h1>";
  html += "<img src=\"data:image/jpeg;base64," + imageBase64 + "\"/>";
  html += "<p><a href=\"/capture\">Capture Another Image</a></p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleRoot() {
  String html = "<h1>Traffic Signal Monitoring System</h1><h2>Trimurti Chowk</h2><p><a href=\"/capture\">Capture Image</a></p>";
  server.send(200, "text/html", html);
}
