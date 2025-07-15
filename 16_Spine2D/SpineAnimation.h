#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

// Spine2D 애니메이션 관리 클래스
class SpineAnimation {
public:
    // 애니메이션 정보 구조체
    struct AnimationInfo {
        std::string name;
        float duration;
        bool loop;
        float speed;
        
        AnimationInfo() : duration(0.0f), loop(true), speed(1.0f) {}
        AnimationInfo(const std::string& n, float d, bool l = true, float s = 1.0f)
            : name(n), duration(d), loop(l), speed(s) {}
    };

    // 스켈레톤 정보 구조체
    struct SkeletonInfo {
        std::string name;
        std::string atlasPath;
        std::string jsonPath;
        std::vector<AnimationInfo> animations;
        
        SkeletonInfo() {}
        SkeletonInfo(const std::string& n, const std::string& atlas, const std::string& json)
            : name(n), atlasPath(atlas), jsonPath(json) {}
    };

public:
    SpineAnimation();
    ~SpineAnimation();

    // 스켈레톤 로드
    bool LoadSkeleton(const std::string& name, const std::string& atlasPath, const std::string& jsonPath);
    
    // 애니메이션 추가
    void AddAnimation(const std::string& skeletonName, const std::string& animationName, 
                     float duration, bool loop = true, float speed = 1.0f);
    
    // 애니메이션 재생
    void PlayAnimation(const std::string& skeletonName, const std::string& animationName);
    void StopAnimation(const std::string& skeletonName);
    void PauseAnimation(const std::string& skeletonName);
    void ResumeAnimation(const std::string& skeletonName);
    
    // 애니메이션 업데이트
    void Update(float deltaTime);
    
    // 애니메이션 상태 조회
    bool IsPlaying(const std::string& skeletonName) const;
    float GetCurrentStateTime(const std::string& skeletonName) const;
    std::string GetCurrentStateAnimation(const std::string& skeletonName) const;
    
    // 스켈레톤 정보 조회
    const SkeletonInfo* GetSkeletonInfo(const std::string& name) const;
    std::vector<std::string> GetSkeletonNames() const;
    std::vector<std::string> GetAnimationNames(const std::string& skeletonName) const;

private:
    // 애니메이션 상태 구조체
    struct AnimationState {
        std::string currentAnimation;
        float currentTime;
        bool isPlaying;
        bool isPaused;
        float speed;
        
        AnimationState() : currentTime(0.0f), isPlaying(false), isPaused(false), speed(1.0f) {}
    };

    // 스켈레톤 데이터
    std::unordered_map<std::string, SkeletonInfo> m_skeletons;
    
    // 애니메이션 상태
    std::unordered_map<std::string, AnimationState> m_animationStates;

private:
    // 애니메이션 상태 초기화
    void InitializeAnimationState(const std::string& skeletonName);
    
    // 애니메이션 시간 업데이트
    void UpdateAnimationTime(const std::string& skeletonName, float deltaTime);
}; 