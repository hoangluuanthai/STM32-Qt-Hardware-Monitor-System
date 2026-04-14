# STM32 Integrated Hardware Monitoring & Control System

## 📝 Tổng quan
Đây là hệ thống giám sát và điều khiển thiết bị nhúng tích hợp. Dự án cho phép kết nối vi điều khiển STM32 với máy tính qua giao diện GUI để điều khiển thời gian thực, lưu trữ dữ liệu vào EEPROM và hiển thị thông số lên màn hình LCD. Dự án tập trung vào việc tối ưu hóa các giao thức truyền thông công nghiệp và quản lý tài nguyên vi điều khiển.

## 🚀 Các tính năng chính
* **Giao diện điều khiển trên PC:** Ứng dụng được viết bằng **Qt Framework (C++)**, hỗ trợ gửi lệnh điều khiển và vẽ đồ thị dữ liệu nhận được từ cảm biến/vi điều khiển.
* **Giao thức UART tùy chỉnh:** Thiết kế cấu trúc gói tin (Data Framing) riêng biệt để đảm bảo truyền nhận dữ liệu giữa vi điều khiển và máy tính luôn chính xác, không bị mất gói.
* **Quản lý bộ nhớ EEPROM (I2C):** Lập trình driver **I2C** để lưu trữ các thông số cấu hình hệ thống bền vững (không mất dữ liệu khi tắt nguồn).
* **Hiển thị LCD (SPI):** Tối ưu hóa giao tiếp **SPI** để hiển thị giao diện người dùng cục bộ trên màn hình TFT LCD với tốc độ phản hồi nhanh.
* **Xử lý tối ưu (Interrupt):** Toàn bộ hệ thống firmware được lập trình dựa trên cơ chế ngắt (Interrupt-driven) và thư viện **STM32 HAL**, giúp CPU xử lý đa tác vụ hiệu quả.

## 🛠 Công nghệ sử dụng
* **Ngôn ngữ:** Embedded C (Firmware), C++ (Qt App).
* **Vi điều khiển:** STM32 (Dòng F1/F4).
* **Giao tiếp:** UART, I2C, SPI.
* **Công cụ:** STM32CubeIDE, Qt Creator, Git/GitHub.

## 📐 Kiến trúc hệ thống
Luồng dữ liệu của hệ thống được tổ chức như sau:
`Giao diện người dùng (PC) <-> UART <-> STM32 MCU <-> EEPROM (I2C) & TFT LCD (SPI)`

## 📁 Cấu trúc thư mục
* `firmware/`: Chứa mã nguồn cho STM32 (thư mục Core, Drivers và file cấu hình `.ioc`).
* `pc-app/`: Chứa mã nguồn ứng dụng máy tính viết bằng Qt (file `CMakeLists.txt`, `.cpp`, `.h`, `.ui`).

---
**Tác giả:** Hoàng Lưu An Thái
**Lĩnh vực:** Kỹ thuật Nhúng & Tự động hóa
