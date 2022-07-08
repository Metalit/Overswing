#pragma once

#include "beatsaber-hook/shared/utils/typedefs-wrappers.hpp"

struct SwingInfo {
    bool leftSaber;
    int preSwing = 0;
    int postSwing = 0;
};

extern UnorderedEventCallback<SwingInfo> overswingCallbacks;
