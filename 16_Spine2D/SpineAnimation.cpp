#include "SpineAnimation.h"
#include <iostream>
#include <algorithm>


SpineAnimation::SpineAnimation() {
}

SpineAnimation::~SpineAnimation() {
    // 리소스 정리
    m_skeletons.clear();
    m_animationStates.clear();
}

bool SpineAnimation::LoadSkeleton(const std::string& name, const std::string& atlasPath, const std::string& jsonPath) {
    // 이미 로드된 스켈레톤인지 확인
    if (m_skeletons.find(name) != m_skeletons.end()) {
        std::cout << "스켈레톤 '" << name << "'이 이미 로드되어 있습니다." << std::endl;
        return false;
    }
    
    // 새로운 스켈레톤 정보 생성
    SkeletonInfo skeletonInfo(name, atlasPath, jsonPath);
    
    // 실제 Spine2D 라이브러리를 사용한 로드
    // 예시 코드:
    /*
    spine::Atlas* atlas = new spine::Atlas(atlasPath.c_str(), nullptr);
    if (!atlas) {
        std::cout << "아틀라스 로드 실패: " << atlasPath << std::endl;
        return false;
    }
    
    spine::SkeletonJson json(atlas);
    spine::SkeletonData* skeletonData = json.readSkeletonDataFile(jsonPath.c_str());
    if (!skeletonData) {
        std::cout << "스켈레톤 데이터 로드 실패: " << jsonPath << std::endl;
        delete atlas;
        return false;
    }
    
    // 애니메이션 정보 추출
    for (int i = 0; i < skeletonData->getAnimationsCount(); i++) {
        spine::Animation* animation = skeletonData->getAnimation(i);
        if (animation) {
            AnimationInfo animInfo;
            animInfo.name = animation->getName();
            animInfo.duration = animation->getDuration();
            animInfo.loop = true; // 기본값
            animInfo.speed = 1.0f; // 기본값
            skeletonInfo.animations.push_back(animInfo);
        }
    }
    */
    
    // Spine2D 기본 애니메이션 정보 추가
    skeletonInfo.animations.push_back(AnimationInfo("idle", 1.0f, true, 1.0f));
    skeletonInfo.animations.push_back(AnimationInfo("walk", 1.5f, true, 1.0f));
    skeletonInfo.animations.push_back(AnimationInfo("jump", 0.8f, false, 1.0f));
    
    // 스켈레톤 정보 저장
    m_skeletons[name] = skeletonInfo;
    
    // 애니메이션 상태 초기화
    InitializeAnimationState(name);
    
    std::cout << "스켈레톤 " << name << " 로드 완료" << std::endl;
    return true;
}

void SpineAnimation::AddAnimation(const std::string& skeletonName, const std::string& animationName, 
                                 float duration, bool loop, float speed) {
    auto it = m_skeletons.find(skeletonName);
    if (it == m_skeletons.end()) {
        std::cout << "스켈레톤 " << skeletonName << "을 찾을 수 없습니다." << std::endl;
        return;
    }
    
    // 애니메이션 정보 추가
    AnimationInfo animInfo(animationName, duration, loop, speed);
    it->second.animations.push_back(animInfo);
//    std::cout << "애니메이션 " << animationName << "추가됨" << std::endl;
}

void SpineAnimation::PlayAnimation(const std::string& skeletonName, const std::string& animationName) {
    auto stateIt = m_animationStates.find(skeletonName);
    if (stateIt == m_animationStates.end()) {
        std::cout << "스켈레톤 " << skeletonName << "의 애니메이션 상태를 찾을 수 없습니다." << std::endl;
        return;
    }
    
    // 애니메이션 존재 확인
    auto skeletonIt = m_skeletons.find(skeletonName);
    if (skeletonIt == m_skeletons.end()) {
        return;
    }
    
    bool animationFound = false;
    for (const auto& anim : skeletonIt->second.animations) {
        if (anim.name == animationName) {
            animationFound = true;
            break;
        }
    }
    
    if (!animationFound) {
        std::cout << "애니메이션 '" << animationName << "'을 찾을 수 없습니다." << std::endl;
        return;
    }
    
    // 애니메이션 재생 시작
    stateIt->second.currentAnimation = animationName;
    stateIt->second.currentTime = 0.0f;
    stateIt->second.isPlaying = true;
    stateIt->second.isPaused = false;
    
    std::cout << "애니메이션 '" << animationName << "' 재생 시작" << std::endl;
}

void SpineAnimation::StopAnimation(const std::string& skeletonName) {
    auto it = m_animationStates.find(skeletonName);
    if (it != m_animationStates.end()) {
        it->second.isPlaying = false;
        it->second.isPaused = false;
        it->second.currentTime = 0.0f;
        std::cout << "애니메이션 정지: " << skeletonName << std::endl;
    }
}

void SpineAnimation::PauseAnimation(const std::string& skeletonName) {
    auto it = m_animationStates.find(skeletonName);
    if (it != m_animationStates.end()) {
        it->second.isPaused = true;
        std::cout << "애니메이션 일시정지: " << skeletonName << std::endl;
    }
}

void SpineAnimation::ResumeAnimation(const std::string& skeletonName) {
    auto it = m_animationStates.find(skeletonName);
    if (it != m_animationStates.end()) {
        it->second.isPaused = false;
        std::cout << "애니메이션 재개: " << skeletonName << std::endl;
    }
}

void SpineAnimation::Update(float deltaTime) {
    for (auto& statePair : m_animationStates) {
        AnimationState& state = statePair.second;
        
        if (!state.isPlaying || state.isPaused) {
            continue;
        }
        
        // 애니메이션 시간 업데이트
        UpdateAnimationTime(statePair.first, deltaTime);
    }
}

bool SpineAnimation::IsPlaying(const std::string& skeletonName) const {
    auto it = m_animationStates.find(skeletonName);
    if (it != m_animationStates.end()) {
        return it->second.isPlaying && !it->second.isPaused;
    }
    return false;
}

float SpineAnimation::GetCurrentStateTime(const std::string& skeletonName) const {
    auto it = m_animationStates.find(skeletonName);
    if (it != m_animationStates.end()) {
        return it->second.currentTime;
    }
    return 0.0f;
}

std::string SpineAnimation::GetCurrentStateAnimation(const std::string& skeletonName) const {
    auto it = m_animationStates.find(skeletonName);
    if (it != m_animationStates.end()) {
        return it->second.currentAnimation;
    }
    return "";
}

const SpineAnimation::SkeletonInfo* SpineAnimation::GetSkeletonInfo(const std::string& name) const {
    auto it = m_skeletons.find(name);
    if (it != m_skeletons.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> SpineAnimation::GetSkeletonNames() const {
    std::vector<std::string> names;
    names.reserve(m_skeletons.size());
    
    for (const auto& pair : m_skeletons) {
        names.push_back(pair.first);
    }
    
    return names;
}

std::vector<std::string> SpineAnimation::GetAnimationNames(const std::string& skeletonName) const {
    std::vector<std::string> names;
    
    auto it = m_skeletons.find(skeletonName);
    if (it != m_skeletons.end()) {
        names.reserve(it->second.animations.size());
        for (const auto& anim : it->second.animations) {
            names.push_back(anim.name);
        }
    }
    
    return names;
}

void SpineAnimation::InitializeAnimationState(const std::string& skeletonName) {
    AnimationState state;
    state.currentAnimation = "";
    state.currentTime = 0.0f;
    state.isPlaying = false;
    state.isPaused = false;
    state.speed = 1.0f;
    
    m_animationStates[skeletonName] = state;
}

void SpineAnimation::UpdateAnimationTime(const std::string& skeletonName, float deltaTime) {
    auto stateIt = m_animationStates.find(skeletonName);
    auto skeletonIt = m_skeletons.find(skeletonName);
    
    if (stateIt == m_animationStates.end() || skeletonIt == m_skeletons.end()) {
        return;
    }
    
    AnimationState& state = stateIt->second;
    
    // 현재 애니메이션 정보 찾기
    const AnimationInfo* currentAnimInfo = nullptr;
    for (const auto& anim : skeletonIt->second.animations) {
        if (anim.name == state.currentAnimation) {
            currentAnimInfo = &anim;
            break;
        }
    }
    
    if (!currentAnimInfo) {
        return;
    }
    
    // 시간 업데이트
    state.currentTime += deltaTime * state.speed * currentAnimInfo->speed;
    
    // 애니메이션 종료 체크
    if (state.currentTime >= currentAnimInfo->duration) {
        if (currentAnimInfo->loop) {
            // 루프 애니메이션: 시간을 0으로 리셋
            state.currentTime = 0.0f;
        } else {
            // 비루프 애니메이션: 정지
            state.isPlaying = false;
            state.currentTime = currentAnimInfo->duration;
        }
    }
} 