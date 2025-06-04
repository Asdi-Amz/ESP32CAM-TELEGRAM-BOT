/*Asdi Amamence BSCPE 1-7
JUNE 4, 2025
PROJECT FOR PHYSICS LAB
*/

#include <stdlib.h> //for rand()
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// #define DEBUG //(for debugging, uncomment this)

#ifdef DEBUG
#define DEBUG_BEGIN(baud) Serial.begin(baud)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_BEGIN(baud) // No-op
#define DEBUG_PRINT(x)    // No-op
#define DEBUG_PRINTLN(x)  // No-op
#define DEBUG_PRINTF(...) // No-op
#endif

const char *ssid = "YOURWIFISSID";
const char *password = "YOURWIFIPASSWORD";

String BOTtoken = "XXXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
String CHAT_ID = "0000000000";
bool sendPhoto = false;
bool messageHandlingEnabled = true;

unsigned long lastPhotoTime = 0;
const unsigned long photoCooldown = 7000; // 7 seconds cooldown

const byte redLed = 2;    // red led
const byte greenLed = 13; // green led
const byte sensPin = 14;  // motion sensor
const byte buzzer = 15;   // buzzer

bool isGreenOn = false;

static WiFiClientSecure clientTCP;
static UniversalTelegramBot bot(BOTtoken, clientTCP);

#define FLASH_LED_PIN 4
bool flashState = LOW;

const char *motionMessages[] = {
    " ⚠️ Intruder detected... You are not alone.",
    " 🚨 Unwelcome presence... stay on high alert.",
    " 👁️ Watching you now... movement has been seen.",
    " 🕯️ Something moves close... you're not alone.",
    " 💀 Entity detected... Remain where you are.",
    " 👣 Steps in darkness... none should be there.",
    " 🔦 Shadows shifting... You're being observed.",
    " 👻 You feel it too... Something's out there.",
    " 📸 Camera triggered... something is nearby.",
    " 🧍 Figure detected... This is no false alarm."};

const int numMessages = sizeof(motionMessages) / sizeof(motionMessages[0]);

int msgIndex;

// Replace the existing command definitions with:
const String PHOTO_CMD = "/photo";
const String FLASH_CMD = "/flash";
const String ENABLE_CMD = "/enable";
const String DISABLE_CMD = "/disable";
const String STATUS_CMD = "/status";
const String HELP_CMD = "/help";
const String MEMBERS_CMD = "/members";
const String MSG_TOGGLE_CMD = "/messages";

bool motionEnabled = true;

// Add with other global definitions
const char *teamMembers[] = {
    "👨‍💻 Amamence (Creator)",
    "👨‍💻 Jalandoon",
    "👨‍💻 Solon",
    "👨‍💻 Hojland",
    "👨‍💻 Villarba"};

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

void configInitCamera()
{
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
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;

  // Balanced High Quality
  if (psramFound())
  {
    config.frame_size = FRAMESIZE_SXGA; // 1280x1024
    config.jpeg_quality = 12;           // Good quality
    config.fb_count = 2;                // Double buffering
  }
  /*
  if (psramFound())
  {
    config.frame_size = FRAMESIZE_VGA; // Smaller size (640x480)
    config.jpeg_quality = 12;          // Lower quality = smaller file
    config.fb_count = 2;               // Double buffering
  }
  */
  else
  {
    config.frame_size = FRAMESIZE_CIF; // Even smaller (352x288)
    config.jpeg_quality = 15;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK)
  {
    DEBUG_PRINTF("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }
}

String sendPhotoTelegram()
{
  const char *myDomain = "api.telegram.org";
  static char getAll[150]; // Static buffer for better memory management
  static char getBody[1024];
  getAll[0] = '\0';
  getBody[0] = '\0';

  // Dispose first picture and get a new one with flash
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb)
    esp_camera_fb_return(fb);

  digitalWrite(FLASH_LED_PIN, HIGH);
  fb = esp_camera_fb_get();
  digitalWrite(FLASH_LED_PIN, LOW);

  if (!fb)
  {
    DEBUG_PRINTLN("Camera capture failed");
    return "Camera capture failed";
  }

  /*
  if (!clientTCP.connect(myDomain, 443))
  {
    esp_camera_fb_return(fb);
    return "Connection to api.telegram.org failed";
  }
  */

  // Replace the connection check with a more efficient one
  if (!clientTCP.connected())
  {
    if (!clientTCP.connect(myDomain, 443))
    {
      esp_camera_fb_return(fb);
      return "Connection to api.telegram.org failed";
    }
  }
  /*
  // Prepare HTTP headers
  const String head =
      "--AsdiAmz\r\n"
      "Content-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" +
      CHAT_ID +
      "\r\n--AsdiAmz\r\nContent-Disposition: form-data; name=\"photo\"; "
      "filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  */

  // HTTP Headers
  msgIndex = rand() % numMessages;
  const String head =
      "--AsdiAmz\r\n"
      "Content-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" +
      CHAT_ID +
      "\r\n--AsdiAmz\r\n"
      "Content-Disposition: form-data; name=\"caption\"; \r\n\r\n" +
      String(motionMessages[msgIndex]) +
      "\r\n--AsdiAmz\r\n"
      "Content-Disposition: form-data; name=\"photo\"; "
      "filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";

  const String tail = "\r\n--AsdiAmz--\r\n";

  size_t imageLen = fb->len;
  size_t totalLen = head.length() + imageLen + tail.length();

  // Send HTTP POST request
  clientTCP.println("POST /bot" + BOTtoken + "/sendPhoto HTTP/1.1");
  clientTCP.println("Host: " + String(myDomain));
  clientTCP.println("Content-Length: " + String(totalLen));
  clientTCP.println("Content-Type: multipart/form-data; boundary=AsdiAmz");
  clientTCP.println();
  clientTCP.print(head);

  /*

  // Send image data in larger chunks for better performance
  uint8_t *fbBuf = fb->buf;
  const size_t bufferSize = 4096; // Increased buffer size
  size_t fbLen = fb->len;

  for (size_t n = 0; n < fbLen; n += bufferSize)
  {
    if (n + bufferSize < fbLen)
    {
      clientTCP.write(fbBuf, bufferSize);
      fbBuf += bufferSize;
    }
    else
    {
      size_t remainder = fbLen % bufferSize;
      clientTCP.write(fbBuf, remainder);
    }
    // Add yield() to prevent watchdog timer issues
    yield();
  }

*/

  const size_t bufferSize = 1024; // Smaller 1KB chunks for slow connections
  size_t fbLen = fb->len;
  uint8_t *fbBuf = fb->buf;

  for (size_t n = 0; n < fbLen; n += bufferSize)
  {
    size_t thisChunkSize = min(bufferSize, fbLen - n);
    clientTCP.write(fbBuf + n, thisChunkSize);

    // More frequent yields
    if (n % (bufferSize * 4) == 0)
    {
      yield();
    }
  }

  clientTCP.print(tail);
  esp_camera_fb_return(fb);

  // Wait for response with timeout
  unsigned long timeout = millis() + 6000;
  bool state = false;

  while (millis() < timeout)
  {
    while (clientTCP.available())
    {
      char c = clientTCP.read();
      if (state)
      {
        if (strlen(getBody) < sizeof(getBody) - 1)
          strncat(getBody, &c, 1);
      }
      if (c == '\n')
      {
        if (strlen(getAll) == 0)
          state = true;
        getAll[0] = '\0';
      }
      else if (c != '\r')
      {
        if (strlen(getAll) < sizeof(getAll) - 1)
          strncat(getAll, &c, 1);
      }
      timeout = millis() + 1000; // Reset timeout on data receive
    }
    if (strlen(getBody) > 0)
      break;
    delay(10); // Small delay to prevent CPU hogging
  }

  clientTCP.stop();

  // Success indication
  if (strlen(getBody) > 0)
  {
    DEBUG_PRINTLN("Photo sent successfully!");
    // Double beep
    for (int i = 0; i < 2; i++)
    {
      digitalWrite(buzzer, HIGH);
      delay(100);
      digitalWrite(buzzer, LOW);
      delay(100);
    }
  }

  return String(getBody);
}

void handleNewMessages(int numNewMessages)
{
  if (numNewMessages == 0)
    return;

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    if (chat_id != CHAT_ID)
    {
      bot.sendMessage(chat_id, "⛔ Unauthorized user", "");
      continue;
    }

    if (text == PHOTO_CMD)
    {
      bot.sendMessage(chat_id, "📸 Taking photo...", "");
      sendPhotoTelegram();
    }

    else if (text == FLASH_CMD)
    {
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
      bot.sendMessage(chat_id,
                      String("💡 Flash: ") + (flashState ? "ON ✅" : "OFF ❌"), "");
    }

    else if (text == ENABLE_CMD)
    {
      if (!motionEnabled)
      {
        motionEnabled = true;
        digitalWrite(greenLed, HIGH);
        bot.sendMessage(chat_id, "🟢 Motion detection enabled!", "");
      }
      else
      {
        bot.sendMessage(chat_id, "ℹ️ Motion detection already enabled", "");
      }
    }

    else if (text == DISABLE_CMD)
    {
      if (motionEnabled)
      {
        motionEnabled = false;
        digitalWrite(greenLed, LOW);
        bot.sendMessage(chat_id, "🔴 Motion detection disabled!", "");
      }
      else
      {
        bot.sendMessage(chat_id, "ℹ️ Motion detection already disabled", "");
      }
    }

    else if (text == STATUS_CMD)
    {
      String status = "📊 System Status:\n";
      status += "Motion Detection: " + String(motionEnabled ? "Enabled ✅" : "Disabled ❌") + "\n";
      status += "Message Handling: " + String(messageHandlingEnabled ? "Enabled ✅" : "Disabled ❌") + "\n";
      status += "Flash: " + String(flashState ? "ON ✅" : "OFF ❌") + "\n";
      status += "WiFi Signal: " + String(WiFi.RSSI()) + " dBm 📶\n";
      status += "Uptime: " + String(millis() / 1000) + " seconds ⏱️\n";
      status += "Last Photo: " + String((millis() - lastPhotoTime) / 1000) + "s ago 📸";
      bot.sendMessage(chat_id, status, "");
    }

    // Modify the HELP_CMD section
    else if (text == HELP_CMD)
    {
      String help = "🔰 Available Commands:\n\n";
      help += PHOTO_CMD + " - Take a photo 📸\n";
      help += FLASH_CMD + " - Toggle flash 💡\n";
      help += ENABLE_CMD + " - Enable motion detection 🟢\n";
      help += DISABLE_CMD + " - Disable motion detection 🔴\n";
      help += MSG_TOGGLE_CMD + " - Toggle message handling 💬\n";
      help += STATUS_CMD + " - Show system status 📊\n";
      help += MEMBERS_CMD + " - Show project team members 👥\n";
      help += HELP_CMD + " - Show this help message ℹ️";
      bot.sendMessage(chat_id, help, "");
    }

    // Add this else if block in handleNewMessages() before the else block
    else if (text == MEMBERS_CMD)
    {
      String members = "👥 Project Team Members:\n\n";
      for (const char *member : teamMembers)
      {
        members += String(member) + "\n";
      }
      members += "\n🏆 ESP32-CAM Sentinel Bot Project";
      bot.sendMessage(chat_id, members, "");
    }

    else if (text == MSG_TOGGLE_CMD)
    {
      messageHandlingEnabled = !messageHandlingEnabled;
      String status;

      // Auto-enable motion detection if both would be disabled
      if (!messageHandlingEnabled && !motionEnabled)
      {
        motionEnabled = true;
        status = "⚠️ Message handling disabled\n"
                 "Motion detection automatically enabled for security\n"
                 "System running in motion-only mode 🔒";
      }
      else
      {
        status = messageHandlingEnabled ? "✅ Message handling enabled" : "❌ Message handling disabled (Motion only mode)";
      }

      bot.sendMessage(chat_id, status, "");
    }

    else
    {
      String unknownCmd = "❓ Unknown command. Send " + HELP_CMD + " for available commands.";
      bot.sendMessage(chat_id, unknownCmd, "");
    }
    // Small yield to prevent watchdog triggers
    yield();
  }
}

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  DEBUG_BEGIN(115200);
  bool redLedActive = false;

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, flashState);

  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(sensPin, INPUT);

  // Two beeps before PIR initialization
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(redLed, LOW);
    digitalWrite(buzzer, LOW);
    delay(100);
  }
  // Replace the single delay with this code in setup():
  DEBUG_PRINTLN("Initializing PIR sensor (90 seconds)...");

  const unsigned long INIT_TIME = 90000;     // 1.5 minutes in milliseconds
  const unsigned long BLINK_INTERVAL = 2000; // 2 seconds per blink cycle
  unsigned long startTime = millis();
  unsigned long lastBlink = 0;
  bool ledState = false;

  while (millis() - startTime < INIT_TIME)
  {
    // Blink every 2 seconds
    if (millis() - lastBlink >= BLINK_INTERVAL)
    {
      ledState = !ledState;
      digitalWrite(redLed, ledState);
      DEBUG_PRINT("."); // Progress indicator
      lastBlink = millis();
    }
    yield(); // Prevent watchdog trigger
  }

  digitalWrite(redLed, LOW); // Ensure LED is off after initialization
  DEBUG_PRINTLN("\nPIR initialization complete!");

  // Three beeps after PIR initialization
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
    delay(100);
  }

  configInitCamera();

  WiFi.mode(WIFI_STA);
  DEBUG_PRINTLN();
  DEBUG_PRINT("Connecting to ");
  DEBUG_PRINTLN(ssid);
  WiFi.begin(ssid, password);

  // clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  // With these lines:
  clientTCP.setInsecure();  // Skip certificate validation
  clientTCP.setTimeout(10); // Faster timeout for better response

  while (WiFi.status() != WL_CONNECTED)
  {
    redLedActive = !redLedActive;
    digitalWrite(redLed, redLedActive ? HIGH : LOW);
    DEBUG_PRINT(".");
    delay(500);
  }

  digitalWrite(redLed, LOW);

  String welcome = "🤖 Sentinel Bot is now Online! 👁️\n\n";
  welcome += "Available Commands:\n";
  welcome += PHOTO_CMD + " - Take a photo 📸\n";
  welcome += FLASH_CMD + " - Toggle flash 💡\n";
  welcome += ENABLE_CMD + " - Enable motion detection 🟢\n";
  welcome += DISABLE_CMD + " - Disable motion detection 🔴\n";
  welcome += MSG_TOGGLE_CMD + " - Enable/Disable Message Handling 💬\n";
  welcome += STATUS_CMD + " - Show system status 📊\n";
  welcome += MEMBERS_CMD + " - Show project team members 👥\n";
  welcome += HELP_CMD + " - Show this help message ℹ️";

  messageHandlingEnabled = true;

  bot.sendMessage(CHAT_ID, welcome, "");

  digitalWrite(buzzer, HIGH);
  delay(300);
  digitalWrite(buzzer, LOW);
  digitalWrite(greenLed, HIGH);
}

// Add these constants at the top with other globals
const unsigned long LED_BLINK_INTERVAL = 500; // LED blink interval in ms
const unsigned long MOTION_RESET_TIME = 3000; // Time to wait before considering new motion
unsigned long lastGreenBlink = 0;
bool isMotionActive = false;

const unsigned long ALTERNATE_BLINK_INTERVAL = 500; // Faster blink for disabled state
unsigned long lastAlternateBlink = 0;
bool alternateState = false;

const unsigned long MSG_CHECK_TIMEOUT = 1000;  // Max time for message check
const unsigned long MSG_HANDLE_TIMEOUT = 2000; // Max time for message handling

const unsigned long BOT_CHECK_INTERVAL = 5000; // Check messages every 5000ms
unsigned long lastBotCheckTime = 0;

void loop()
{
  unsigned long currentMillis = millis();
  static unsigned long operationStart = currentMillis;

  // 1. LED Updates - Always run these first for consistent blinking
  if (motionEnabled)
  {
    // Normal operation - blink green
    if (currentMillis - lastGreenBlink >= LED_BLINK_INTERVAL)
    {
      isGreenOn = !isGreenOn;
      digitalWrite(greenLed, isGreenOn ? HIGH : LOW);
      lastGreenBlink = currentMillis;
    }
  }
  else
  {
    // Motion detection disabled - alternate blinking
    if (currentMillis - lastAlternateBlink >= ALTERNATE_BLINK_INTERVAL)
    {
      alternateState = !alternateState;
      digitalWrite(greenLed, alternateState ? HIGH : LOW);
      digitalWrite(redLed, alternateState ? LOW : HIGH);
      lastAlternateBlink = currentMillis;
    }
  }

  // 2. Check Telegram messages with timeout protection
  if (currentMillis - lastBotCheckTime > BOT_CHECK_INTERVAL)
  {
    if (messageHandlingEnabled && WiFi.status() == WL_CONNECTED &&
        (currentMillis - operationStart < MSG_CHECK_TIMEOUT))
    {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      if (numNewMessages)
      {
        handleNewMessages(numNewMessages);
      }
    }
    lastBotCheckTime = currentMillis;
  }

  // 4. Motion Detection
  if (motionEnabled)
  {
    if (digitalRead(sensPin) == HIGH)
    {
      if (!isMotionActive && !sendPhoto &&
          (currentMillis - lastPhotoTime > photoCooldown))
      {

        isMotionActive = true;
        sendPhoto = true;
        operationStart = currentMillis; // Reset operation timer

        // Quick alert pattern
        digitalWrite(greenLed, LOW);
        digitalWrite(redLed, HIGH);
        digitalWrite(buzzer, HIGH);
        delay(100); // Minimal blocking delay
        digitalWrite(buzzer, LOW);

        // Take and send photo
        if (currentMillis - operationStart < MSG_HANDLE_TIMEOUT)
        {
          sendPhotoTelegram();

          // Send random message
          // msgIndex = rand() % numMessages;
          // bot.sendMessage(CHAT_ID, motionMessages[msgIndex]);
        }

        // Completion indication
        digitalWrite(buzzer, HIGH);
        delay(100);
        digitalWrite(buzzer, LOW);
        digitalWrite(redLed, LOW);

        sendPhoto = false;
        lastPhotoTime = currentMillis;
      }
    }
    else
    {
      // Reset motion state after delay
      if (isMotionActive && (currentMillis - lastPhotoTime > MOTION_RESET_TIME))
      {
        isMotionActive = false;
      }
    }
  }

  // 5. System protection
  if (currentMillis - operationStart > MSG_HANDLE_TIMEOUT)
  {
    operationStart = currentMillis; // Reset operation timer
  }

  yield(); // Give system time to handle background tasks
}