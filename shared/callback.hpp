#pragma once

#include "beatsaber-hook/shared/utils/typedefs-wrappers.hpp"

#include "GlobalNamespace/CutScoreBuffer.hpp"

struct SwingInfo {
    GlobalNamespace::CutScoreBuffer* scoreBuffer;
    bool rightSaber;
    float preSwing = 0;
    float postSwing = 0;
};

extern UnorderedEventCallback<SwingInfo> overswingCallbacks;
