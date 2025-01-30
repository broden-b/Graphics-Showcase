#pragma once
#include "Vector3.h"
#include <vector>
#include <functional>

class Camera;

class CameraPath {
public:
    struct CameraKeyframe {
        Vector3 position;
        float pitch;
        float yaw;
        float duration; 
        bool isPausePoint;  
        std::function<void()> onReachCallback;  

        CameraKeyframe(const Vector3& pos, float p, float y, float d, bool pause = false)
            : position(pos), pitch(p), yaw(y), duration(d), isPausePoint(pause), onReachCallback(nullptr) {}
    };

    enum class PathState {
        STOPPED,
        PLAYING,
        PAUSED,
        COMPLETED
    };

    CameraPath();
    ~CameraPath() = default;

    void AddKeyframe(const Vector3& position, float pitch, float yaw, float duration, bool isPausePoint = false);
    void SetKeyframeCallback(size_t index, std::function<void()> callback);
    void Clear();
    void Play();
    void Resume();
    void Pause();
    void Reset();

    bool IsPlaying() const { return state == PathState::PLAYING; }
    bool IsPaused() const { return state == PathState::PAUSED; }
    bool IsComplete() const { return state == PathState::COMPLETED; }
    size_t GetCurrentKeyframe() const { return currentKeyframe; }

    void Update(float dt, Camera* camera);

private:
    void InterpolateBetweenKeyframes(const CameraKeyframe& start, const CameraKeyframe& end, float t, Camera* camera);
    float NormalizeAngle(float angle);
    float ShortestAngleDistance(float start, float end);
    void HandleKeyframeReached();

    std::vector<CameraKeyframe> keyframes;
    size_t currentKeyframe;
    float currentTime;
    PathState state;
};
