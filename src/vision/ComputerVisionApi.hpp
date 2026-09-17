#pragma once
/**
 * CCOS - Computer Vision API
 * Optional computer-vision subsystem for native C++ builds.
 */

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <functional>
#include <opencv2/opencv.hpp>
#include <mediapipe/framework/calculator_framework.h>
#include <mediapipe/framework/formats/image_frame.h>

namespace ccos::vision {

struct DetectedObject {
    std::string label;
    float confidence;
    cv::Rect boundingBox;
    int trackId = -1;
};

struct FaceLandmark {
    cv::Point2f position;
    std::string landmarkName;
    float confidence;
};

struct PoseKeypoint {
    std::string name;
    cv::Point2f position;
    float visibility;
};

struct MotionData {
    cv::Point2f motionVector;
    float magnitude;
    float angle;
    bool isSignificant;
};

struct SceneAnalysis {
    std::string dominantColor;
    std::vector<std::string> detectedObjects;
    std::string sceneType;
    float brightness;
    float contrast;
    float sharpness;
    bool hasFaces;
    int faceCount;
    bool hasText;
    std::string mood;
};

class ComputerVisionEngine {
public:
    static ComputerVisionEngine& getInstance();

    bool initializeOpenCV();
    bool initializeMediaPipe(const std::string& modelPath);
    bool initializeYOLO(const std::string& weightsPath, const std::string& configPath);
    bool initializeDeepSort(const std::string& reidModelPath);

    std::vector<DetectedObject> detectObjects(const cv::Mat& frame,
                                               const std::vector<std::string>& classes = {});

    std::map<int, std::vector<DetectedObject>> trackObjects(const std::vector<cv::Mat>& frames);

    struct FaceDetectionResult {
        std::vector<cv::Rect> faces;
        std::vector<std::vector<FaceLandmark>> landmarks;
        std::vector<float> emotions;
        std::vector<int> ages;
        std::vector<std::string> genders;
    };
    FaceDetectionResult detectFaces(const cv::Mat& frame);

    std::vector<std::vector<PoseKeypoint>> estimatePose(const cv::Mat& frame);
    cv::Mat segmentScene(const cv::Mat& frame, const std::vector<std::string>& targetClasses = {});
    std::vector<cv::Mat> segmentInstances(const cv::Mat& frame);
    std::vector<MotionData> analyzeMotion(const std::vector<cv::Mat>& frames);
    cv::Mat stabilizeFrame(const cv::Mat& frame, const cv::Mat& prevFrame);
    SceneAnalysis analyzeScene(const cv::Mat& frame);
    std::vector<std::pair<cv::Rect, std::string>> detectText(const cv::Mat& frame);
    cv::Mat estimateDepth(const cv::Mat& frame);
    cv::Mat removeBackground(const cv::Mat& frame, bool useAI = true);
    std::vector<cv::Point2f> trackFeatures(const cv::Mat& prevFrame,
                                            const cv::Mat& currFrame,
                                            const std::vector<cv::Point2f>& prevPoints);
    std::vector<std::pair<cv::KeyPoint, cv::KeyPoint>> matchFeatures(
        const cv::Mat& img1, const cv::Mat& img2);
    cv::Mat detectEdges(const cv::Mat& frame, const std::string& method = "canny");
    cv::Mat superResolve(const cv::Mat& frame, int scale = 2);
    cv::Mat deblur(const cv::Mat& frame, const std::string& method = "wiener");
    cv::Mat mergeHDR(const std::vector<cv::Mat>& exposures);
    cv::Mat stitchPanorama(const std::vector<cv::Mat>& images);
    cv::Mat chromaKey(const cv::Mat& frame, const cv::Scalar& color, float tolerance = 0.3f);
    cv::Mat rotoscope(const cv::Mat& frame, const std::vector<cv::Point>& points);

    struct QualityMetrics {
        float sharpness;
        float noiseLevel;
        float compressionArtifacts;
        float colorAccuracy;
        float overallScore;
    };
    QualityMetrics assessQuality(const cv::Mat& frame);

    std::vector<int> detectSceneChanges(const std::vector<cv::Mat>& frames, float threshold = 0.5f);
    cv::Mat applySmartColorGrade(const cv::Mat& frame, const std::string& preset = "natural");
    cv::Mat autoFrame(const cv::Mat& frame, float targetAspect = 9.0f / 16.0f);
    std::vector<cv::Mat> interpolateFrames(const cv::Mat& frame1, const cv::Mat& frame2, int numFrames = 3);

    bool isInitialized() const { return initialized_; }
    std::string getBackendInfo() const;

private:
    ComputerVisionEngine() = default;
    ~ComputerVisionEngine() = default;
    ComputerVisionEngine(const ComputerVisionEngine&) = delete;
    ComputerVisionEngine& operator=(const ComputerVisionEngine&) = delete;

    bool initialized_ = false;
    bool openCVInitialized_ = false;
    bool mediaPipeInitialized_ = false;
    bool yoloInitialized_ = false;

    std::unique_ptr<mediapipe::CalculatorGraph> mediaPipeGraph_;
    cv::Ptr<cv::Tracker> createTracker(const std::string& type = "CSRT");
    cv::Ptr<cv::ml::SVM> motionClassifier_;
};

class VisionTimelineTools {
public:
    static std::vector<int> generateMotionKeyframes(const std::string& videoPath, float sensitivity = 0.5f);
    static std::vector<std::pair<int, int>> detectSceneCuts(const std::string& videoPath);
    static std::vector<cv::Mat> extractMLReadyFrames(const std::string& videoPath, int maxFrames = 100);
    static cv::Mat createTrackingMask(const std::string& videoPath, const cv::Rect& initialBox);

    struct EditingSuggestions {
        std::vector<std::pair<int, int>> highlightMoments;
        std::vector<std::string> detectedThemes;
        std::string suggestedMusicMood;
        float recommendedPace;
    };
    static EditingSuggestions suggestEdits(const std::string& videoPath);
};

} // namespace ccos::vision
