#include "CameraPath.h"
#include "Camera.h"
#include <algorithm>

CameraPath::CameraPath()
    : currentKeyframe(0), currentTime(0.0f), state(PathState::STOPPED) {}

void CameraPath::Clear() {
    keyframes.clear();
    currentTime = 0.0f;
    currentKeyframe = 0;
    state = PathState::STOPPED;
}

void CameraPath::Play() {
    if (!keyframes.empty()) {
        state = PathState::PLAYING;
        currentKeyframe = 0;
        currentTime = 0.0f;
    }
}

void CameraPath::Resume() {
    if (state == PathState::PAUSED) {
        state = PathState::PLAYING;
    }
}

void CameraPath::Pause() {
    if (state == PathState::PLAYING) {
        state = PathState::PAUSED;
    }
}

void CameraPath::Reset() {
    currentTime = 0.0f;
    currentKeyframe = 0;
    state = PathState::STOPPED;
}

void CameraPath::AddKeyframe(const Vector3& position, float pitch, float yaw, float duration, bool isPausePoint) {
    keyframes.emplace_back(position, pitch, yaw, duration, isPausePoint);
}

void CameraPath::SetKeyframeCallback(size_t index, std::function<void()> callback) {
    if (index < keyframes.size()) {
        keyframes[index].onReachCallback = callback;
    }
}

float CameraPath::NormalizeAngle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

float CameraPath::ShortestAngleDistance(float start, float end) {
    float diff = NormalizeAngle(end - start);
    return diff;
}

void CameraPath::InterpolateBetweenKeyframes(const CameraKeyframe& start, const CameraKeyframe& end, float t, Camera* camera) {
    Vector3 newPosition = start.position + (end.position - start.position) * t;

    float pitchDiff = ShortestAngleDistance(start.pitch, end.pitch);
    float yawDiff = ShortestAngleDistance(start.yaw, end.yaw);

    float newPitch = start.pitch + pitchDiff * t;
    float newYaw = start.yaw + yawDiff * t;

    camera->SetPosition(newPosition);
    camera->SetPitch(newPitch);
    camera->SetYaw(newYaw);
}

void CameraPath::HandleKeyframeReached() {
    const CameraKeyframe& currentFrame = keyframes[currentKeyframe];

    if (currentFrame.onReachCallback) {
        currentFrame.onReachCallback();
    }

    if (currentFrame.isPausePoint) {
        state = PathState::PAUSED;
    }

    if (currentKeyframe >= keyframes.size() - 1) {
        state = PathState::COMPLETED;
    }
}

void CameraPath::Update(float dt, Camera* camera) {
    if (state != PathState::PLAYING || keyframes.size() < 2 || currentKeyframe >= keyframes.size() - 1) {
        return;
    }

    currentTime += dt;
    const CameraKeyframe& currentFrame = keyframes[currentKeyframe];
    const CameraKeyframe& nextFrame = keyframes[currentKeyframe + 1];

    float t = currentTime / currentFrame.duration;

    if (t >= 1.0f) {
        currentTime = 0.0f;
        currentKeyframe++;
        HandleKeyframeReached();
    }
    else {
        float smoothT = t * t * (3 - 2 * t);
        InterpolateBetweenKeyframes(currentFrame, nextFrame, smoothT, camera);
    }
}