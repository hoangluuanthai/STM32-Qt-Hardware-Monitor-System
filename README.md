# STM32 Integrated Hardware Monitoring & Control System

## 📝 Overview
This project presents an integrated embedded solution for real-time hardware monitoring and control. By bridging an **STM32 microcontroller** with a custom **PC-side GUI (Qt)**, the system enables bidirectional communication, non-volatile data storage via **EEPROM (I2C)**, and localized visual feedback on a **TFT LCD (SPI)**. The project emphasizes industrial communication protocols, optimized resource management, and robust firmware architecture.

## 🚀 Key Features
* **Qt-Based PC Dashboard:** A professional cross-platform application developed in **C++ (Qt Framework)** for real-time telemetry visualization and remote command transmission.
* **Custom UART Protocol:** Engineered a dedicated **Data Framing protocol** to ensure high-reliability transmission between the MCU and PC, maintaining data integrity and preventing packet loss.
* **Non-volatile Storage (I2C):** Developed optimized **I2C drivers** to manage system configurations and historical parameters stored in an **EEPROM (AT24C series)**.
* **High-Speed SPI Display:** Implemented low-latency **SPI communication** for a local TFT LCD interface, providing immediate system status updates and user feedback.
* **Interrupt-Driven Firmware:** The entire firmware is architected using **STM32 HAL** and an **interrupt-based model**, ensuring efficient CPU utilization and real-time multitasking responsiveness.

## 🛠 Technology Stack
* **Languages:** Embedded C (Firmware), C++ (Qt Application).
* **Microcontroller:** STM32 (F1/F4 Series).
* **Communication Protocols:** UART (Custom Framing), I2C, SPI.
* **Development Tools:** STM32CubeIDE, Qt Creator, Git/GitHub.

## 📐 System Architecture
The system data flow is organized as follows:
`User Interface (PC) <-> UART (Custom Protocol) <-> STM32 MCU <-> EEPROM (I2C) & TFT LCD (SPI)`

## 📁 Directory Structure
* `firmware/`: Contains the complete STM32 source code (Core, Drivers, and `.ioc` configuration).
* `pc-app/`: Contains the Qt-based PC monitoring application (Source code, Headers, and UI files).

---
**Author:** Hoang Luu An Thai  
**Field:** Automation & Control Engineering
