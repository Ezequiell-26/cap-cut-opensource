#pragma once
/**
 * CCOS - Computer Vision API
 * Integración con OpenCV, MediaPipe, YOLO y otras librerías MIT de visión por computadora
 * Licencias: MIT (MediaPipe), Apache 2.0 (OpenCV), GPL/Commercial (YOLO)
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
    std::string sceneType; // "indoor", "outdoor", "studio", etc.
    float brightness;
    float contrast;
    float sharpness;
    bool hasFaces;
    int faceCount;
    bool hasText;
    std::string mood; // "happy", "sad", "neutral", etc.
};

class ComputerVisionEngine {
public:
    static ComputerVisionEngine& getInstance();
    
    // Inicialización de motores
    bool initializeOpenCV();
    bool initializeMediaPipe(const std::string& modelPath);
    bool initializeYOLO(const std::string& weightsPath, const std::string& configPath);
    bool initializeDeepSort(const std::string& reidModelPath);
    
    // Detección de objetos en tiempo real
    std::vector<DetectedObject> detectObjects(const cv::Mat& frame, 
                                               const std::vector<std::string>& classes = {});
    
    // Seguimiento de objetos multi-objeto
    std::map<int, std::vector<DetectedObject>> trackObjects(const std::vector<cv::Mat>& frames);
    
    // Detección facial avanzada
    struct FaceDetectionResult {
        std::vector<cv::Rect> faces;
        std::vector<std::vector<FaceLandmark>> landmarks;
        std::vector<float> emotions; // [angry, disgust, fear, happy, neutral, sad, surprise]
        std::vector<int> ages;
        std::vector<std::string> genders;
    };
    FaceDetectionResult detectFaces(const cv::Mat& frame);
    
    // Análisis de pose humana
    std::vector<std::vector<PoseKeypoint>> estimatePose(const cv::Mat& frame);
    
    // Segmentación semántica
    cv::Mat segmentScene(const cv::Mat& frame, 
                         const std::vector<std::string>& targetClasses = {});
    
    // Segmentación de instancias
    std::vector<cv::Mat> segmentInstances(const cv::Mat& frame);
    
    // Detección de movimiento
    std::vector<MotionData> analyzeMotion(const std::vector<cv::Mat>& frames);
    
    // Estabilización de video
    cv::Mat stabilizeFrame(const cv::Mat& frame, const cv::Mat& prevFrame);
    
    // Análisis de escena completo
    SceneAnalysis analyzeScene(const cv::Mat& frame);
    
    // Detección de texto (OCR)
    std::vector<std::pair<cv::Rect, std::string>> detectText(const cv::Mat& frame);
    
    // Estimación de profundidad
    cv::Mat estimateDepth(const cv::Mat& frame);
    
    // Remoción de fondo
    cv::Mat removeBackground(const cv::Mat& frame, bool useAI = true);
    
    // Tracking de puntos clave
    std::vector<cv::Point2f> trackFeatures(const cv::Mat& prevFrame, 
                                            const cv::Mat& currFrame,
                                            const std::vector<cv::Point2f>& prevPoints);
    
    // Matcheo de características
    std::vector<std::pair<cv::KeyPoint, cv::KeyPoint>> matchFeatures(
        const cv::Mat& img1, const cv::Mat& img2);
    
    // Detección de bordes mejorada
    cv::Mat detectEdges(const cv::Mat& frame, const std::string& method = "canny");
    
    // Super-resolución
    cv::Mat superResolve(const cv::Mat& frame, int scale = 2);
    
    // Deblurring
    cv::Mat deblur(const cv::Mat& frame, const std::string& method = "wiener");
    
    // HDR merge
    cv::Mat mergeHDR(const std::vector<cv::Mat>& exposures);
    
    // Panorama stitching
    cv::Mat stitchPanorama(const std::vector<cv::Mat>& images);
    
    // Green screen / Chroma key avanzado
    cv::Mat chromaKey(const cv::Mat& frame, const cv::Scalar& color, 
                      float tolerance = 0.3f);
    
    // Rotoscopia asistida por IA
    cv::Mat rotoscope(const cv::Mat& frame, const std::vector<cv::Point>& points);
    
    // Análisis de calidad de video
    struct QualityMetrics {
        float sharpness;
        float noiseLevel;
        float compressionArtifacts;
        float colorAccuracy;
        float overallScore;
    };
    QualityMetrics assessQuality(const cv::Mat& frame);
    
    // Detección de escenas (scene change detection)
    std::vector<int> detectSceneChanges(const std::vector<cv::Mat>& frames, 
                                         float threshold = 0.5f);
    
    // Color grading asistido por IA
    cv::Mat applySmartColorGrade(const cv::Mat& frame, 
                                  const std::string& preset = "natural");
    
    // Auto-framing para diferentes aspect ratios
    cv::Mat autoFrame(const cv::Mat& frame, float targetAspect = 9.0f/16.0f);
    
    // Slow motion interpolado (optical flow)
    std::vector<cv::Mat> interpolateFrames(const cv::Mat& frame1, 
                                            const cv::Mat& frame2, 
                                            int numFrames = 3);
    
    // Obtener estado del motor
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
    
    std::unique_ptr<m mediapipe::CalculatorGraph> mediaPipeGraph_;
    cv::Ptr<cv::Tracker> createTracker(const std::string& type = "CSRT");
    cv::Ptr<cv::ml::SVM> motionClassifier_;
};

// Funciones utilitarias para integración con el timeline
class VisionTimelineTools {
public:
    // Generar keyframes basados en detección de movimiento
    static std::vector<int> generateMotionKeyframes(const std::string& videoPath, 
                                                     float sensitivity = 0.5f);
    
    // Auto-cortar escenas basado en análisis de contenido
    static std::vector<std::pair<int, int>> detectSceneCuts(const std::string& videoPath);
    
    // Generar proxies optimizados para ML
    static std::vector<cv::Mat> extractMLReadyFrames(const std::string& videoPath, 
                                                      int maxFrames = 100);
    
    // Crear máscara de seguimiento para efectos
    static cv::Mat createTrackingMask(const std::string& videoPath, 
                                       const cv::Rect& initialBox);
    
    // Analizar footage para sugerencias de edición
    struct EditingSuggestions {
        std::vector<std::pair<int, int>> highlightMoments;
        std::vector<std::string> detectedThemes;
        std::string suggestedMusicMood;
        float recommendedPace; // 0.0 (lento) a 1.0 (rápido)
    };
    static EditingSuggestions suggestEdits(const std::string& videoPath);
};

} // namespace ccos::vision
