#define CAMERA_MODEL_AI_THINKER
#include "time.h"      // For setting up NTP time synchronization
#include "SPI.h"       // Required for SD functionality on ESP32
#include "esp_camera.h" // ESP32 camera library
#include <WiFi.h>       // WiFi library
#include <ESP_Mail_Client.h> // ESP Mail Client for sending emails
#include "camera_pins.h"  // Camera pin definitions
#include "SD_MMC.h"   // Use SD_MMC instead of "SD.h" for ESP32
#include "LittleFS.h" // File system for ESP32 (for storing logs, etc.)

// SMTP (Email) Configuration
#define SMTP_HOST "smtp.gmail.com"  // SMTP server for Gmail
#define SMTP_PORT 465  // 465 for SSL, 587 for TLS
#define AUTHOR_EMAIL "seanjeffersoncalaoagan@gmail.com"  // Sender email
#define AUTHOR_PASSWORD "bboi folo rgpb xxxc"  // Sender email password
#define RECIPIENT_EMAIL "xianshizukana+misc@gmail.com"  // Recipient email

// Email session configuration
Session_Config config;
SMTPSession smtp;

// WiFi Credentials
const char* ssid = "TP-Link_54A6";
const char* password = "81076987";

// Function Prototypes
void sendEmail(uint8_t* image_data, size_t image_size);
void startCameraServer();
void setupLedFlash(int pin);

// Function to set the correct time using NTP
void setClock() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Waiting for NTP time sync...");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {  // Wait for a valid time
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("\nCurrent time: ");
    Serial.println(asctime(&timeinfo));
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    // Camera Configuration
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
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_QVGA;
    config.pixel_format = PIXFORMAT_JPEG;
    config.fb_location = CAMERA_FB_IN_DRAM; 
    config.jpeg_quality = 12;
    config.fb_count = 2;

    // Initialize Camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    // Mount LittleFS (File System)
    if (!LittleFS.begin(true)) { 
        Serial.println("Failed to mount LittleFS, formatting...");
    } else {
        Serial.println("LittleFS mounted successfully!");
    }

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    // Set time using NTP
    setClock();

    // Start Camera Server
    startCameraServer();
    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
}

void loop() {
    delay(1000);
    
    // Capture image
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }
    
    // Send the captured image via email
    sendEmail(fb->buf, fb->len);
    
    // Release the frame buffer
    esp_camera_fb_return(fb);
    
    // Wait 60 seconds before sending the next email
    delay(60000);
}

// Function to send an email with the captured image
void sendEmail(uint8_t* image_data, size_t image_size) {
    smtp.debug(1);  // Enable debugging
    smtp.callback([](SMTP_Status status){
        Serial.println(status.info());
    });

    // Configure email session
    Session_Config config;
    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;
    config.login.user_domain = "";
    
    // Create email message
    SMTP_Message message;
    message.sender.name = "ESP32-CAM";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "ESP32-CAM Capture";
    message.addRecipient("Recipient", RECIPIENT_EMAIL);
    message.text.content = "Captured image from ESP32-CAM";

    // Attach the image
    SMTP_Attachment att;
    att.descr.filename = "esp32_photo.jpg";
    att.descr.mime = "image/jpeg";
    att.blob.data = image_data;
    att.blob.size = image_size;
    message.addAttachment(att);

    // Connect to mail server
    if (!smtp.connect(&config)) {
        Serial.println("Could not connect to mail server");
        return;
    }

    // Send the email
    if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println("Email send failed");
    } else {
        Serial.println("Email sent successfully");
    }
}
