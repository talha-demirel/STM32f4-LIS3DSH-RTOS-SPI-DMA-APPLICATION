# High-Performance Accelerometer Data Logger (STM32 + FreeRTOS + LIS3DSH)

![Language](https://img.shields.io/badge/language-C-blue.svg) ![Platform](https://img.shields.io/badge/platform-STM32-orange.svg) ![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-green.svg) ![License](https://img.shields.io/badge/license-MIT-lightgrey.svg)

## 📌 Project Overview

This project implements a real-time, high-performance 3-axis accelerometer data logging system using the **STM32F4** microcontroller and the **LIS3DSH** MEMS sensor. 

Unlike simple polling methods, this system utilizes **FreeRTOS** with a Producer-Consumer architecture, **DMA (Direct Memory Access)**, and **FIFO Burst Reads** to minimize CPU intervention. It is designed to handle high-data-rate sensors without blocking the processor, minimize data loss under high data rates.

## 🚀 Key Features

* **Real-Time OS:** Built on **FreeRTOS** with preemptive scheduling.
* **Zero-Copy Data Transfer:** Uses **SPI DMA** to read sensor data and **UART DMA** to transmit logs, keeping the CPU free for other tasks.
* **Producer-Consumer Architecture:**
    * **Producer Task:** Triggered by hardware interrupts (Data Ready/Watermark), reads data via SPI DMA.
    * **Consumer Task:** Processes raw data, converts to G-force values, and streams via UART.
* **Robust Synchronization:** Uses **Binary Semaphores** for DMA completion signaling and **Queues** for inter-task communication.
* **Error Handling:** Implements a robust status-check mechanism. The system verifies the LIS3DSH Chip ID (WHO_AM_I) before initialization and monitors SPI communication timeouts. It also handles DMA transmission errors via HAL callbacks to prevent task deadlocks.
* **Power Efficient:** The MCU sleeps (Idle Task) when not processing data interrupts.

## 🛠 Hardware & Software
* **Microcontroller:** STM32F407G-DISC1 (STM32F4 Discovery Board)
* **Sensor:** LIS3DSH MEMS Accelerometer (SPI Interface)
* **IDE:** STM32CubeIDE
* **Framework:** STM32 HAL Library + FreeRTOS

## ⚙️ System Architecture

1.  **Initialization:** The system configures the LIS3DSH in **Stream Mode** with a Watermark threshold of 10 samples.
2.  **Interrupt:** When FIFO reaches the threshold, the sensor triggers an **EXTI Interrupt**.
3.  **Task Notification:** The ISR notifies the *Producer Task* immediately.
4.  **Burst Read (DMA):** The *Producer Task* wakes up and initiates an SPI DMA transfer to read X, Y, Z data in one burst.
5.  **Queue:** Processed data is pushed into a FreeRTOS Queue.
6.  **Transmission:** The *Consumer Task* pulls data from the Queue and transmits it to the PC via UART DMA.

## 🔌 Pin Configuration (Default for Discovery Board)

| Pin | Function | Description |
| :--- | :--- | :--- |
| **PA5** | SPI1_SCK | Sensor Clock |
| **PA6** | SPI1_MISO | Sensor Data Out |
| **PA7** | SPI1_MOSI | Sensor Data In |
| **PE3** | CS_PIN | Chip Select |
| **PE0** | MEMS_INT1 | Data Ready/FIFO Interrupt |
| **PA2** | USART2_TX | Serial Debug Output (Connect to PC) |


## 📉 Visualizing Data

The data is output via UART (Baud Rate: 115200) in the following format:

```text
x-> 0.02
y-> -0.05
z-> 0.98
```

You can use standard tools like Putty, TeraTerm, or SerialPlot to view or graph the incoming data in real-time.

## 🔧 How to Run

Clone this repository:

```bash
git clone https://github.com/talha-demirel/STM32f4-LIS3DSH-RTOS-SPI-DMA-APPLICATION.git
```
Open the project in STM32CubeIDE.

Build the project (Hammer icon).

Connect your STM32 board and Flash the code (Run icon).

Open a Serial Terminal (Baud: 115200) to see the accelerometer data.

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.
