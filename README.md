# openCV-Starter-cpp

![OpenCV](https://img.shields.io/badge/OpenCV-4.x-blue)



## Overview

<p align="center">
  <img src="https://github.com/zhinCode/openCV-Starter-cpp/assets/172176975/c8712b46-5dfc-48b5-944c-1f4b0d5be915">
</p>

real-time face and body detection using OpenCV and DNN models.
It supports webcam and video file input, detects faces and bodies, and marks them with colored rectangles based on confidence levels.

- For testing purposes, you can select a video file at the start of the program. If no file is selected, a webcam device is required.
- (테스트를 위해 프로그램 시작시 동영상 파일를 선택할 수 있음. 선택하지 않을시 웹캠디바이스 필요)

## Features

- Real-time face and body detection
- Confidence level-based color coding:
  - Green: High confidence (80% and above)
  - Yellow: Medium confidence (50% to 79%)
  - Red: Low confidence (below 50%)
- Labels for detected faces and bodies

## Prerequisites

- OpenCV 4.x
- CMake
- C++17 compatible compiler
- DNN model files:
  - `deploy.prototxt`
  - `res10_300x300_ssd_iter_140000_fp16.caffemodel`
  - `MobileNetSSD_deploy.prototxt`
  - `MobileNetSSD_deploy.caffemodel`

## Installation


1. Clone the repository:
   ```sh
   git clone https://github.com/zhinCode/openCV-Starter-cpp.git
   cd openCV-Starter-cpp
   ```

2. Download the DNN model files and place them in the resources directory.
    ```sh
    mkdir build
    cd build
    cmake ..
    make
    ```

## Usage

## License
This project is licensed under the MIT License. See the LICENSE file for details.

## Acknowledgements
- OpenCV library
- DNN model files from OpenCV repository