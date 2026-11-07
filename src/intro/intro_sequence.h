#pragma once

// Controls the timed startup sequence. Rendering is intentionally kept
// separate so the intro can reuse the existing fullscreen post-process path.
class IntroSequence {
public:
    enum class Phase {
        BlackNoise,
        BlueSignal,
        StudioLogo,
        Complete
    };

    void reset();
    void update(float deltaSeconds);
    void skip();

    Phase phase() const { return phase_; }
    float phaseTime() const { return phaseTime_; }
    float phaseProgress() const;
    bool isFinished() const { return phase_ == Phase::Complete; }

private:
    Phase phase_ = Phase::BlackNoise;
    float phaseTime_ = 0.0f;

    static float duration(Phase phase);
    void advance();
};
