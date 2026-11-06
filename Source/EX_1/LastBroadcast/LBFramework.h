#pragma once

#include "CoreMinimal.h"
#include "EX_1Character.h"
#include "Engine/DataAsset.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SaveGame.h"
#include "LBFramework.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class USpotLightComponent;
class UAudioComponent;
class USoundBase;
class UFont;
class ALBDirector;
class ALBEnemy;

UENUM(BlueprintType)
enum class ELBStage : uint8 { Outside, Intro, Explore, PowerReveal, Objectives, Ready, Unlocking, Escape, Ending, Complete };
UENUM(BlueprintType, meta=(ScriptName="LBDeviceKind"))
enum class ELBDevice : uint8 { Note, PowerPuzzle, MiddlePuzzle, ZoneSelect, RadioStart, ExitConsole, Speaker, EntranceTrigger, EscapeTrigger };
UENUM(BlueprintType)
enum class ELBEnemyState : uint8 { Inactive, Patrol, Investigate, Chase, Search, Stopped, FinalChase };

UCLASS(BlueprintType)
class EX_1_API ULBSettings : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Broadcast") float RadioCooldown = 20.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Broadcast") float InvestigationTime = 8.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Progression") float UnlockTime = 6.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Progression") float IntroTime = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Progression") float RevealTime = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Progression") float EndingTime = 7.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float WalkSpeed = 250.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float SprintSpeed = 450.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float SprintSeconds = 4.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float RecoveryDelay = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float RecoverySeconds = 6.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") float PatrolSpeed = 120.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") float ChaseSpeed = 320.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") float SightRange = 900.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") float SightFullAngle = 90.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") float SprintHearingRange = 1400.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") float LostSightSeconds = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") float SearchSeconds = 6.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzles") TArray<FName> RequiredPuzzleIds = { TEXT("MID_A"), TEXT("MID_B") };
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzles") bool bPuzzleContentReady = false;
    // Development-only substitutes. Shipping builds always ignore this flag.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzles") bool bEnablePrototypePuzzles = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> Chime;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> IntroVoice;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> Footstep;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> DoorSound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> Alarm;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> Warning;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> Hum;
};

UCLASS()
class EX_1_API ULBCheckpoint : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 DataVersion = 2;
    UPROPERTY() FName CheckpointId;
    UPROPERTY() ELBStage Stage = ELBStage::Explore;
    UPROPERTY() bool bPower = false;
    UPROPERTY() bool bExit = false;
    UPROPERTY() TArray<FName> Solved;
    UPROPERTY() TArray<FName> CompletedEvents;
    UPROPERTY() FTransform PlayerTransform;
    UPROPERTY() FTransform EnemyTransform;
};

UCLASS()
class EX_1_API ULBPreferences : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() float Volume = .65f;
    UPROPERTY() float Brightness = 1.f;
    UPROPERTY() float FOV = 85.f;
    UPROPERTY() float Sensitivity = .16f;
    UPROPERTY() bool bSubtitles = true;
};

UCLASS(Blueprintable)
class EX_1_API ALBDevice : public AActor
{
    GENERATED_BODY()
public:
    ALBDevice();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") ELBDevice Kind = ELBDevice::Note;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") FName StableId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast", meta=(MultiLine=true)) FString NoteText;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") TArray<FName> Prerequisites;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") bool bRequiresPower = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") FVector InvestigationPoint;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") FVector TriggerExtent = FVector(100,100,150);
    UFUNCTION(BlueprintCallable, Category="Last Broadcast") void SubmitSolved();
    UFUNCTION(BlueprintCallable, Category="Last Broadcast") void SetSolvedVisual(bool bSolved);
};

UCLASS(Blueprintable)
class EX_1_API ALBDoor : public AActor
{
    GENERATED_BODY()
public:
    ALBDoor();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> FrameRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> Hinge;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Leaf;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") FName DoorId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") bool bExternal = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") bool bDefaultOpen = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") float OpenAngle = 105.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bLocked = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bOpen = true;
    UFUNCTION(BlueprintCallable) void SetOpen(bool bValue, bool bInstant = false);
    UFUNCTION(BlueprintCallable) void ResetDoor();
    void Use();
protected:
    virtual void BeginPlay() override;
};

UCLASS()
class EX_1_API ALBPlayer : public AEX_1Character
{
    GENERATED_BODY()
public:
    ALBPlayer();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Component) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USpotLightComponent> Flashlight;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Stamina = 1.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bSprinting = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bInteractHeld = false;
    UPROPERTY(Transient) TObjectPtr<ALBDirector> Director;
    AActor* FocusedActor() const;
    void InteractPressed();
    void InteractReleased();
    void ResetTransient();
    float DevWalkRemaining = 0;
    void Forward(float Value);
    void Back(float Value);
    void Right(float Value);
    void Left(float Value);
    void Turn(float Value);
    void Look(float Value);
    void ToggleFlashlight();
    bool CanMove() const;
protected:
    virtual void BeginPlay() override;
    float SinceSprint = 10.f;
    float StepTime = 0.f;
    bool bSprintExhausted = false;
};

UCLASS(Blueprintable)
class EX_1_API ALBEnemy : public AActor
{
    GENERATED_BODY()
public:
    ALBEnemy();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> BodyRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) ELBEnemyState State = ELBEnemyState::Inactive;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") TArray<FVector> PatrolPoints;
    UPROPERTY(Transient) TObjectPtr<ALBDirector> Director;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector Target;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bSeesPlayer = false;
    void Hear(FVector Position, int32 Priority, float Timestamp);
    UFUNCTION(BlueprintCallable, Category="Last Broadcast") void BeginFinalChase();
    void ResetEnemy(const FTransform& Transform, ELBEnemyState NewState);
    void BuildNavigation();
    bool FindRoute(FVector Destination);
    bool HasSight() const;
    bool ClearPath(FVector From, FVector To, bool bDoorsBlock = false) const;
    const TArray<FVector>& GetRoute() const { return Route; }
protected:
    virtual void BeginPlay() override;
    TArray<uint8> Walkable;
    TArray<FVector> Route;
    FVector GridOrigin = FVector(-2100,-1550,0);
    int32 GridW = 76, GridH = 58;
    float CellSize = 60.f;
    int32 PatrolIndex = 0, RouteIndex = 0;
    float LostSight = 0, Wait = 0, Repath = 0, StepTime = 0, LastSoundTime = -1000;
    int32 LastSoundPriority = 0;
    FVector LastSeen;
    void MoveAlongRoute(float DeltaSeconds, float Speed);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLBStageChanged, ELBStage, NewStage);

UCLASS(Blueprintable)
class EX_1_API ALBDirector : public AActor
{
    GENERATED_BODY()
public:
    ALBDirector();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") TObjectPtr<ULBSettings> Settings;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") FVector IntroSpawn = FVector(550,1850,96);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") FVector PowerRespawn = FVector(-1550,-1180,96);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") FVector EnemyStart = FVector(-800,-220,0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") FVector ExitRespawn = FVector(2110,1010,96);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") FVector EndingCamera = FVector(2220,1140,175);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Last Broadcast") FRotator EndingRotation = FRotator(-5,-110,0);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) ELBStage Stage = ELBStage::Outside;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bPower = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bExit = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FName> Solved;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FName> CompletedEvents;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName SelectedZone;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName PlayingZone;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Cooldown = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float UnlockProgress = 0;
    UPROPERTY(BlueprintAssignable) FLBStageChanged OnStageChanged;
    UPROPERTY(Transient) TObjectPtr<ALBPlayer> Player;
    UPROPERTY(Transient) TObjectPtr<ALBEnemy> Enemy;
    UPROPERTY(Transient) TObjectPtr<ULBCheckpoint> Checkpoint;
    UPROPERTY(Transient) TObjectPtr<ULBPreferences> Preferences;
    UPROPERTY(Transient) TArray<TObjectPtr<ALBDevice>> Devices;
    UPROPERTY(Transient) TArray<TObjectPtr<ALBDoor>> Doors;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> RadioAudio;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> AmbientAudio;
    UPROPERTY(Transient) TObjectPtr<AActor> EndCameraActor;
    // Menu modes: empty is gameplay. Title, Pause, Settings, Note, Puzzle, Failure, Complete.
    FString Menu = TEXT("Title"), ReturnMenu = TEXT("Title"), ModalTitle, ModalText;
    FString Toast, Subtitle;
    float ToastRemaining = 0, StageTime = 0, Grace = 0;
    bool bCaptured = false, bSucceeded = false, bDebug = false, bTestMode = false;
    FName ActivePuzzle;
    int32 BroadcastSerial = 0;
    TArray<float> RecordedSteps;
    TArray<TWeakObjectPtr<UAudioComponent>> AudioVoices;
    float EscapeElapsed = 0;
    FString SaveSlot = TEXT("LastBroadcast_Checkpoint_v2");
    UFUNCTION(BlueprintCallable) void SetStage(ELBStage NewStage);
    UFUNCTION(BlueprintCallable) void StartNewGame();
    UFUNCTION(BlueprintCallable) bool ContinueGame();
    UFUNCTION(BlueprintCallable) void BeginEntrance();
    UFUNCTION(BlueprintCallable) void SkipCinematic();
    UFUNCTION(BlueprintCallable) void OnPuzzleSolved(FName PuzzleId);
    UFUNCTION(BlueprintCallable) bool TryStartBroadcast(FName ZoneId, FString& Reason);
    UFUNCTION(BlueprintCallable) bool TryBeginExitUnlock(FString& Reason);
    UFUNCTION(BlueprintCallable) void CancelExitUnlock();
    UFUNCTION(BlueprintCallable) void ConfirmExitOpened();
    UFUNCTION(BlueprintCallable) void ConfirmEscape();
    UFUNCTION(BlueprintCallable) void Capture();
    UFUNCTION(BlueprintCallable) void RestoreCheckpoint();
    UFUNCTION(BlueprintCallable) void SaveCheckpoint(FName Id, FVector Spawn, FRotator Facing);
    UFUNCTION(BlueprintCallable) void Use(AActor* Actor);
    UFUNCTION(BlueprintCallable) void PauseMenu();
    UFUNCTION(BlueprintCallable) void CloseModal();
    UFUNCTION(BlueprintCallable) void SetMenu(const FString& NewMenu);
    UFUNCTION(BlueprintCallable) void Notify(const FString& Text, float Seconds = 3.f);
    UFUNCTION(BlueprintCallable) bool AllObjectivesComplete() const;
    UFUNCTION(BlueprintCallable) bool PrototypePuzzlesEnabled() const;
    UFUNCTION(BlueprintCallable) FString Objective() const;
    UFUNCTION(BlueprintCallable) ALBDevice* FindDevice(FName Id, ELBDevice Kind) const;
    void MenuAction(FName Action);
    void HandlePlayerStep();
    void PlaySound(USoundBase* Sound, FVector Position, float Volume = 1.f, bool bSpatial = true);
    void ApplyPreferences(bool bSave = false);
    void RefreshWorld();
    void ResetAudio();
    bool IsGameplay() const;
    bool IsEnemyActive() const;
    void RunFunctionalTests();
    float CaptureFade() const { return FMath::Clamp(FailureTime,0.f,1.f); }
protected:
    virtual void BeginPlay() override;
    float RadioRemaining = 0, FailureTime = -1, AlarmPulse = 0, EndStepTime = 0;
    bool bIntroVoicePlayed = false, bEndView = false;
    bool bIntroLightsCut = false;
    int32 PlaybackIndex = 0;
    void FinishIntro();
    void FinishReveal();
    void FinishEnding();
};

UCLASS()
class EX_1_API ALBController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void SetupInputComponent() override;
    UFUNCTION(Exec) void LBStatus();
    UFUNCTION(Exec) void LBTest();
    UFUNCTION(Exec) void LBSolve(FName PuzzleId);
    UFUNCTION(Exec) void LBTeleport(FName LocationId);
    UFUNCTION(Exec) void LBWalk(float Seconds);
    UFUNCTION(Exec) void LBPhoto(const FString& Name);
    void EscapeKey(); void EnterKey(); void SpaceKey(); void DebugKey();
    ALBDirector* Director() const;
};

UCLASS()
class EX_1_API ALBHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
    UPROPERTY(Transient) TObjectPtr<UFont> KoreanFont;
protected:
    void Text(const FString& Value, float X, float Y, float Size, FLinearColor Color = FLinearColor::White);
    void Wrapped(const FString& Value, float X, float& Y, float Width, float Size, FLinearColor Color = FLinearColor::White);
    void Rect(float X, float Y, float W, float H, FLinearColor Color);
    void Button(FName Id, const FString& Label, float X, float Y, float W = 380.f);
    float UI = 1.f;
};

UCLASS()
class EX_1_API ALBGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ALBGameMode();
};
