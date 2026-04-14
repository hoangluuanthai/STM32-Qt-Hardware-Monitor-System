# STM32 Integrated Hardware Monitoring & Control System

## 📝 Overview
Hệ thống giám sát và điều khiển tích hợp cao, cho phép kết nối trực tiếp giữa vi điều khiển STM32 và máy tính (PC). Dự án thể hiện khả năng làm chủ các chuẩn truyền thông công nghiệp cơ bản và phát triển ứng dụng GUI chuyên nghiệp để quản lý dữ liệu phần cứng.

## 🚀 Key Features
- **PC Dashboard:** Phát triển ứng dụng PC thời gian thực bằng **Qt Framework (C++)**, hỗ trợ truyền lệnh điều khiển và vẽ đồ thị dữ liệu.
- **Custom UART Protocol:** Thiết kế giao thức truyền nhận dữ liệu có cấu trúc (Framing) đảm bảo tính toàn vẹn thông tin.
- **EEPROM Management:** Lập trình driver **I2C** để lưu trữ và truy xuất các tham số cấu hình hệ thống vào bộ nhớ AT24C.
- **Visual Feedback:** Hiển thị trạng thái và thông số vận hành lên **TFT LCD** thông qua giao tiếp **SPI** tốc độ cao.
- **Optimized Firmware:** Sử dụng kiến trúc **Interrupt-driven** kết hợp thư viện **STM32 HAL** để đạt hiệu suất xử lý tối ưu.

## 🛠 Tech Stack
- **Languages:** Embedded C, C++ (Qt).
- **MCU:** STM32 (F103/F4 series).
- **Communication:** UART, I2C, SPI.
- **Tools:** STM32CubeIDE, Qt Creator, Git.

## 📐 System Architecture
Hệ thống hoạt động theo mô hình luồng dữ liệu hai chiều:
`User Interface (Qt PC) <-> UART <-> STM32 MCU <-> EEPROM (I2C) & TFT LCD (SPI)`


## 📁 Directory Structure
- `firmware/`: Toàn bộ mã nguồn STM32 (Core, Drivers, Linker script).
- `pc-app/`: Mã nguồn dự án Qt C++ (Source, Headers, UI).

---
**Author:** Hoàng Lưu An Thái  
**Field:** Embedded Systems & Automation Engineering
