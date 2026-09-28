# STM32 FreeRTOS BMS Telemetry System

This is an end-to-end simulated Battery Management System project. An STM32F103C8 node runs FreeRTOS tasks at 100 Hz to generate 12 cell voltages, pack current, and four NTC temperatures, then performs fixed-point filtering and protection evaluation before transmitting telemetry over Classical CAN 2.0B. On Linux, a SocketCAN gateway assembles complete snapshots, while a loadable kernel module filters frames, maintains a ring buffer, and exposes diagnostics through debugfs.


## Validation Status

The following paths have been executed successfully:

| Validation | Result |
|---|---|
| Keil Arm Compiler 6 build | 0 errors |
| Keil CPU Simulator software loopback and fault injection | Passed |
| Host CMake/CTest unit tests | Passed |
| GitHub Actions | Passed |
| WSL SocketCAN `vcan0` userspace gateway | Passed with complete JSON output |
| Buildroot 2025.02 LTS | AArch64 image generated successfully |
| QEMU AArch64 | Linux 6.6.52 booted successfully |
| `bms_can_monitor.ko` | Loaded and monitored IDs `0x300-0x307` |
| debugfs ring buffer | 8 frames received, 0 overwritten, capacity 128 |

Example telemetry output:

```json
{"sequence":1,"uptime_ms":10000,"cells_mv":[3650,3653,3656,3659,3662,3665,3668,3671,3674,3677,3680,3683],"pack_mv":43998,"current_a":-12.50,"ntc_c":[25.0,25.5,26.0,26.5],"min_cell_mv":3650,"max_cell_mv":3683,"faults":0,"dropped":0,"state":0}
```

Kernel module statistics:

```text
received=8
overwritten=0
buffered=8
capacity=128
```

## Implemented Features

- STM32F103C8, CMSIS-RTOS v1, and FreeRTOS `heap_4`
- period 10 ms sampling
- Simulation of 12 cells, pack current, and four NTC channels
- Fixed-point first-order IIR filtering without a floating-point hardware dependency
- Overvoltage, undervoltage, overtemperature, charge overcurrent, and discharge overcurrent protection with debounce and recovery hysteresis
- Priority-based tasks, queues, task notifications, and ISR-safe reception
- Classical CAN 2.0B, a 500 kbit/s configuration, and an eight-frame telemetry protocol
- SocketCAN gateway that assembles complete snapshots by 16-bit sequence number
- Linux 6.6 kernel module subscribing to `0x300-0x307` with a 128-frame ring buffer
- Kernel frame records and statistics exported through debugfs
- Buildroot external tree integrating the gateway, kernel module, can-utils, and startup script

## Repository Layout

```text
.
├── firmware
│   ├── App                 BMS logic and CAN protocol
│   ├── Core                STM32 startup, FreeRTOS tasks, and interrupts
│   ├── Drivers             CMSIS and STM32F1 HAL
│   ├── Middlewares         FreeRTOS and CMSIS-RTOS v1
│   ├── MDK-ARM             Keil Arm Compiler 6 project
│   └── BMS_F103.ioc        STM32CubeMX configuration
├── linux
│   ├── userspace           SocketCAN JSON gateway
│   ├── kernel              CAN monitoring kernel module
│   ├── buildroot-external  QEMU AArch64 Buildroot external tree
│   └── scripts             vcan demonstration frames
└── tests                   Portable core-logic tests

```

## Quick Start

### 1. Host Unit Tests

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### 2. Keil CPU Simulator

1. Install Keil MDK, Arm Compiler 6, and `Keil::STM32F1xx_DFP` 2.4.1.
2. Open `firmware/MDK-ARM/BMS_F103.uvprojx`.
3. Select `BMS_F103_Keil_Simulator` and rebuild the target.
4. Enter Debug mode, select Simulator, and run.
5. Observe `g_loopback_rx_count`, `g_last_fault_flags`, and `g_dropped_frames`.

The simulation repeats every 30 seconds: cell 5 is driven into overvoltage from 10-12 seconds, and NTC channel 2 is driven into overtemperature from 20-22 seconds.

### 3. Linux SocketCAN

```sh
make -C linux/userspace
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set dev vcan0 up
./linux/userspace/bms-gateway vcan0
```

Send the demonstration frames from another terminal:

```sh
sh linux/scripts/send_demo_frames.sh vcan0
```

### 4. Buildroot and QEMU

```sh
git clone --depth 1 --branch 2025.02.x \
  https://gitlab.com/buildroot.org/buildroot.git buildroot-2025.02
cd buildroot-2025.02
make BR2_EXTERNAL=/path/to/stm32-freertos-bms/linux/buildroot-external \
  bms_qemu_aarch64_defconfig
make -j"$(nproc)"
```

Start QEMU:

```sh
qemu-system-aarch64 -M virt -cpu cortex-a53 -smp 2 -m 512 -nographic \
  -kernel output/images/Image \
  -append "root=/dev/vda rw rootwait console=ttyAMA0" \
  -drive file=output/images/rootfs.ext2,if=none,format=raw,id=hd0 \
  -device virtio-blk-device,drive=hd0
```

## Design Decisions

- **Classical CAN instead of CAN-FD:** STM32F103 bxCAN supports Classical CAN only, so every frame is strictly limited to eight data bytes.
- **Software loopback as the default simulation backend:**  FreeRTOS queues simulate CAN transmission and reception in Keil to verify the software communication flow.
- **Statically bounded resources:** Sample queue length 4, snapshot queue length 1, receive queue length 16, and kernel ring capacity 128.
- **Protection independent of communication:** Protection evaluation never waits for CAN; communication congestion only increments a drop counter.
- **Sequence-aware protocol:** Every frame contains a 16-bit sample sequence, preventing the Linux gateway from combining data from different periods.

