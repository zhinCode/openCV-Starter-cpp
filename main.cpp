// main.cpp
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <vector>

// 유틸리티 함수: std::string -> std::wstring 변환
std::wstring s2ws(const std::string& s) {
    int len;
    int slength = static_cast<int>(s.length()) + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    std::wstring r(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &r[0], len);
    return r;
}

// 유틸리티 함수: std::wstring -> std::string 변환
std::string ws2s(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring openFileDialog() {
    OPENFILENAME ofn;       // 구조체 초기화
    wchar_t szFile[260] = { 0 }; // 파일 경로를 저장할 버퍼
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(*szFile);
    ofn.lpstrFilter = L"Video Files\0*.MP4;*.AVI;*.MKV;*.MOV;*.WMV\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        return std::wstring(szFile);
    }
    else {
        return L"";
    }
}

int main() {
    std::wstring winput = openFileDialog();
    std::string input = ws2s(winput);

    // 절대 경로로 DNN 모델 파일 경로 설정
    wchar_t absolutePathFaceModel[MAX_PATH];
    wchar_t absolutePathFaceWeights[MAX_PATH];
    wchar_t absolutePathBodyModel[MAX_PATH];
    wchar_t absolutePathBodyWeights[MAX_PATH];
    GetFullPathNameW(L"resources\\deploy.prototxt", MAX_PATH, absolutePathFaceModel, NULL);
    GetFullPathNameW(L"resources\\res10_300x300_ssd_iter_140000_fp16.caffemodel", MAX_PATH, absolutePathFaceWeights, NULL);
    GetFullPathNameW(L"resources\\MobileNetSSD_deploy.prototxt", MAX_PATH, absolutePathBodyModel, NULL);
    GetFullPathNameW(L"resources\\MobileNetSSD_deploy.caffemodel", MAX_PATH, absolutePathBodyWeights, NULL);
    std::string faceModelConfiguration = ws2s(absolutePathFaceModel);
    std::string faceModelWeights = ws2s(absolutePathFaceWeights);
    std::string bodyModelConfiguration = ws2s(absolutePathBodyModel);
    std::string bodyModelWeights = ws2s(absolutePathBodyWeights);

    // 얼굴 DNN 모델 로드
    cv::dnn::Net faceNet = cv::dnn::readNetFromCaffe(faceModelConfiguration, faceModelWeights);
    if (faceNet.empty()) {
        std::cerr << "Error loading face DNN model from " << faceModelConfiguration << " and " << faceModelWeights << std::endl;
        return -1;
    }

    // 몸 DNN 모델 로드
    cv::dnn::Net bodyNet = cv::dnn::readNetFromCaffe(bodyModelConfiguration, bodyModelWeights);
    if (bodyNet.empty()) {
        std::cerr << "Error loading body DNN model from " << bodyModelConfiguration << " and " << bodyModelWeights << std::endl;
        return -1;
    }

    // 비디오 캡처 초기화
    cv::VideoCapture cap;
    if (input.empty()) {
        cap.open(0); // 웹캠 사용
        if (!cap.isOpened()) {
            std::cerr << "Error opening video capture" << std::endl;
            return -1;
        }
    }
    else {
        cap.open(input); // 동영상 파일 사용
        if (!cap.isOpened()) {
            std::cerr << "Error opening video file" << std::endl;
            return -1;
        }
    }

    cv::Mat frame, faceBlob, bodyBlob;
    std::vector<cv::Rect> faceTrackWindows, bodyTrackWindows;
    std::vector<float> faceConfidences, bodyConfidences;
    bool tracking = false;

    // 윈도우 설정
    cv::namedWindow("Webcam Feed", cv::WINDOW_NORMAL);
    int window_width = 640, window_height = 480;
    cv::resizeWindow("Webcam Feed", window_width, window_height);

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        // 얼굴 인식
        faceBlob = cv::dnn::blobFromImage(frame, 1.0, cv::Size(300, 300), cv::Scalar(104.0, 177.0, 123.0), false, false);
        faceNet.setInput(faceBlob);
        cv::Mat faceDetections = faceNet.forward();

        // 몸 인식
        bodyBlob = cv::dnn::blobFromImage(frame, 0.007843, cv::Size(300, 300), cv::Scalar(127.5, 127.5, 127.5), false, false);
        bodyNet.setInput(bodyBlob);
        cv::Mat bodyDetections = bodyNet.forward();

        faceTrackWindows.clear();
        faceConfidences.clear();
        bodyTrackWindows.clear();
        bodyConfidences.clear();

        // 얼굴 인식 결과 처리
        float* faceData = (float*)faceDetections.data;
        for (size_t i = 0; i < faceDetections.total(); i += 7) {
            float confidence = faceData[i + 2];
            if (confidence > 0.3) { // 임계값을 낮추어 인식 성능 향상
                int x1 = static_cast<int>(faceData[i + 3] * frame.cols);
                int y1 = static_cast<int>(faceData[i + 4] * frame.rows);
                int x2 = static_cast<int>(faceData[i + 5] * frame.cols);
                int y2 = static_cast<int>(faceData[i + 6] * frame.rows);
                faceTrackWindows.push_back(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)));
                faceConfidences.push_back(confidence);
            }
        }

        // 몸 인식 결과 처리
        float* bodyData = (float*)bodyDetections.data;
        for (size_t i = 0; i < bodyDetections.total(); i += 7) {
            float confidence = bodyData[i + 2];
            int classId = static_cast<int>(bodyData[i + 1]);
            if (classId == 15 && confidence > 0.3) { // Class 15 is for 'person', 임계값을 낮추어 인식 성능 향상
                int x1 = static_cast<int>(bodyData[i + 3] * frame.cols);
                int y1 = static_cast<int>(bodyData[i + 4] * frame.rows);
                int x2 = static_cast<int>(bodyData[i + 5] * frame.cols);
                int y2 = static_cast<int>(bodyData[i + 6] * frame.rows);
                bodyTrackWindows.push_back(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)));
                bodyConfidences.push_back(confidence);
            }
        }

        if (!faceTrackWindows.empty() || !bodyTrackWindows.empty()) {
            tracking = true;
        }

        if (tracking) {
            for (size_t i = 0; i < faceTrackWindows.size(); ++i) {
                cv::Scalar color;
                if (faceConfidences[i] > 0.8) {
                    color = cv::Scalar(0, 255, 0); // Green
                }
                else if (faceConfidences[i] > 0.5) {
                    color = cv::Scalar(0, 255, 255); // Yellow
                }
                else {
                    color = cv::Scalar(0, 0, 255); // Red
                }
                cv::rectangle(frame, faceTrackWindows[i], color, 2);
                cv::putText(frame, "Face", cv::Point(faceTrackWindows[i].x, faceTrackWindows[i].y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
            }
            for (size_t i = 0; i < bodyTrackWindows.size(); ++i) {
                cv::rectangle(frame, bodyTrackWindows[i], cv::Scalar(255, 0, 0), 2); // Blue
                cv::putText(frame, "Person", cv::Point(bodyTrackWindows[i].x, bodyTrackWindows[i].y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 2);
            }
        }

        // 결과 출력
        cv::imshow("Webcam Feed", frame);
        if (cv::waitKey(30) >= 0) break;
    }

    return 0;
}
