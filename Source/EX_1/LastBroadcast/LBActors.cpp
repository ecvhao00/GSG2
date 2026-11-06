#include "LBFramework.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

ALBDevice::ALBDevice()
{
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DeviceMesh"));
    SetRootComponent(Mesh);
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Mesh->SetCanEverAffectNavigation(false);
}

void ALBDevice::SubmitSolved()
{
    for (TActorIterator<ALBDirector> It(GetWorld()); It; ++It) { It->OnPuzzleSolved(StableId); break; }
}

void ALBDevice::SetSolvedVisual(bool bSolved)
{
    if (UMaterialInstanceDynamic* MI = Mesh->CreateAndSetMaterialInstanceDynamic(0))
    {
        MI->SetVectorParameterValue(TEXT("Color"), bSolved ? FLinearColor(.1f,.7f,.4f) : FLinearColor(.5f,.25f,.08f));
    }
}

ALBDoor::ALBDoor()
{
    PrimaryActorTick.bCanEverTick = true;
    FrameRoot=CreateDefaultSubobject<USceneComponent>(TEXT("FrameRoot"));SetRootComponent(FrameRoot);
    Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));Hinge->SetupAttachment(FrameRoot);
    Leaf = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorLeaf")); Leaf->SetupAttachment(Hinge);
    Leaf->SetMobility(EComponentMobility::Movable);
    Leaf->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Leaf->SetCanEverAffectNavigation(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
    Leaf->SetStaticMesh(Cube.Object);
    Leaf->SetRelativeLocation(FVector(0,65,120)); Leaf->SetRelativeScale3D(FVector(.12f,1.3f,2.4f));
}

void ALBDoor::BeginPlay() { Super::BeginPlay(); ResetDoor(); }
void ALBDoor::SetOpen(bool bValue, bool bInstant)
{
    if (bValue && bLocked) return;
    const bool bChanged = bOpen != bValue;
    bOpen = bValue;
    // An open leaf cannot snag a capsule or obstruct the second doorway.
    Leaf->SetCollisionEnabled(bOpen ? ECollisionEnabled::QueryOnly : ECollisionEnabled::QueryAndPhysics);
    Leaf->SetCollisionResponseToAllChannels(bOpen ? ECR_Ignore : ECR_Block);
    Leaf->SetCollisionResponseToChannel(ECC_GameTraceChannel2,ECR_Block);
    if (bInstant) Hinge->SetRelativeRotation(FRotator(0,bOpen ? OpenAngle : 0,0));
    if (bChanged)
    {
        for (TActorIterator<ALBDirector> It(GetWorld()); It; ++It) { It->PlaySound(It->Settings->DoorSound,GetActorLocation()); break; }
    }
}
void ALBDoor::ResetDoor() { bLocked = false; SetOpen(bDefaultOpen,true); }
void ALBDoor::Use()
{
    for (TActorIterator<ALBDirector> It(GetWorld()); It; ++It)
    {
        if (bLocked) It->Notify(TEXT("방송실 비상 개방 장치와 연결되어 있다."));
        else if (!bExternal || !It->bExit) SetOpen(!bOpen);
        break;
    }
}
void ALBDoor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Hinge->SetRelativeRotation(FMath::RInterpTo(Hinge->GetRelativeRotation(),FRotator(0,bOpen ? OpenAngle : 0,0),DeltaSeconds,6.f));
}

ALBPlayer::ALBPlayer()
{
    PrimaryActorTick.bCanEverTick = true;
    GetFirstPersonCameraComponent()->SetupAttachment(GetCapsuleComponent());
    GetFirstPersonCameraComponent()->SetRelativeLocation(FVector(0,0,62));
    GetFirstPersonCameraComponent()->SetRelativeRotation(FRotator::ZeroRotator);
    GetFirstPersonCameraComponent()->bEnableFirstPersonFieldOfView = false;
    GetFirstPersonCameraComponent()->bEnableFirstPersonScale = false;
    GetFirstPersonMesh()->SetVisibility(false,true); GetMesh()->SetVisibility(false,true);
    GetCharacterMovement()->MaxWalkSpeed = 250.f;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxStepHeight = 25;
    bUseControllerRotationYaw = true;
    Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
    Flashlight->SetupAttachment(GetFirstPersonCameraComponent());
    Flashlight->SetRelativeLocation(FVector(15,12,-10));
    Flashlight->SetIntensity(100); Flashlight->SetAttenuationRadius(1800);
    Flashlight->SetInnerConeAngle(17); Flashlight->SetOuterConeAngle(32);
    Flashlight->SetLightColor(FLinearColor(.82f,.87f,1.f));
    Flashlight->SetCastShadows(true);
}
void ALBPlayer::BeginPlay()
{
    Super::BeginPlay();
    for (TActorIterator<ALBDirector> It(GetWorld()); It; ++It) { Director = *It; break; }
}
void ALBPlayer::SetupPlayerInputComponent(UInputComponent* Component)
{
    // Inherit the first-person template body/camera, with a dedicated horror input layer.
    Component->BindAxis(TEXT("LBForward"),this,&ALBPlayer::Forward);
    Component->BindAxis(TEXT("LBRight"),this,&ALBPlayer::Right);
    Component->BindAxisKey(EKeys::MouseX,this,&ALBPlayer::Turn);
    Component->BindAxisKey(EKeys::MouseY,this,&ALBPlayer::Look);
    Component->BindKey(EKeys::E,IE_Pressed,this,&ALBPlayer::InteractPressed);
    Component->BindKey(EKeys::E,IE_Released,this,&ALBPlayer::InteractReleased).bExecuteWhenPaused = true;
    Component->BindKey(EKeys::F,IE_Pressed,this,&ALBPlayer::ToggleFlashlight);
}
bool ALBPlayer::CanMove() const
{
    return Director && Director->IsGameplay() && Director->Menu.IsEmpty() && Director->Stage != ELBStage::Intro && Director->Stage != ELBStage::Ending && Director->Stage != ELBStage::Complete;
}
void ALBPlayer::Forward(float V) { if (CanMove()) AddMovementInput(GetActorForwardVector(),V); }
void ALBPlayer::Back(float V) { Forward(-V); }
void ALBPlayer::Right(float V) { if (CanMove()) AddMovementInput(GetActorRightVector(),V); }
void ALBPlayer::Left(float V) { Right(-V); }
void ALBPlayer::Turn(float V) { if (CanMove()) AddControllerYawInput(V * (Director->Preferences->Sensitivity / .16f)); }
void ALBPlayer::Look(float V) { if (CanMove()) AddControllerPitchInput(-V * (Director->Preferences->Sensitivity / .16f)); }
void ALBPlayer::ToggleFlashlight() { if (CanMove()) Flashlight->ToggleVisibility(); }
AActor* ALBPlayer::FocusedActor() const
{
    if (!GetWorld()) return nullptr;
    FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
    FVector End = Start + GetFirstPersonCameraComponent()->GetForwardVector()*200.f;
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(LBFocus),false,this);
    GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_GameTraceChannel2,Params);
    return Hit.GetActor();
}
void ALBPlayer::InteractPressed()
{
    bInteractHeld = true;
    if (Director && Director->Menu.IsEmpty() && Director->IsGameplay()) Director->Use(FocusedActor());
}
void ALBPlayer::InteractReleased()
{
    bInteractHeld = false;
    if (Director) Director->CancelExitUnlock();
}
void ALBPlayer::ResetTransient()
{
    Stamina = 1; SinceSprint = 10; bSprinting = false; bInteractHeld = false; bSprintExhausted=false; DevWalkRemaining=0;
    GetCharacterMovement()->StopMovementImmediately(); Flashlight->SetVisibility(true);
}
void ALBPlayer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Director) return;
    const ULBSettings* S = Director->Settings;
    const APlayerController* PC = Cast<APlayerController>(GetController());
    const bool bWantsSprint = PC && PC->IsInputKeyDown(EKeys::LeftShift);
    if(!bWantsSprint)bSprintExhausted=false;
    const bool bMoving = GetVelocity().SizeSquared2D()>100.f;
    bSprinting = CanMove() && bMoving && bWantsSprint && Stamina > .001f && !bSprintExhausted;
    GetCharacterMovement()->MaxWalkSpeed = bSprinting ? S->SprintSpeed : S->WalkSpeed;
    if (bSprinting) { Stamina=FMath::Max(0.f,Stamina-DeltaSeconds/S->SprintSeconds); SinceSprint=0;if(Stamina<=.001f)bSprintExhausted=true; }
    else { SinceSprint += DeltaSeconds; if (SinceSprint >= S->RecoveryDelay) Stamina=FMath::Min(1.f,Stamina+DeltaSeconds/S->RecoverySeconds); }
    StepTime -= DeltaSeconds;
#if !UE_BUILD_SHIPPING
    if(DevWalkRemaining>0&&CanMove()){Forward(1);DevWalkRemaining=FMath::Max(0.f,DevWalkRemaining-DeltaSeconds);}
#endif
    if (CanMove() && bMoving && StepTime <= 0 && GetCharacterMovement()->IsMovingOnGround())
    {
        StepTime = bSprinting ? .3f : .52f;
        Director->HandlePlayerStep();
    }
}

ALBEnemy::ALBEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    BodyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Body")); SetRootComponent(BodyRoot);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere"));
    const struct { const TCHAR* Name; FVector Position; FVector Scale; } Pieces[] = {
        {TEXT("Torso"),{0,0,120},{.34f,.58f,.72f}}, {TEXT("Head"),{0,0,178},{.34f,.34f,.39f}},
        {TEXT("LegL"),{0,-17,44},{.19f,.20f,.88f}}, {TEXT("LegR"),{0,17,44},{.19f,.20f,.88f}},
        {TEXT("ArmL"),{0,-40,106},{.15f,.17f,.92f}}, {TEXT("ArmR"),{0,40,106},{.15f,.17f,.92f}}
    };
    for (const auto& Piece : Pieces)
    {
        auto* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Piece.Name); Mesh->SetupAttachment(BodyRoot);
        Mesh->SetStaticMesh(Piece.Name == FString(TEXT("Head")) ? Sphere.Object : Cube.Object);
        Mesh->SetRelativeLocation(Piece.Position); Mesh->SetRelativeScale3D(Piece.Scale);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCanEverAffectNavigation(false);
    }
}
void ALBEnemy::BeginPlay()
{
    Super::BeginPlay();
    for (TActorIterator<ALBDirector> It(GetWorld()); It; ++It) { Director=*It; break; }
    SetActorHiddenInGame(true);
}
bool ALBEnemy::ClearPath(FVector From,FVector To,bool bDoorsBlock) const
{
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(LBPath),false,this);
    if (Director && Director->Player) Params.AddIgnoredActor(Director->Player);
    FCollisionObjectQueryParams Objects(ECC_WorldStatic);
    if (bDoorsBlock)
    {
        Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
        if(Director)for(ALBDoor* Door:Director->Doors)if(Door->bOpen)Params.AddIgnoredActor(Door);
    }
    return !GetWorld()->SweepSingleByObjectType(Hit,From+FVector(0,0,95),To+FVector(0,0,95),FQuat::Identity,Objects,FCollisionShape::MakeCapsule(28,75),Params);
}
void ALBEnemy::BuildNavigation()
{
    GridW=84; GridH=64;
    Walkable.SetNumZeroed(GridW*GridH);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(LBGrid),false,this);
    for(int32 Y=0;Y<GridH;++Y) for(int32 X=0;X<GridW;++X)
    {
        FVector P=GridOrigin+FVector(X*CellSize,Y*CellSize,0);
        FHitResult Ground;
        bool bFloor=GetWorld()->LineTraceSingleByObjectType(Ground,P+FVector(0,0,30),P-FVector(0,0,80),FCollisionObjectQueryParams(ECC_WorldStatic),Params);
        Walkable[Y*GridW+X]=bFloor && ClearPath(P,P+FVector(.01f,0,0));
    }
    UE_LOG(LogTemp,Display,TEXT("LB_NAV built %d nodes"),Walkable.Num());
}
bool ALBEnemy::FindRoute(FVector Destination)
{
    Route.Reset(); RouteIndex=0;
    if (Walkable.IsEmpty()) BuildNavigation();
    auto Nearest=[&](FVector P) {
        int32 Best=INDEX_NONE; float Dist=FLT_MAX;
        for(int32 I=0;I<Walkable.Num();++I) if(Walkable[I])
        {
            FVector V=GridOrigin+FVector((I%GridW)*CellSize,(I/GridW)*CellSize,0);
            float D=FVector::DistSquared2D(P,V);
            if(D<Dist && ClearPath(P,V)) { Best=I; Dist=D; }
        }
        return Best;
    };
    int32 Start=Nearest(GetActorLocation()), Goal=Nearest(Destination);
    if(Start==INDEX_NONE || Goal==INDEX_NONE) return false;
    TArray<float> Cost; Cost.Init(FLT_MAX,Walkable.Num());
    TArray<int32> Parent; Parent.Init(INDEX_NONE,Walkable.Num());
    TArray<int32> Open={Start}; TSet<int32> Closed; Cost[Start]=0;
    auto Pos=[&](int32 I){return GridOrigin+FVector((I%GridW)*CellSize,(I/GridW)*CellSize,0);};
    while(!Open.IsEmpty())
    {
        int32 BestAt=0; float BestScore=FLT_MAX;
        for(int32 K=0;K<Open.Num();++K) { float V=Cost[Open[K]]+FVector::Dist2D(Pos(Open[K]),Pos(Goal)); if(V<BestScore){BestScore=V;BestAt=K;} }
        int32 Current=Open[BestAt]; Open.RemoveAtSwap(BestAt);
        if(Current==Goal)
        {
            for(int32 I=Goal;I!=Start && I!=INDEX_NONE;I=Parent[I]) Route.Insert(Pos(I),0);
            if(ClearPath(Pos(Goal),Destination)) Route.Add(FVector(Destination.X,Destination.Y,0));
            return true;
        }
        Closed.Add(Current);
        int32 CX=Current%GridW,CY=Current/GridW;
        for(int32 DY=-1;DY<=1;++DY) for(int32 DX=-1;DX<=1;++DX)
        {
            if(!DX&&!DY) continue;
            int32 NX=CX+DX,NY=CY+DY;
            if(NX<0||NY<0||NX>=GridW||NY>=GridH)continue;
            int32 N=NY*GridW+NX;
            if(!Walkable[N]||Closed.Contains(N))continue;
            if(DX&&DY && (!Walkable[CY*GridW+NX] || !Walkable[NY*GridW+CX]))continue;
            bool bDoorBlocked=false;
            if(Director) for(ALBDoor* Door:Director->Doors) if(Door->bLocked && FVector::Dist2D(Pos(N),Door->GetActorLocation()+Door->GetActorRightVector()*65)<95) {bDoorBlocked=true;break;}
            if(bDoorBlocked)continue;
            float NewCost=Cost[Current]+CellSize*((DX&&DY)?1.4142f:1.f);
            if(NewCost<Cost[N]) {Cost[N]=NewCost;Parent[N]=Current;Open.AddUnique(N);}
        }
    }
    return false;
}
bool ALBEnemy::HasSight() const
{
    if(!Director||!Director->Player||!Director->IsEnemyActive())return false;
    FVector Eyes=GetActorLocation()+FVector(0,0,175);
    FVector P=Director->Player->GetActorLocation()+FVector(0,0,50);
    FVector D=P-Eyes;
    if(D.Size()>Director->Settings->SightRange) return false;
    if(FVector::DotProduct(GetActorForwardVector(),D.GetSafeNormal2D())<FMath::Cos(FMath::DegreesToRadians(Director->Settings->SightFullAngle*.5f)))return false;
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(LBSight),false,this);
    const bool bHit=GetWorld()->LineTraceSingleByChannel(Hit,Eyes,P,ECC_Visibility,Params);
    return !bHit || Hit.GetActor()==Director->Player;
}
void ALBEnemy::Hear(FVector Position,int32 Priority,float Timestamp)
{
    if(!Director||!Director->IsEnemyActive()||HasSight())return;
    if(Director->Stage==ELBStage::Escape&&Director->bExit)return;
    if(Timestamp<LastSoundTime || (FMath::IsNearlyEqual(Timestamp,LastSoundTime,.001f)&&Priority<LastSoundPriority))return;
    LastSoundTime=Timestamp; LastSoundPriority=Priority;
    Target=Position; State=ELBEnemyState::Investigate; Wait=0; Repath=0;
}
void ALBEnemy::BeginFinalChase()
{
    if(!Director||!Director->Player||!Director->IsEnemyActive()||Director->Stage!=ELBStage::Escape||!Director->bExit)return;
    Target=Director->Player->GetActorLocation();
    if(State==ELBEnemyState::FinalChase)return;
    State=ELBEnemyState::FinalChase;
    LastSeen=Target;LostSight=0;Wait=0;Repath=0;Route.Reset();RouteIndex=0;
    SetActorHiddenInGame(false);
    Director->PlaySound(Director->Settings->Warning,GetActorLocation(),1.f,false);
}
void ALBEnemy::ResetEnemy(const FTransform& Transform,ELBEnemyState NewState)
{
    SetActorTransform(Transform); State=NewState; Target=GetActorLocation();
    bSeesPlayer=false; Wait=0; LostSight=0; Repath=0; Route.Reset(); RouteIndex=0; LastSoundTime=-1000; LastSoundPriority=0;
    SetActorHiddenInGame(NewState==ELBEnemyState::Inactive);
}
void ALBEnemy::MoveAlongRoute(float DT,float Speed)
{
    if(RouteIndex>=Route.Num())return;
    // Smooth corners only when the full body fits through the shortcut.
    while(RouteIndex+1<Route.Num() && ClearPath(GetActorLocation(),Route[RouteIndex+1])) ++RouteIndex;
    FVector Delta=Route[RouteIndex]-GetActorLocation(); Delta.Z=0;
    if(Delta.Size()<22){++RouteIndex;return;}
    FVector Next=GetActorLocation()+Delta.GetSafeNormal()*FMath::Min(Speed*DT,Delta.Size());
    if(Director) for(ALBDoor* Door:Director->Doors)
    {
        FVector Center=Door->GetActorLocation()+Door->GetActorRightVector()*65;
        if(FVector::Dist2D(GetActorLocation(),Center)<150)
        {
            if(Door->bLocked && FVector::Dist2D(Next,Center)<95) { Repath=0;return; }
            if(!Door->bLocked)Door->SetOpen(true,true);
        }
    }
    if(ClearPath(GetActorLocation(),Next))
    {
        SetActorLocation(Next); SetActorRotation(FRotator(0,Delta.Rotation().Yaw,0));
        float Bob=FMath::Sin(GetWorld()->TimeSeconds*6.f)*1.7f;
        BodyRoot->SetRelativeScale3D(FVector(1,1,1.f+Bob*.001f));
        StepTime-=DT;
        if(StepTime<=0) {StepTime=(State==ELBEnemyState::Chase||State==ELBEnemyState::FinalChase)?.4f:.7f;Director->PlaySound(Director->Settings->Footstep,GetActorLocation(),.85f);}
    }
    else {Repath=0;}
}
void ALBEnemy::Tick(float DT)
{
    Super::Tick(DT);
    if(!Director||!Director->IsEnemyActive()||Director->Grace>0)return;
    const ULBSettings* S=Director->Settings;
    SetActorHiddenInGame(false);
    bSeesPlayer=HasSight();
    // Exit opening locks the target to the player, independently of perception.
    // Ordinary sight/hearing/search rules continue to apply before this stage.
    const bool bFinalChase=Director->Stage==ELBStage::Escape&&Director->bExit;
    if(bFinalChase)
    {
        if(State!=ELBEnemyState::FinalChase)BeginFinalChase();
        Target=Director->Player->GetActorLocation();LastSeen=Target;LostSight=0;Wait=0;
    }
    else if(bSeesPlayer)
    {
        if(State!=ELBEnemyState::Chase){Wait=1.f;Director->PlaySound(S->Warning,GetActorLocation(),1.f,false);}
        State=ELBEnemyState::Chase; LostSight=0; LastSeen=Director->Player->GetActorLocation(); Target=LastSeen;
    }
    else if(State==ELBEnemyState::Chase)
    {
        LostSight+=DT; Target=LastSeen;
        if(LostSight>=S->LostSightSeconds){State=ELBEnemyState::Search;Wait=0;Target=LastSeen;Repath=0;}
    }
    const FVector PlayerLocation=Director->Player->GetActorLocation();
    if(FVector::Dist2D(PlayerLocation,GetActorLocation())<57 && FMath::Abs(PlayerLocation.Z-96)<80 && ClearPath(GetActorLocation(),FVector(PlayerLocation.X,PlayerLocation.Y,0),true))
    {Director->Capture();return;}
    if(State==ELBEnemyState::Patrol && (Route.IsEmpty()||FVector::Dist2D(Target,GetActorLocation())<45))
    {
        if(PatrolPoints.IsEmpty())return;
        Target=PatrolPoints[PatrolIndex++%PatrolPoints.Num()]; Repath=0;
    }
    float Distance=FVector::Dist2D(GetActorLocation(),Target);
    if(State==ELBEnemyState::Chase && Wait>0){Wait-=DT;return;}
    if((State==ELBEnemyState::Investigate||State==ELBEnemyState::Search) && (Distance<55 || (Repath>0&&Route.IsEmpty())))
    {
        Wait+=DT;
        SetActorRotation(FRotator(0,GetActorRotation().Yaw+DT*24,0));
        const float Duration=State==ELBEnemyState::Investigate?S->InvestigationTime:S->SearchSeconds;
        if(Wait>=Duration){State=ELBEnemyState::Patrol;Route.Reset();Repath=0;Wait=0;}
        return;
    }
    Repath-=DT;
    const bool bChasing=State==ELBEnemyState::Chase||State==ELBEnemyState::FinalChase;
    if(Repath<=0) {FindRoute(Target);Repath=bChasing?.55f:2.f;}
    MoveAlongRoute(DT,bChasing?S->ChaseSpeed:S->PatrolSpeed);
}
