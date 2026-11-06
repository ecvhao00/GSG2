#include "LBFramework.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/LightComponent.h"
#include "Engine/Light.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

ALBDirector::ALBDirector()
{
    PrimaryActorTick.bCanEverTick=true;
    Settings=CreateDefaultSubobject<ULBSettings>(TEXT("DefaultTuning"));
    IntroSpawn=FVector(1700,1740,96);
    ExitRespawn=FVector(2340,1100,96);
    EnemyStart=FVector(-300,-420,0);
    EndingCamera=FVector(2600,1300,180);
    EndingRotation=FRotator(-8,-135,0);
}
void ALBDirector::BeginPlay()
{
    Super::BeginPlay();
    if(!Settings)Settings=NewObject<ULBSettings>(this);
    for(TActorIterator<ALBDevice> It(GetWorld());It;++It)Devices.Add(*It);
    for(TActorIterator<ALBDoor> It(GetWorld());It;++It)Doors.Add(*It);
    for(TActorIterator<ALBEnemy> It(GetWorld());It;++It){Enemy=*It;Enemy->Director=this;break;}
    Player=Cast<ALBPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if(Player)Player->Director=this;
    Preferences=Cast<ULBPreferences>(UGameplayStatics::LoadGameFromSlot(TEXT("LastBroadcast_Preferences"),0));
    if(!Preferences)Preferences=NewObject<ULBPreferences>(this);
    bTestMode=FParse::Param(FCommandLine::Get(),TEXT("LBFunctionalTest"));
    if(bTestMode)SaveSlot=TEXT("LastBroadcast_Automation_Isolated");
    else if(FParse::Param(FCommandLine::Get(),TEXT("LBVisualTest")))SaveSlot=TEXT("LastBroadcast_Visual_Isolated");
    ApplyPreferences(); RefreshWorld();
    if(Enemy)Enemy->BuildNavigation();
    if(bTestMode){ Menu.Empty(); GetWorldTimerManager().SetTimerForNextTick(this,&ALBDirector::RunFunctionalTests); }
    else SetMenu(TEXT("Title"));
}
bool ALBDirector::PrototypePuzzlesEnabled() const
{
#if UE_BUILD_SHIPPING
    return false;
#else
    return Settings->bEnablePrototypePuzzles;
#endif
}
bool ALBDirector::AllObjectivesComplete() const
{
    if(!Settings->bPuzzleContentReady&&!PrototypePuzzlesEnabled())return false;
    if(Settings->RequiredPuzzleIds.IsEmpty())return Settings->bPuzzleContentReady;
    for(FName Id:Settings->RequiredPuzzleIds)if(!Solved.Contains(Id))return false;
    return true;
}
bool ALBDirector::IsGameplay() const
{
    return !bCaptured&&!bSucceeded&&(Menu.IsEmpty()||Menu==TEXT("Puzzle"));
}
bool ALBDirector::IsEnemyActive() const
{
    return Enemy&&Player&&IsGameplay()&&(Stage==ELBStage::Objectives||Stage==ELBStage::Ready||Stage==ELBStage::Unlocking||Stage==ELBStage::Escape);
}
void ALBDirector::SetStage(ELBStage NewStage)
{
    Stage=NewStage;StageTime=0;
    OnStageChanged.Broadcast(Stage);
    UE_LOG(LogTemp,Display,TEXT("LB_STAGE %s"),*UEnum::GetValueAsString(Stage));
}
void ALBDirector::Notify(const FString& Text,float Seconds){Toast=Text;ToastRemaining=Seconds;}
void ALBDirector::SetMenu(const FString& NewMenu)
{
    if(Stage==ELBStage::Unlocking)CancelExitUnlock();
    if(Player){Player->bInteractHeld=false;Player->GetCharacterMovement()->StopMovementImmediately();}
    Menu=NewMenu;
    const bool bFreeze=!Menu.IsEmpty()&&Menu!=TEXT("Puzzle");
    UGameplayStatics::SetGamePaused(this,bFreeze);
    for(auto Voice:AudioVoices)if(Voice.IsValid())Voice->SetPaused(bFreeze);
    if(APlayerController* PC=UGameplayStatics::GetPlayerController(this,0))
    {
        PC->bShowMouseCursor=!Menu.IsEmpty();PC->bEnableClickEvents=!Menu.IsEmpty();PC->bEnableMouseOverEvents=!Menu.IsEmpty();
        if(Menu.IsEmpty()) {FInputModeGameOnly Mode;PC->SetInputMode(Mode);}
        else {FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);PC->SetInputMode(Mode);}
    }
}
void ALBDirector::ResetAudio()
{
    for(auto Voice:AudioVoices)if(Voice.IsValid())Voice->Stop();
    AudioVoices.Reset();RadioAudio=nullptr;AmbientAudio=nullptr;
    RadioRemaining=0;PlayingZone=NAME_None;
}
void ALBDirector::PlaySound(USoundBase* Sound,FVector Position,float Volume,bool bSpatial)
{
    if(!Sound||bTestMode||!Preferences)return;
    UAudioComponent* Voice=bSpatial?UGameplayStatics::SpawnSoundAtLocation(this,Sound,Position,FRotator::ZeroRotator,Volume*Preferences->Volume):UGameplayStatics::SpawnSound2D(this,Sound,Volume*Preferences->Volume);
    if(Voice) AudioVoices.Add(Voice);
}
void ALBDirector::ApplyPreferences(bool bSave)
{
    Preferences->Volume=FMath::Clamp(Preferences->Volume,0.f,1.f);
    Preferences->Brightness=FMath::Clamp(Preferences->Brightness,.4f,2.f);
    Preferences->FOV=FMath::Clamp(Preferences->FOV,65.f,105.f);
    Preferences->Sensitivity=FMath::Clamp(Preferences->Sensitivity,.04f,.4f);
    if(Player)
    {
        auto* Camera=Player->GetFirstPersonCameraComponent();
        Camera->SetFieldOfView(Preferences->FOV); Camera->PostProcessBlendWeight=1;
        Camera->PostProcessSettings.bOverride_AutoExposureBias=true;
        Camera->PostProcessSettings.AutoExposureBias=FMath::Log2(Preferences->Brightness);
    }
    if(AmbientAudio)AmbientAudio->SetVolumeMultiplier(Preferences->Volume*.18f);
    if(bSave)UGameplayStatics::SaveGameToSlot(Preferences,TEXT("LastBroadcast_Preferences"),0);
}
void ALBDirector::StartNewGame()
{
    ResetAudio();bCaptured=false;bSucceeded=false;bIntroVoicePlayed=false;bEndView=false;bIntroLightsCut=false;
    if(IsValid(EndCameraActor)){EndCameraActor->Destroy();EndCameraActor=nullptr;}
    Solved.Reset();CompletedEvents.Reset();Checkpoint=nullptr;SelectedZone=NAME_None;UnlockProgress=0;Cooldown=0;Grace=0;
    bPower=false;bExit=false;FailureTime=-1;Subtitle.Empty();Toast.Empty();ToastRemaining=0;RecordedSteps.Reset();
    SetStage(ELBStage::Outside);
    if(Player)
    {
        Player->SetActorLocation(FVector(1700,2180,96));Player->ResetTransient();
        if(auto* PC=Cast<APlayerController>(Player->Controller)){PC->SetControlRotation(FRotator(0,-90,0));PC->SetViewTarget(Player);}
    }
    if(Enemy)Enemy->ResetEnemy(FTransform(FRotator(0,0,0),EnemyStart),ELBEnemyState::Inactive);
    for(ALBDoor* Door:Doors)Door->ResetDoor();
    RefreshWorld();SetMenu(TEXT(""));ApplyPreferences();
    if(Settings->Hum&&!bTestMode)
    {
        AmbientAudio=UGameplayStatics::SpawnSound2D(this,Settings->Hum,Preferences->Volume*.18f);
        if(AmbientAudio)AudioVoices.Add(AmbientAudio);
    }
    Notify(TEXT("WASD 이동 · 마우스 시선 · E 상호작용 · Shift 달리기 · F 손전등"),7);
}
void ALBDirector::BeginEntrance()
{
    if(Stage!=ELBStage::Outside||bCaptured)return;
    SetStage(ELBStage::Intro);Player->GetCharacterMovement()->StopMovementImmediately();Player->bInteractHeld=false;
    for(ALBDoor* Door:Doors)if(Door->bExternal){Door->SetOpen(false);Door->bLocked=true;}
    PlaySound(Settings->DoorSound,Player->GetActorLocation());
}
void ALBDirector::FinishIntro()
{
    if(Stage!=ELBStage::Intro)return;
    CompletedEvents.AddUnique(TEXT("Intro"));Subtitle.Empty();SetStage(ELBStage::Explore);RefreshWorld();
    SaveCheckpoint(TEXT("C0"),IntroSpawn,FRotator(0,-90,0));Player->bInteractHeld=false;
    Notify(TEXT("현관이 잠겼다. 전력을 복구할 방법을 찾자。"),5);
}
void ALBDirector::FinishReveal()
{
    if(Stage!=ELBStage::PowerReveal)return;
    CompletedEvents.AddUnique(TEXT("PowerReveal"));SetStage(ELBStage::Objectives);
    if(Enemy){Enemy->State=ELBEnemyState::Patrol;Enemy->SetActorHiddenInGame(false);}
    SaveCheckpoint(TEXT("C1"),PowerRespawn,FRotator(0,90,0));
    Notify(TEXT("교무실에서 방송 장치의 사용법을 확인하자。"),5);
}
void ALBDirector::SkipCinematic()
{
    if(StageTime<1)return;
    if(Stage==ELBStage::Intro){for(auto Voice:AudioVoices)if(Voice.IsValid()&&Voice!=AmbientAudio)Voice->Stop();FinishIntro();}
    else if(Stage==ELBStage::Ending)FinishEnding();
}
ALBDevice* ALBDirector::FindDevice(FName Id,ELBDevice Kind) const
{
    for(ALBDevice* D:Devices)if(D&&D->StableId==Id&&D->Kind==Kind)return D;
    return nullptr;
}
void ALBDirector::Use(AActor* Actor)
{
    if(!Actor||!IsGameplay()||Stage==ELBStage::Intro||Stage==ELBStage::PowerReveal||Stage==ELBStage::Ending||Stage==ELBStage::Complete)return;
    if(ALBDoor* Door=Cast<ALBDoor>(Actor)){Door->Use();return;}
    ALBDevice* D=Cast<ALBDevice>(Actor);if(!D)return;
    if(D->Kind==ELBDevice::Note)
    {
        if(D->StableId==TEXT("RADIO_GUIDE"))CompletedEvents.AddUnique(TEXT("RadioGuideRead"));
        ModalTitle=D->DisplayName;ModalText=D->NoteText;SetMenu(TEXT("Note"));return;
    }
    if(D->bRequiresPower&&!bPower){Notify(TEXT("전력 복구 필요"));return;}
    switch(D->Kind)
    {
    case ELBDevice::PowerPuzzle:
    case ELBDevice::MiddlePuzzle:
        if(Solved.Contains(D->StableId)){Notify(TEXT("이미 완료한 장치다。"));return;}
        if(D->Kind==ELBDevice::PowerPuzzle && Stage!=ELBStage::Explore)return;
        if(D->Kind==ELBDevice::MiddlePuzzle && Stage!=ELBStage::Objectives&&Stage!=ELBStage::Ready)return;
        for(FName Id:D->Prerequisites)if(!Solved.Contains(Id)){Notify(TEXT("먼저 해결할 작업이 남아 있다。"));return;}
        ActivePuzzle=D->StableId;ModalTitle=D->DisplayName;
        ModalText=TEXT("퍼즐의 구체적인 풀이와 정답은 아직 미정이다.\n개발용 완료 입력으로 진행 연결을 검증한다.\n\n이 화면을 열어도 적과 방송 쿨타임은 계속 진행된다.");
        if(!PrototypePuzzlesEnabled())ModalText=TEXT("이 퍼즐은 아직 구성되지 않았다. 기획 및 구현 완료 후 연결해야 한다.");
        SetMenu(TEXT("Puzzle"));break;
    case ELBDevice::ZoneSelect:
        if(Stage==ELBStage::Objectives||Stage==ELBStage::Ready){SelectedZone=D->StableId;Notify(D->DisplayName+TEXT(" 선택"),1.5f);}break;
    case ELBDevice::RadioStart:
        {FString Reason;if(!TryStartBroadcast(SelectedZone,Reason))Notify(Reason);}break;
    case ELBDevice::ExitConsole:
        {FString Reason;if(!TryBeginExitUnlock(Reason))Notify(Reason);}break;
    default:break;
    }
}
void ALBDirector::OnPuzzleSolved(FName Id)
{
    if(!IsGameplay()||Solved.Contains(Id))return;
    const bool bPowerPuzzle=Id==TEXT("P01");
    if(bPowerPuzzle&&Stage!=ELBStage::Explore)return;
    if(!bPowerPuzzle&&(Stage!=ELBStage::Objectives&&Stage!=ELBStage::Ready))return;
    ALBDevice* D=FindDevice(Id,bPowerPuzzle?ELBDevice::PowerPuzzle:ELBDevice::MiddlePuzzle);
    if(!D)return;
    if(D->bRequiresPower&&!bPower)return;
    for(FName Prerequisite:D->Prerequisites)if(!Solved.Contains(Prerequisite))return;
    Solved.Add(Id);D->SetSolvedVisual(true);ActivePuzzle=NAME_None;
    if(Menu==TEXT("Puzzle"))SetMenu(TEXT(""));
    if(bPowerPuzzle)
    {
        bPower=true;SetStage(ELBStage::PowerReveal);RefreshWorld();
        if(Enemy){Enemy->SetActorHiddenInGame(false);Enemy->State=ELBEnemyState::Inactive;}
        PlaySound(Settings->Warning,EnemyStart,.6f);Notify(TEXT("전력이 복구되었다. 복도에서 소리가 들린다。"),4);
    }
    else
    {
        SetStage(AllObjectivesComplete()?ELBStage::Ready:ELBStage::Objectives);
        FVector Spawn=D->InvestigationPoint;Spawn.Z=96;
        SaveCheckpoint(TEXT("C2"),Spawn,FRotator(0,(D->GetActorLocation()-Spawn).Rotation().Yaw,0));
        Notify(AllObjectivesComplete()?TEXT("방송실에서 현관을 개방하자。"):TEXT("작업 완료. 남은 목표를 확인하자。"),4);
    }
}
bool ALBDirector::TryStartBroadcast(FName ZoneId,FString& Reason)
{
    if(!bPower){Reason=TEXT("전력 복구 필요");return false;}
    if(!IsGameplay()||!Menu.IsEmpty()||(Stage!=ELBStage::Objectives&&Stage!=ELBStage::Ready)){Reason=TEXT("지금은 방송할 수 없다。");return false;}
    if(ZoneId.IsNone()){Reason=TEXT("방송할 구역을 선택하세요");return false;}
    ALBDevice* Speaker=FindDevice(ZoneId,ELBDevice::Speaker);
    if(!Speaker||!Settings->Chime){Reason=TEXT("방송 장치 연결 오류");UE_LOG(LogTemp,Error,TEXT("LB_RADIO missing speaker/audio"));return false;}
    if(Cooldown>0||RadioRemaining>0){Reason=FString::Printf(TEXT("방송 재사용까지 %d초"),FMath::CeilToInt(Cooldown));return false;}
    UAudioComponent* Voice=nullptr;
    if(!bTestMode)
    {
        Voice=UGameplayStatics::SpawnSoundAtLocation(this,Settings->Chime,Speaker->GetActorLocation(),FRotator::ZeroRotator,Preferences->Volume);
        if(!Voice){Reason=TEXT("방송 재생 실패");return false;}
    }
    RadioAudio=Voice;if(Voice)AudioVoices.Add(Voice);
    PlayingZone=ZoneId;Cooldown=Settings->RadioCooldown;RadioRemaining=3;BroadcastSerial++;
    if(Enemy)Enemy->Hear(Speaker->InvestigationPoint,2,GetWorld()->TimeSeconds);
    Notify(Speaker->DisplayName+TEXT(" 방송 중"),2);
    UE_LOG(LogTemp,Display,TEXT("LB_RADIO serial=%d zone=%s cooldown=%.2f origin=%s"),BroadcastSerial,*ZoneId.ToString(),Cooldown,*Speaker->GetActorLocation().ToString());
    return true;
}
bool ALBDirector::TryBeginExitUnlock(FString& Reason)
{
    if(!bPower){Reason=TEXT("전력 복구 필요");return false;}
    if(!AllObjectivesComplete()){Reason=TEXT("현관 개방을 위한 작업이 남아 있다");return false;}
    if(Stage!=ELBStage::Ready||!IsGameplay()||!Menu.IsEmpty()){Reason=TEXT("지금은 개방할 수 없다。");return false;}
    if(!Player||!Cast<ALBDevice>(Player->FocusedActor())||Cast<ALBDevice>(Player->FocusedActor())->Kind!=ELBDevice::ExitConsole){Reason=TEXT("콘솔을 가까이에서 바라보자。");return false;}
    UnlockProgress=0;SetStage(ELBStage::Unlocking);return true;
}
void ALBDirector::CancelExitUnlock()
{
    if(Stage!=ELBStage::Unlocking)return;
    UnlockProgress=0;SetStage(ELBStage::Ready);
    if(Player)Player->bInteractHeld=false;
    Notify(TEXT("조작이 중단되었다. 처음부터 다시 시도하자"),2);
}
void ALBDirector::ConfirmExitOpened()
{
    if(Stage!=ELBStage::Unlocking||bExit||bCaptured||bSucceeded)return;
    bExit=true;UnlockProgress=Settings->UnlockTime;SetStage(ELBStage::Escape);EscapeElapsed=0;RecordedSteps.Reset();
    if(Player)Player->bInteractHeld=false;
    if(RadioAudio)RadioAudio->Stop();RadioRemaining=0;PlayingZone=NAME_None;
    RefreshWorld();SaveCheckpoint(TEXT("C3"),ExitRespawn,FRotator(0,180,0));
    ALBDevice* Console=FindDevice(TEXT("EXIT_CONSOLE"),ELBDevice::ExitConsole);
    if(Enemy)Enemy->BeginFinalChase();
    PlaySound(Settings->Alarm,Console?Console->GetActorLocation():ExitRespawn,.8f);AlarmPulse=3;
    Notify(TEXT("현관으로 탈출하자"),5);
}
void ALBDirector::ConfirmEscape()
{
    if(Stage!=ELBStage::Escape||!bExit||bCaptured||bSucceeded)return;
    bSucceeded=true;SetStage(ELBStage::Ending);ResetAudio();Subtitle.Empty();
    if(Enemy){Enemy->State=ELBEnemyState::Stopped;Enemy->SetActorHiddenInGame(true);}
    if(Player)Player->GetCharacterMovement()->StopMovementImmediately();
    EndCameraActor=GetWorld()->SpawnActor<ACameraActor>(EndingCamera,EndingRotation);
    if(auto* Camera=Cast<ACameraActor>(EndCameraActor.Get()))Camera->GetCameraComponent()->SetFieldOfView(70);
    bEndView=false;PlaybackIndex=0;EndStepTime=0;
}
void ALBDirector::FinishEnding()
{
    if(Stage!=ELBStage::Ending)return;
    SetStage(ELBStage::Complete);SetMenu(TEXT("Complete"));
}
void ALBDirector::Capture()
{
    if(bCaptured||bSucceeded||!IsEnemyActive()||Grace>0)return;
    CancelExitUnlock();bCaptured=true;FailureTime=0;
    if(Enemy)Enemy->State=ELBEnemyState::Stopped;
    if(Player){Player->bInteractHeld=false;Player->GetCharacterMovement()->StopMovementImmediately();}
    ResetAudio();PlaySound(Settings->Warning,Player?Player->GetActorLocation():GetActorLocation(),1.f,false);
    if(Menu==TEXT("Puzzle")){Menu.Empty();UGameplayStatics::SetGamePaused(this,false);}
    UE_LOG(LogTemp,Display,TEXT("LB_CAPTURE checkpoint=%s"),Checkpoint?*Checkpoint->CheckpointId.ToString():TEXT("none"));
}
void ALBDirector::SaveCheckpoint(FName Id,FVector Spawn,FRotator Facing)
{
    Checkpoint=NewObject<ULBCheckpoint>(this);Checkpoint->CheckpointId=Id;Checkpoint->Stage=Stage;
    Checkpoint->bPower=bPower;Checkpoint->bExit=bExit;Checkpoint->Solved=Solved;Checkpoint->CompletedEvents=CompletedEvents;
    Checkpoint->PlayerTransform=FTransform(Facing,Spawn);
    Checkpoint->EnemyTransform=FTransform(FRotator(0,0,0),Id==TEXT("C3")?FVector(1830,400,0):EnemyStart);
    if(!UGameplayStatics::SaveGameToSlot(Checkpoint,SaveSlot,0))Notify(TEXT("체크포인트 저장에 실패했다. 현재 세션에서는 재시작할 수 있다。"),5);
}
bool ALBDirector::ContinueGame()
{
    ULBCheckpoint* Loaded=Cast<ULBCheckpoint>(UGameplayStatics::LoadGameFromSlot(SaveSlot,0));
    if(!Loaded||Loaded->DataVersion!=2||Loaded->CheckpointId.IsNone()||Loaded->Stage==ELBStage::Intro||Loaded->Stage==ELBStage::Unlocking||Loaded->Stage==ELBStage::Ending||Loaded->Stage==ELBStage::Complete)
    {Notify(TEXT("사용 가능한 체크포인트가 없다. 새 게임을 시작하자。"),5);return false;}
    Checkpoint=Loaded;RestoreCheckpoint();return true;
}
void ALBDirector::RestoreCheckpoint()
{
    if(!Checkpoint)return;
    ResetAudio();UnlockProgress=0;Cooldown=0;SelectedZone=NAME_None;ActivePuzzle=NAME_None;
    bCaptured=false;bSucceeded=false;FailureTime=-1;Grace=1;Toast.Empty();Subtitle.Empty();
    bPower=Checkpoint->bPower;bExit=Checkpoint->bExit;Solved=Checkpoint->Solved;CompletedEvents=Checkpoint->CompletedEvents;
    SetStage(Checkpoint->Stage);
    if(Player)
    {
        Player->SetActorTransform(Checkpoint->PlayerTransform);Player->ResetTransient();
        if(auto* PC=Cast<APlayerController>(Player->Controller)){PC->SetControlRotation(Checkpoint->PlayerTransform.Rotator());PC->SetViewTarget(Player);}
    }
    for(ALBDoor* Door:Doors)Door->ResetDoor();
    if(Enemy)Enemy->ResetEnemy(Checkpoint->EnemyTransform,bPower?ELBEnemyState::Patrol:ELBEnemyState::Inactive);
    RefreshWorld();SetMenu(TEXT(""));ApplyPreferences();
    if(Stage==ELBStage::Escape)
    {
        EscapeElapsed=0;RecordedSteps.Reset();AlarmPulse=3;
        if(Enemy)Enemy->BeginFinalChase();
        PlaySound(Settings->Alarm,ExitRespawn,.8f);
    }
    if(Settings->Hum&&!bTestMode){AmbientAudio=UGameplayStatics::SpawnSound2D(this,Settings->Hum,Preferences->Volume*.18f);if(AmbientAudio)AudioVoices.Add(AmbientAudio);}
}
void ALBDirector::RefreshWorld()
{
    for(TActorIterator<ALight> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("LB_Power")))It->GetLightComponent()->SetVisibility(bPower||Stage==ELBStage::Outside||(Stage==ELBStage::Intro&&StageTime<1));
    for(ALBDevice* D:Devices)if(D->Kind==ELBDevice::PowerPuzzle||D->Kind==ELBDevice::MiddlePuzzle)D->SetSolvedVisual(Solved.Contains(D->StableId));
    for(ALBDoor* D:Doors)if(D->bExternal)
    {
        D->bLocked=false;D->SetOpen(Stage==ELBStage::Outside||bExit,true);D->bLocked=Stage!=ELBStage::Outside&&!bExit;
    }
}
void ALBDirector::HandlePlayerStep()
{
    if(!Player)return;
    PlaySound(Settings->Footstep,Player->GetActorLocation(),Player->bSprinting?.65f:.3f,false);
    if(Stage==ELBStage::Escape)RecordedSteps.Add(EscapeElapsed);
    if(Player->bSprinting&&Enemy&&FVector::Dist2D(Player->GetActorLocation(),Enemy->GetActorLocation())<=Settings->SprintHearingRange)
        Enemy->Hear(Player->GetActorLocation()-FVector(0,0,96),1,GetWorld()->TimeSeconds);
}
void ALBDirector::PauseMenu()
{
    if(Stage==ELBStage::Intro||Stage==ELBStage::Ending){SkipCinematic();return;}
    if(Menu==TEXT("Note")||Menu==TEXT("Puzzle")){CloseModal();return;}
    if(Menu==TEXT("Settings")){SetMenu(ReturnMenu);return;}
    if(Menu==TEXT("Pause")){SetMenu(TEXT(""));return;}
    if(Menu.IsEmpty()&&!bCaptured&&!bSucceeded)SetMenu(TEXT("Pause"));
}
void ALBDirector::CloseModal(){ActivePuzzle=NAME_None;ModalText.Empty();SetMenu(TEXT(""));}
void ALBDirector::MenuAction(FName Action)
{
    if(Action==TEXT("New")){StartNewGame();return;}
    if(Action==TEXT("Continue")){ContinueGame();return;}
    if(Action==TEXT("Resume")){SetMenu(TEXT(""));return;}
    if(Action==TEXT("Retry")){RestoreCheckpoint();return;}
    if(Action==TEXT("Title")){ResetAudio();SetMenu(TEXT("Title"));return;}
    if(Action==TEXT("Settings")){ReturnMenu=Menu;SetMenu(TEXT("Settings"));return;}
    if(Action==TEXT("Back")){SetMenu(ReturnMenu);return;}
    if(Action==TEXT("Close")){CloseModal();return;}
    if(Action==TEXT("Solve")){if(Menu==TEXT("Puzzle")&&PrototypePuzzlesEnabled())OnPuzzleSolved(ActivePuzzle);return;}
    if(Action==TEXT("Quit")){if(APlayerController* PC=UGameplayStatics::GetPlayerController(this,0))PC->ConsoleCommand(TEXT("quit"));return;}
    const FString A=Action.ToString();const float Direction=A.EndsWith(TEXT("Plus"))?1.f:-1.f;
    if(A.StartsWith(TEXT("Volume")))Preferences->Volume+=Direction*.1f;
    if(A.StartsWith(TEXT("Brightness")))Preferences->Brightness+=Direction*.1f;
    if(A.StartsWith(TEXT("FOV")))Preferences->FOV+=Direction*5;
    if(A.StartsWith(TEXT("Sensitivity")))Preferences->Sensitivity+=Direction*.02f;
    if(A==TEXT("Subtitles"))Preferences->bSubtitles=!Preferences->bSubtitles;
    ApplyPreferences(true);
}
FString ALBDirector::Objective() const
{
    switch(Stage)
    {
    case ELBStage::Outside:return TEXT("학교 구관 안으로 들어가자");
    case ELBStage::Intro:return TEXT("닫힌 현관");
    case ELBStage::Explore:return TEXT("교실 1에서 단서를 찾고 배전실의 전력을 복구하자");
    case ELBStage::PowerReveal:return TEXT("복도에서 무언가 움직인다");
    case ELBStage::Objectives:
        if(!CompletedEvents.Contains(TEXT("RadioGuideRead")))return TEXT("교무실에서 방송 장치의 사용법을 확인하자");
        {FString Targets;for(FName Id:Settings->RequiredPuzzleIds)if(!Solved.Contains(Id)){ALBDevice* D=FindDevice(Id,ELBDevice::MiddlePuzzle);if(!Targets.IsEmpty())Targets+=TEXT(" / ");Targets+=D?D->DisplayName:Id.ToString();}return TEXT("방송으로 길을 확보하고 남은 작업을 마치자 — ")+Targets;}
    case ELBStage::Ready:return TEXT("방송실에서 현관을 개방하자");
    case ELBStage::Unlocking:return TEXT("E를 계속 누르고 콘솔을 바라보자");
    case ELBStage::Escape:return TEXT("현관으로 탈출하자");
    default:return TEXT("");
    }
}
void ALBDirector::Tick(float DT)
{
    Super::Tick(DT);
    if(!Player){Player=Cast<ALBPlayer>(UGameplayStatics::GetPlayerPawn(this,0));if(Player){Player->Director=this;ApplyPreferences();}}
    if(!Menu.IsEmpty()&&Menu!=TEXT("Puzzle"))return;
    if(bCaptured)
    {
        FailureTime+=DT;
        if(FailureTime>=1)SetMenu(TEXT("Failure"));
        return;
    }
    StageTime+=DT;Grace=FMath::Max(0.f,Grace-DT);ToastRemaining=FMath::Max(0.f,ToastRemaining-DT);
    Cooldown=FMath::Max(0.f,Cooldown-DT);RadioRemaining=FMath::Max(0.f,RadioRemaining-DT);
    if(RadioRemaining<=0)PlayingZone=NAME_None;
    AudioVoices.RemoveAll([](const TWeakObjectPtr<UAudioComponent>& P){return !P.IsValid();});
    if(!Player)return;
    if(Stage==ELBStage::Outside||Stage==ELBStage::Escape)
    {
        const ELBDevice Kind=Stage==ELBStage::Outside?ELBDevice::EntranceTrigger:ELBDevice::EscapeTrigger;
        for(ALBDevice* D:Devices)if(D->Kind==Kind)
        {
            FVector Delta=Player->GetActorLocation()-D->GetActorLocation();
            if(FMath::Abs(Delta.X)<=D->TriggerExtent.X&&FMath::Abs(Delta.Y)<=D->TriggerExtent.Y&&FMath::Abs(Delta.Z)<=D->TriggerExtent.Z)
            {if(Kind==ELBDevice::EntranceTrigger)BeginEntrance();else ConfirmEscape();break;}
        }
    }
    if(Stage==ELBStage::Intro)
    {
        if(StageTime>=1&&!bIntroLightsCut){bIntroLightsCut=true;RefreshWorld();PlaySound(Settings->Warning,IntroSpawn,.25f,false);}
        if(StageTime>=3&&!bIntroVoicePlayed){bIntroVoicePlayed=true;PlaySound(Settings->IntroVoice,IntroSpawn,.9f,false);}
        Subtitle=StageTime>=3&&StageTime<Settings->IntroTime-1?TEXT("[교내 방송] 아직 퇴실하지 않은 학생은 방송실로 오세요。"):TEXT("");
        if(StageTime>=Settings->IntroTime)FinishIntro();
    }
    else if(Stage==ELBStage::PowerReveal&&StageTime>=Settings->RevealTime)FinishReveal();
    else if(Stage==ELBStage::Unlocking)
    {
        ALBDevice* Focus=Cast<ALBDevice>(Player->FocusedActor());
        if(!Player->bInteractHeld||!Focus||Focus->Kind!=ELBDevice::ExitConsole||!Menu.IsEmpty())CancelExitUnlock();
        else {UnlockProgress+=DT;if(UnlockProgress>=Settings->UnlockTime)ConfirmExitOpened();}
    }
    else if(Stage==ELBStage::Escape)
    {
        EscapeElapsed+=DT;AlarmPulse-=DT;
        // Alarm playback never replaces the final chase's player target.
        if(AlarmPulse<=0){PlaySound(Settings->Alarm,ExitRespawn,.6f);AlarmPulse=3;}
    }
    else if(Stage==ELBStage::Ending)
    {
        if(StageTime>=1&&!bEndView)
        {
            bEndView=true;if(auto* PC=UGameplayStatics::GetPlayerController(this,0))PC->SetViewTarget(EndCameraActor);
        }
        if(StageTime>=1&&StageTime<5)
        {
            const float Last=RecordedSteps.IsEmpty()?0:RecordedSteps.Last();
            while(PlaybackIndex<RecordedSteps.Num()&&RecordedSteps[PlaybackIndex]<Last-3.7f)++PlaybackIndex;
            float At=PlaybackIndex<RecordedSteps.Num()?RecordedSteps[PlaybackIndex]-FMath::Max(0.f,Last-3.7f):100.f;
            if(StageTime-1>=At){PlaySound(Settings->Footstep,EndingCamera,.8f);++PlaybackIndex;}
            if(RecordedSteps.IsEmpty()){EndStepTime-=DT;if(EndStepTime<=0){PlaySound(Settings->Footstep,EndingCamera,.8f);EndStepTime=.5f;}}
        }
        if(StageTime>=Settings->EndingTime)FinishEnding();
    }
}

ALBGameMode::ALBGameMode(){DefaultPawnClass=ALBPlayer::StaticClass();PlayerControllerClass=ALBController::StaticClass();HUDClass=ALBHUD::StaticClass();}
