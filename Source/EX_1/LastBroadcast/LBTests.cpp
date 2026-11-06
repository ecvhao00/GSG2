#include "LBFramework.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Components/StaticMeshComponent.h"

void ALBDirector::RunFunctionalTests()
{
#if !UE_BUILD_SHIPPING
    if(!bTestMode)
    {
        Notify(TEXT("회귀 검증은 별도 -LBFunctionalTest 실행에서만 지원한다。"));return;
    }
    int32 Passed=0,Failed=0;FString Report=TEXT("{\n  \"suite\": \"LastBroadcast.RealMapRegression\",\n  \"checks\": [\n");
    auto Check=[&](const TCHAR* Name,bool bPass)
    {
        UE_LOG(LogTemp,Display,TEXT("LB_CHECK %s %s"),bPass?TEXT("PASS"):TEXT("FAIL"),Name);
        Report+=FString::Printf(TEXT("    {\"name\":\"%s\",\"passed\":%s},\n"),Name,bPass?TEXT("true"):TEXT("false"));
        if(bPass)++Passed;else ++Failed;
    };
    Check(TEXT("map.player_and_enemy_and_settings"),Player&&Enemy&&Settings);
    if(!Player||!Enemy||!Settings){FPlatformMisc::RequestExitWithStatus(false,1);return;}
    auto ResetMid=[&]()
    {
        StartNewGame();BeginEntrance();StageTime=1.1f;SkipCinematic();OnPuzzleSolved(TEXT("P01"));Tick(Settings->RevealTime+.01f);
    };
    auto AimAtConsole=[&]()
    {
        Player->SetActorLocation(FVector(2630,1140,96));Player->SetActorRotation(FRotator::ZeroRotator);
        if(Player->Controller)Player->Controller->SetControlRotation(FRotator::ZeroRotator);
        Player->GetFirstPersonCameraComponent()->SetWorldRotation(FRotator::ZeroRotator);
    };
    auto CompleteMid=[&](){OnPuzzleSolved(TEXT("MID_A"));OnPuzzleSolved(TEXT("MID_B"));};
    auto OpenExit=[&](){AimAtConsole();Player->InteractPressed();Tick(Settings->UnlockTime+.01f);};

    StartNewGame();BeginEntrance();Tick(1.1f);float Before=StageTime;BeginEntrance();
    Check(TEXT("01.entrance_fires_once"),Stage==ELBStage::Intro&&FMath::IsNearlyEqual(Before,StageTime));
    SkipCinematic();
    Check(TEXT("02.skip_commits_locked_entrance_and_C0"),Stage==ELBStage::Explore&&Checkpoint&&Checkpoint->CheckpointId==TEXT("C0")&&CompletedEvents.Contains(TEXT("Intro")));
    FString Reason;
    bool bStarted=TryStartBroadcast(TEXT("Z01"),Reason);Use(FindDevice(TEXT("MID_A"),ELBDevice::MiddlePuzzle));
    Check(TEXT("03.power_gate_rejects_devices_without_spending_cooldown"),!bStarted&&Cooldown==0&&Solved.IsEmpty());
    OnPuzzleSolved(TEXT("P01"));Tick(.8f);Before=StageTime;OnPuzzleSolved(TEXT("P01"));
    Check(TEXT("04.power_success_is_idempotent"),bPower&&Solved.Num()==1&&Stage==ELBStage::PowerReveal&&FMath::IsNearlyEqual(Before,StageTime));
    Tick(Settings->RevealTime);Check(TEXT("04b.reveal_finishes_in_C1"),Stage==ELBStage::Objectives&&Checkpoint->CheckpointId==TEXT("C1"));
    bStarted=TryStartBroadcast(TEXT("Z01"),Reason);bool bUnlocked=TryBeginExitUnlock(Reason);
    Check(TEXT("05.radio_available_before_exit_objectives"),bStarted&&!bUnlocked&&!bExit);
    int32 Serial=BroadcastSerial;FName Frozen=PlayingZone;
    SelectedZone=TEXT("Z02");bool bOther1=TryStartBroadcast(TEXT("Z02"),Reason);bool bOther2=TryStartBroadcast(TEXT("Z03"),Reason);
    Check(TEXT("06.shared_cooldown_and_frozen_broadcast_target"),!bOther1&&!bOther2&&BroadcastSerial==Serial&&PlayingZone==Frozen&&Cooldown==Settings->RadioCooldown);
    for(FName Zone:{FName(TEXT("Z01")),FName(TEXT("Z02")),FName(TEXT("Z03"))})
    {
        Tick(Settings->RadioCooldown+.01f);ALBDevice* Speaker=FindDevice(Zone,ELBDevice::Speaker);
        Enemy->ResetEnemy(FTransform(FRotator::ZeroRotator,EnemyStart),ELBEnemyState::Patrol);
        bStarted=TryStartBroadcast(Zone,Reason);
        Check(*(TEXT("07.speaker_origin.")+Zone.ToString()),Speaker&&bStarted&&PlayingZone==Zone&&Enemy->Target.Equals(Speaker->InvestigationPoint,.1f));
    }
    ResetMid();Player->SetActorLocation(FVector(1830,-420,96));Enemy->ResetEnemy(FTransform(FRotator(0,0,0),FVector(1500,-420,0)),ELBEnemyState::Patrol);Enemy->Tick(.01f);
    Check(TEXT("08a.actual_sight_enters_chase"),Enemy->HasSight()&&Enemy->State==ELBEnemyState::Chase);
    FVector Visual=Enemy->Target;TryStartBroadcast(TEXT("Z01"),Reason);
    Check(TEXT("08b.visual_priority_survives_broadcast"),Enemy->State==ELBEnemyState::Chase&&Enemy->Target.Equals(Visual,.1f));
    float CD=Cooldown;PauseMenu();Tick(9.f);
    Check(TEXT("09.pause_freezes_cooldown"),UGameplayStatics::IsGamePaused(this)&&FMath::IsNearlyEqual(CD,Cooldown));
    PauseMenu();Check(TEXT("09b.resume_preserves_cooldown"),!UGameplayStatics::IsGamePaused(this)&&FMath::IsNearlyEqual(CD,Cooldown));
    ResetMid();CompleteMid();AimAtConsole();
    Check(TEXT("10a.console_is_reachable_by_real_focus_trace"),Player->FocusedActor()==FindDevice(TEXT("EXIT_CONSOLE"),ELBDevice::ExitConsole));
    Player->InteractPressed();Tick(2.f);Player->InteractReleased();
    Check(TEXT("10b.release_resets_all_unlock_progress"),Stage==ELBStage::Ready&&UnlockProgress==0);
    Player->InteractPressed();Tick(.5f);Player->GetFirstPersonCameraComponent()->SetWorldRotation(FRotator(0,180,0));Tick(.1f);
    Check(TEXT("10c.look_away_cancels_unlock"),Stage==ELBStage::Ready&&UnlockProgress==0);
    AimAtConsole();Player->InteractPressed();Tick(.5f);PauseMenu();PauseMenu();Tick(1.f);
    Check(TEXT("11.pause_requires_new_interact_press"),Stage==ELBStage::Ready&&UnlockProgress==0&&!Player->bInteractHeld);
    const FVector EnemyBeforeOpen=Enemy->GetActorLocation();
    OpenExit();ULBCheckpoint* ExitCheckpoint=Checkpoint;ConfirmExitOpened();
    Check(TEXT("12.exit_unlock_commits_once_and_saves_C3"),Stage==ELBStage::Escape&&bExit&&Checkpoint==ExitCheckpoint&&Checkpoint->CheckpointId==TEXT("C3"));
    bool bClosed=false;for(ALBDoor* Door:Doors)if(Door->bExternal)bClosed=Door->bLocked||!Door->bOpen;
    Check(TEXT("12b.external_door_is_physically_open_and_unlocked"),!bClosed);
    Check(TEXT("22.exit_immediately_locks_player_target_without_relocation"),Enemy->State==ELBEnemyState::FinalChase&&Enemy->Target.Equals(Player->GetActorLocation(),.1f)&&Enemy->GetActorLocation().Equals(EnemyBeforeOpen,.1f));
    const FVector BeforeImmediateMove=Enemy->GetActorLocation();
    for(int I=0;I<5;++I)Enemy->Tick(.05f);
    Check(TEXT("22b.final_chase_starts_without_discovery_wait"),FVector::Dist2D(BeforeImmediateMove,Enemy->GetActorLocation())>1);
    const FVector PlayerTarget=Enemy->Target;
    Enemy->Hear(FVector(-1450,900,0),99,GetWorld()->TimeSeconds+1000.f);
    Check(TEXT("23.final_chase_ignores_new_hearing_stimuli"),Enemy->State==ELBEnemyState::FinalChase&&Enemy->Target.Equals(PlayerTarget,.1f));
    bool bEscapeRadio=TryStartBroadcast(TEXT("Z03"),Reason);
    Check(TEXT("23b.escape_radio_cannot_redirect_enemy"),!bEscapeRadio&&Enemy->State==ELBEnemyState::FinalChase);
    Player->SetActorLocation(FVector(1420,-1210,96));
    Enemy->ResetEnemy(FTransform(FRotator(0,90,0),FVector(-1820,1220,0)),ELBEnemyState::Patrol);
    Enemy->Tick(.1f);
    Check(TEXT("24.final_chase_updates_unseen_player_position"),!Enemy->HasSight()&&Enemy->State==ELBEnemyState::FinalChase&&Enemy->Target.Equals(Player->GetActorLocation(),.1f));
    for(int I=0;I<90;++I)Enemy->Tick(.1f);
    Check(TEXT("24b.lost_sight_never_returns_final_chase_to_search_or_patrol"),!bCaptured&&Enemy->State==ELBEnemyState::FinalChase&&Enemy->Target.Equals(Player->GetActorLocation(),.1f));
    Enemy->SetActorLocation(FVector(590,-1000,0));Player->SetActorLocation(FVector(640,-1000,96));Enemy->Tick(.01f);
    Check(TEXT("25.final_chase_cannot_capture_through_partition_wall"),!bCaptured&&Enemy->State==ELBEnemyState::FinalChase);
    ResetMid();OnPuzzleSolved(TEXT("MID_A"));Capture();Tick(1.1f);RestoreCheckpoint();
    Check(TEXT("13.partial_objectives_survive_capture"),Stage==ELBStage::Objectives&&bPower&&Solved.Contains(TEXT("MID_A"))&&!Solved.Contains(TEXT("MID_B"))&&Cooldown==0&&!bCaptured);
    Grace=0;OnPuzzleSolved(TEXT("MID_B"));OpenExit();Capture();Tick(1.1f);RestoreCheckpoint();
    Check(TEXT("14.final_capture_restores_open_exit_without_repeating_puzzles"),Stage==ELBStage::Escape&&bExit&&AllObjectivesComplete()&&Grace>0&&Enemy->State==ELBEnemyState::FinalChase&&Enemy->Target.Equals(Player->GetActorLocation(),.1f));
    const FVector BeforeGrace=Enemy->GetActorLocation();Enemy->Tick(.25f);
    Check(TEXT("14b.final_retry_keeps_one_second_grace"),Enemy->GetActorLocation().Equals(BeforeGrace,.1f));
    Tick(1.01f);for(int I=0;I<5;++I)Enemy->Tick(.05f);
    Check(TEXT("14c.final_retry_resumes_player_chase_after_grace"),Enemy->State==ELBEnemyState::FinalChase&&FVector::Dist2D(BeforeGrace,Enemy->GetActorLocation())>1);
    Player->SetActorLocation(FVector(1700,1760,96));Tick(.1f);
    Check(TEXT("15.inside_entrance_is_not_escape"),Stage==ELBStage::Escape&&!bSucceeded);
    Player->SetActorLocation(FVector(1700,2140,96));Tick(.1f);Capture();
    Check(TEXT("16.escape_result_wins_over_late_capture"),bSucceeded&&!bCaptured&&Stage==ELBStage::Ending);
    Tick(Settings->EndingTime+.1f);Check(TEXT("16b.ending_reaches_completion"),Stage==ELBStage::Complete&&Menu==TEXT("Complete"));

    bool OriginalPrototype=Settings->bEnablePrototypePuzzles,OriginalReady=Settings->bPuzzleContentReady;
    TArray<FName> OriginalRequired=Settings->RequiredPuzzleIds;
    Settings->bEnablePrototypePuzzles=false;Settings->bPuzzleContentReady=false;Settings->RequiredPuzzleIds.Reset();
    Check(TEXT("17.empty_unconfigured_puzzle_list_never_auto_completes"),!AllObjectivesComplete());
    Settings->bEnablePrototypePuzzles=OriginalPrototype;Settings->bPuzzleContentReady=OriginalReady;Settings->RequiredPuzzleIds=OriginalRequired;
    ResetMid();Player->SetActorLocation(FVector(1700,2180,96));
    for(FName Zone:{FName(TEXT("Z01")),FName(TEXT("Z02")),FName(TEXT("Z03"))})
    {
        ALBDevice* Speaker=FindDevice(Zone,ELBDevice::Speaker);
        Enemy->ResetEnemy(FTransform(FRotator(0,-90,0),FVector(1830,1220,0)),ELBEnemyState::Investigate);
        bool bRoute=Speaker&&Enemy->FindRoute(Speaker->InvestigationPoint);
        Check(*(TEXT("18.route_exists.")+Zone.ToString()),bRoute);
        if(Speaker)
        {
            Enemy->Target=Speaker->InvestigationPoint;Enemy->State=ELBEnemyState::Investigate;
            for(int I=0;I<1800&&FVector::Dist2D(Enemy->GetActorLocation(),Speaker->InvestigationPoint)>60;++I)Enemy->Tick(.05f);
            Check(*(TEXT("19.enemy_physically_reaches_room.")+Zone.ToString()),FVector::Dist2D(Enemy->GetActorLocation(),Speaker->InvestigationPoint)<=60);
        }
    }
    // Actual wall and closed-door visibility checks in the classroom boundary.
    ResetMid();Player->SetActorLocation(FVector(300,-720,96));Enemy->ResetEnemy(FTransform(FRotator(0,-90,0),FVector(300,-430,0)),ELBEnemyState::Patrol);
    ALBDoor* SightDoor=nullptr;for(ALBDoor* Door:Doors)if(Door->DoorId==TEXT("D_C1_B"))SightDoor=Door;
    if(SightDoor)
    {
        SightDoor->SetOpen(false,true);
        GetWorld()->SendAllEndOfFrameUpdates();
        UE_LOG(LogTemp,Display,TEXT("LB_DOOR_PROBE world=%s rotation=%s scale=%s collision=%d visibility=%d"),*SightDoor->Leaf->GetComponentLocation().ToString(),*SightDoor->Leaf->GetComponentRotation().ToString(),*SightDoor->Leaf->GetComponentScale().ToString(),(int32)SightDoor->Leaf->GetCollisionEnabled(),(int32)SightDoor->Leaf->GetCollisionResponseToChannel(ECC_Visibility));
        Check(TEXT("20.closed_door_blocks_sight"),!Enemy->HasSight());
        SightDoor->SetOpen(true,true);Check(TEXT("20b.open_door_allows_sight"),Enemy->HasSight());
    }
    ResetMid();OnPuzzleSolved(TEXT("MID_A"));ULBCheckpoint* Saved=Checkpoint;Checkpoint=nullptr;ContinueGame();
    Check(TEXT("21.disk_checkpoint_round_trip"),Checkpoint&&Checkpoint!=Saved&&Solved.Contains(TEXT("MID_A"))&&bPower);
    UGameplayStatics::DeleteGameInSlot(SaveSlot,0);
    Report.RemoveFromEnd(TEXT(",\n"));Report+=FString::Printf(TEXT("\n  ],\n  \"passed\":%d,\n  \"failed\":%d\n}\n"),Passed,Failed);
    FString Dir=FPaths::ProjectSavedDir()/TEXT("LastBroadcast");IFileManager::Get().MakeDirectory(*Dir,true);
    FFileHelper::SaveStringToFile(Report,*(Dir/TEXT("functional_report.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("LB_TESTS_COMPLETE passed=%d failed=%d"),Passed,Failed);
    FPlatformMisc::RequestExitWithStatus(false,Failed?1:0);
#endif
}
