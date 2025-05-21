# ESP32-CAM Sentinel Bot

A Telegram-controlled security camera system using ESP32-CAM with motion detection capabilities.

## Features
- Motion detection with PIR sensor
- Real-time photo capture and delivery via Telegram
- LED status indicators
- Remote commands through Telegram bot
- Flash control
- Message handling toggle
- System status monitoring

## Hardware Requirements
- ESP32-CAM (AI Thinker)
- PIR Motion Sensor
- LEDs (Red and Green)
- Buzzer
- Flash LED

## Setup
1. Copy `src/config.h.example` to `src/config.h`
2. Update credentials in `config.h`
3. Install required libraries:
   - UniversalTelegramBot
   - ArduinoJson
   - ESP32 Camera drivers

## Commands
- `/photo` - Take a photo
- `/flash` - Toggle flash
- `/enable` - Enable motion detection
- `/disable` - Disable motion detection
- `/messages` - Toggle message handling
- `/status` - Show system status
- `/members` - Show team members
- `/help` - Show available commands

## Team Members
- Amamence (Creator)
- Jalandoon
- Solon
- Hojland
- Villarba
