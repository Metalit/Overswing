#include "main.hpp"
#include "callback.hpp"

#include "GlobalNamespace/SaberSwingRatingCounter.hpp"
#include "GlobalNamespace/SaberSwingRating.hpp"
#include "GlobalNamespace/CutScoreBuffer.hpp"
#include "GlobalNamespace/SaberMovementData.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"

using namespace GlobalNamespace;

static ModInfo modInfo;

Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

UnorderedEventCallback<SwingInfo> overswingCallbacks;

std::unordered_map<SaberSwingRatingCounter*, SwingInfo> swingMap;

// populates the SaberSwingRatingCounter -> CutScoreBuffer map specifically before SaberSwingRatingCounter.Init
MAKE_HOOK_MATCH(CutScoreBuffer_Init, &CutScoreBuffer::Init, bool, CutScoreBuffer* self, ByRef<NoteCutInfo> noteCutInfo) {
    
    swingMap.emplace(self->saberSwingRatingCounter, SwingInfo());
    swingMap[self->saberSwingRatingCounter].scoreBuffer = self;
    swingMap[self->saberSwingRatingCounter].leftSaber = self->noteCutInfo.saberType == SaberType::SaberA;
    
    return CutScoreBuffer_Init(self, noteCutInfo);
}

// calculate pre swing overswing and also ensure the pre swing is clamped
MAKE_HOOK_MATCH(SaberSwingRatingCounter_Init, &SaberSwingRatingCounter::Init, void, SaberSwingRatingCounter* self, ISaberMovementData* saberMovementData, UnityEngine::Vector3 notePosition, UnityEngine::Quaternion noteRotation, bool rateBeforeCut, bool rateAfterCut) {
    
    SaberSwingRatingCounter_Init(self, saberMovementData, notePosition, noteRotation, rateBeforeCut, rateAfterCut);

    int& total = swingMap[self].postSwing;

    if(self->rateBeforeCut)
        total += self->beforeCutRating;

    if(self->beforeCutRating > 1)
        self->beforeCutRating = 1;
}

// calculate post swing overswing and also ensure the pre swing is clamped
MAKE_HOOK_MATCH(SaberSwingRatingCounter_ProcessNewData, &SaberSwingRatingCounter::ProcessNewData, void, SaberSwingRatingCounter* self, BladeMovementDataElement newData, BladeMovementDataElement prevData, bool prevDataAreValid) {
    
    bool alreadyCut = self->notePlaneWasCut;

    // avoid clamping in this method only when called from Init
    // since Init also computes beforeCutRating, when this method is reached it will detect that it doesn't need to clamp
    // it will then be clamped at the end of Init, after this call, and then won't trigger dontClamp again
    // it won't know the difference if Init doesn't compute a value over 1, but then it doesn't matter if we clamp anyway
    // unless Init calculates a value below one and then here we calculate a value above one, in which case it will be clamped
    // but hopefully that isn't too significant, as it would be kind of complicated to fix
    bool dontClamp = self->beforeCutRating > 1;
    
    SaberSwingRatingCounter_ProcessNewData(self, newData, prevData, prevDataAreValid);

    int& total = swingMap[self].postSwing;

    if(!alreadyCut) {
        float postAngle = UnityEngine::Vector3::Angle(self->cutTopPos - self->cutBottomPos, self->afterCutTopPos - self->afterCutBottomPos);
        total += SaberSwingRating::AfterCutStepRating(postAngle, 0);
    } else {
        float num = UnityEngine::Vector3::Angle(newData.segmentNormal, self->cutPlaneNormal);
        total += SaberSwingRating::AfterCutStepRating(newData.segmentAngle, num);
    }
    
    // correct for change in ComputeSwingRating
    if(self->beforeCutRating > 1 && !dontClamp)
        self->beforeCutRating = 1;
}

// override to remove clamping at the end of this method and sort it out elsewhere so we can add up overswings
MAKE_HOOK_MATCH(SaberMovementData_CalculateSwingRating, static_cast<float (SaberMovementData::*)(bool, float)>(&SaberMovementData::ComputeSwingRating), float, SaberMovementData* self, bool overrideSegmenAngle, float overrideValue) {
    if (self->validCount < 2)
        return 0;

    int num = self->data.Length();
    int num2 = self->nextAddIndex - 1;
    if (num2 < 0)
        num2 += num;

    float time = self->data[num2].time;
    float num3 = time;
    float num4 = 0;
    UnityEngine::Vector3 segmentNormal = self->data[num2].segmentNormal;
    float angleDiff = (overrideSegmenAngle ? overrideValue : self->data[num2].segmentAngle);
    int num5 = 2;

    num4 += SaberSwingRating::BeforeCutStepRating(angleDiff, 0);

    while (time - num3 < 0.4 && num5 < self->validCount) {
        num2--;
        if (num2 < 0)
            num2 += num;
        UnityEngine::Vector3 segmentNormal2 = self->data[num2].segmentNormal;
        angleDiff = self->data[num2].segmentAngle;
        float num6 = UnityEngine::Vector3::Angle(segmentNormal2, segmentNormal);
        if (num6 > 90)
            break;
        num4 += SaberSwingRating::BeforeCutStepRating(angleDiff, num6);
        num3 = self->data[num2].time;
        num5++;
    }
    return num4;
}

// remove counters from map and send callbacks when they finish
MAKE_HOOK_MATCH(SaberSwingRatingCounter_Finish, &SaberSwingRatingCounter::Finish, void, SaberSwingRatingCounter* self) {

    SaberSwingRatingCounter_Finish(self);
    
    auto iter = swingMap.find(self);
    if(iter != swingMap.end()) {
        overswingCallbacks.invoke(iter->second);
        swingMap.erase(iter);
    }
}

extern "C" void setup(ModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo = info;
	
    getLogger().info("Completed setup!");
}

extern "C" void load() {
    il2cpp_functions::Init();

    getLogger().info("Installing hooks...");
    LoggerContextObject logger = getLogger().WithContext("load");
    INSTALL_HOOK(logger, CutScoreBuffer_Init);
    INSTALL_HOOK(logger, SaberSwingRatingCounter_Init);
    INSTALL_HOOK(logger, SaberSwingRatingCounter_ProcessNewData);
    INSTALL_HOOK_ORIG(logger, SaberMovementData_CalculateSwingRating);
    INSTALL_HOOK(logger, SaberSwingRatingCounter_Finish);
    getLogger().info("Installed all hooks!");
}
