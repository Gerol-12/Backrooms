#include "intro_sequence.h"

#include <algorithm>

float IntroSequence::duration(Phase phase) {
    switch (phase) {
        case Phase::BlackNoise: return 2.0f;
        case Phase::BlueSignal: return 2.5f;
        case Phase::StudioLogo: return 3.0f;
        case Phase::Complete:   return 0.0f;
    }
    return 0.0f;
}

void IntroSequence::reset() {
    phase_ = Phase::BlackNoise;
    phaseTime_ = 0.0f;
}

void IntroSequence::advance() {
    switch (phase_) {
        case Phase::BlackNoise:
            phase_ = Phase::BlueSignal;
            break;
        case Phase::BlueSignal:
            phase_ = Phase::StudioLogo;
            break;
        case Phase::StudioLogo:
        case Phase::Complete:
            phase_ = Phase::Complete;
            break;
    }
    phaseTime_ = 0.0f;
}

void IntroSequence::update(float deltaSeconds) {
    if (isFinished() || deltaSeconds <= 0.0f)
        return;

    // Consume the full delta so a slow frame cannot stretch the sequence.
    float remaining = std::min(deltaSeconds, 1.0f);
    while (remaining > 0.0f && !isFinished()) {
        const float timeLeft = duration(phase_) - phaseTime_;
        if (remaining < timeLeft) {
            phaseTime_ += remaining;
            remaining = 0.0f;
        } else {
            remaining -= timeLeft;
            advance();
        }
    }
}

void IntroSequence::skip() {
    phase_ = Phase::Complete;
    phaseTime_ = 0.0f;
}

float IntroSequence::phaseProgress() const {
    const float phaseDuration = duration(phase_);
    if (phaseDuration <= 0.0f)
        return 1.0f;
    return std::clamp(phaseTime_ / phaseDuration, 0.0f, 1.0f);
}
