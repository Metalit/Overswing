#pragma once

#include "beatsaber-hook/shared/utils/typedefs-wrappers.hpp"

#include "GlobalNamespace/CutScoreBuffer.hpp"

struct SwingInfo {
    GlobalNamespace::CutScoreBuffer* scoreBuffer;
    bool leftSaber;
    int preSwing = 0;
    int postSwing = 0;
};

extern UnorderedEventCallback<SwingInfo> overswingCallbacks;
