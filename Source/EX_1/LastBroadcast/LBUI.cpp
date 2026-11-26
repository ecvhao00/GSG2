#include "LBFramework.h"
#include "Camera/CameraComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Fonts/CompositeFont.h"
#include "UnrealClient.h"
#include "HAL/FileManager.h"
#include "GameFramework/PlayerInput.h"

ALBDirector* ALBController::Director() const { if (const auto* P = Cast<ALBPlayer>(GetPawn()))return P->Director;return nullptr; }
void ALBController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ALBController::EscapeKey).bExecuteWhenPaused = true;
    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ALBController::EnterKey).bExecuteWhenPaused = true;
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ALBController::SpaceKey).bExecuteWhenPaused = true;
#if !UE_BUILD_SHIPPING
    if (PlayerInput)PlayerInput->DebugExecBindings.RemoveAll([](const FKeyBind& Binding) {return Binding.Key == EKeys::F9;});
    InputComponent->BindKey(EKeys::F8, IE_Pressed, this, &ALBController::DebugKey).bExecuteWhenPaused = true;
    InputComponent->BindKey(EKeys::F9, IE_Pressed, this, &ALBController::EnemyDebugKey).bExecuteWhenPaused = true;
#endif
}
void ALBController::EscapeKey() { if (auto* D = Director())D->PauseMenu(); }
void ALBController::EnterKey()
{
    if (auto* D = Director())
    {
        if (D->Menu == TEXT("Title") || D->Menu == TEXT("Complete"))D->StartNewGame();
        else if (D->Menu == TEXT("Failure"))D->RestoreCheckpoint();
        else if (D->Menu == TEXT("Pause"))D->SetMenu(TEXT(""));
        else if (D->Menu == TEXT("Note"))D->CloseModal();
    }
}
void ALBController::SpaceKey() { if (auto* D = Director())D->SkipCinematic(); }
void ALBController::DebugKey()
{
#if !UE_BUILD_SHIPPING
    if (auto* D = Director())D->bDebug = !D->bDebug;
#endif
}
void ALBController::LBStatus()
{
    if (auto* D = Director())UE_LOG(LogTemp, Display, TEXT("LB_STATUS stage=%s power=%d exit=%d solved=%d cooldown=%.3f progress=%.3f enemy_enabled=%d enemy=%s"), *UEnum::GetValueAsString(D->Stage), D->bPower, D->bExit, D->Solved.Num(), D->Cooldown, D->UnlockProgress, D->IsDebugEnemyEnabled(), D->Enemy ? *UEnum::GetValueAsString(D->Enemy->State) : TEXT("none"));
}
void ALBController::EnemyDebugKey()
{
#if !UE_BUILD_SHIPPING
    if (auto* D = Director())if (D->bDebug)D->SetDebugEnemyEnabled(!D->IsDebugEnemyEnabled());
#endif
}
void ALBController::LBEnemy(int32 Enabled)
{
#if !UE_BUILD_SHIPPING
    if (auto* D = Director())
    {
        if (Enabled != 0 && Enabled != 1) { D->Notify(TEXT("사용법: LBEnemy 0 (끄기) / LBEnemy 1 (켜기)"));return; }
        D->SetDebugEnemyEnabled(Enabled == 1);
    }
#endif
}
void ALBController::LBTest()
{
#if !UE_BUILD_SHIPPING
    if (auto* D = Director())D->RunFunctionalTests();
#endif
}
void ALBController::LBSolve(FName PuzzleId)
{
#if !UE_BUILD_SHIPPING
    if (auto* D = Director())if (D->PrototypePuzzlesEnabled())D->OnPuzzleSolved(PuzzleId);
#endif
}
void ALBController::LBTeleport(FName LocationId)
{
#if !UE_BUILD_SHIPPING
    if (auto* D = Director())if (D->Player)
    {
        FVector P = D->IntroSpawn;FRotator R(0, -90, 0);
        if (LocationId == TEXT("power")) { P = FVector(-1730, -850, 96);R.Yaw = 180; }
        if (LocationId == TEXT("radio")) { P = FVector(2350, 960, 96);R.Yaw = -90; }
        if (LocationId == TEXT("archive")) { P = FVector(-1400, 360, 96);R.Yaw = 180; }
        if (LocationId == TEXT("class1")) { P = FVector(360, -1210, 96);R.Yaw = 0; }
        if (LocationId == TEXT("class2")) { P = FVector(1420, -1210, 96);R.Yaw = -90; }
        if (LocationId == TEXT("guide")) { P = FVector(900, -80, 96);R.Yaw = -90; }
        if (LocationId == TEXT("console")) { P = FVector(2630, 1140, 96);R.Yaw = 0; }
        if (LocationId == TEXT("exit")) { P = FVector(1700, 1900, 96);R.Yaw = 90; }
        D->Player->SetActorLocation(P);SetControlRotation(R);
    }
#endif
}
void ALBController::LBPhoto(const FString& Name)
{
#if !UE_BUILD_SHIPPING
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("LastBroadcast/Screenshots");
    IFileManager::Get().MakeDirectory(*Dir, true);
    FScreenshotRequest::RequestScreenshot(Dir / (FPaths::MakeValidFileName(Name) + TEXT(".png")), true, false);
#endif
}
void ALBController::LBWalk(float Seconds)
{
#if !UE_BUILD_SHIPPING
    if (auto* D = Director())if (D->Player)D->Player->DevWalkRemaining = FMath::Clamp(Seconds, 0.f, 10.f);
#endif
}

void ALBHUD::Rect(float X, float Y, float W, float H, FLinearColor C) { DrawRect(C, X * UI, Y * UI, W * UI, H * UI); }
void ALBHUD::Text(const FString& V, float X, float Y, float Size, FLinearColor C) { DrawText(V, C, X * UI, Y * UI, KoreanFont, Size / 48.f * UI, false); }
void ALBHUD::Wrapped(const FString& V, float X, float& Y, float Width, float Size, FLinearColor Color)
{
    FString Line;float Used = 0;
    for (TCHAR Ch : V)
    {
        float Advance = Ch < 128 ? Size * .55f : Size;
        if (Ch == '\n' || Used + Advance > Width) { Text(Line, X, Y, Size, Color);Y += Size * 1.65f;Line.Empty();Used = 0;if (Ch == '\n')continue; }
        Line.AppendChar(Ch);Used += Advance;
    }
    if (!Line.IsEmpty()) { Text(Line, X, Y, Size, Color);Y += Size * 1.65f; }
}
void ALBHUD::Button(FName Id, const FString& Label, float X, float Y, float W)
{
    float MX = 0, MY = 0;if (PlayerOwner)PlayerOwner->GetMousePosition(MX, MY);
    bool Hover = MX / UI >= X && MX / UI <= X + W && MY / UI >= Y && MY / UI <= Y + 48;
    Rect(X, Y, W, 48, Hover ? FLinearColor(.2f, .28f, .27f, .98f) : FLinearColor(.075f, .10f, .105f, .98f));
    Rect(X, Y, 3, 48, Hover ? FLinearColor(.65f, .85f, .72f) : FLinearColor(.25f, .4f, .35f));
    Text(Label, X + 18, Y + 11, 19, Hover ? FLinearColor::White : FLinearColor(.8f, .85f, .81f));
    AddHitBox(FVector2D(X * UI, Y * UI), FVector2D(W * UI, 48 * UI), Id, true, 1);
}
void ALBHUD::NotifyHitBoxClick(FName Id) { if (auto* C = Cast<ALBController>(PlayerOwner))if (auto* D = C->Director())D->MenuAction(Id); }
void ALBHUD::DrawHUD()
{
    Super::DrawHUD();if (!Canvas)return;
    const ALBPlayer* OwningPlayer = Cast<ALBPlayer>(GetOwningPawn());
    ALBDirector* D = OwningPlayer ? OwningPlayer->Director.Get() : nullptr;if (!D)return;
    if (!KoreanFont)
    {
        KoreanFont = NewObject<UFont>(this);KoreanFont->FontCacheType = EFontCacheType::Runtime;KoreanFont->LegacyFontSize = 48;
        KoreanFont->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Emplace(TEXT("Regular"), FPaths::ProjectContentDir() / TEXT("LastBroadcast/Fonts/DroidSansFallback.ttf"), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
    }
    UI = FMath::Max(.5f, Canvas->ClipY / 900.f);float W = Canvas->ClipX / UI, H = 900;
    const FLinearColor Muted(.55f, .65f, .62f), Cream(.86f, .9f, .83f), Green(.48f, .8f, .63f);
    for (float Y = 0;Y < H;Y += 4)Rect(0, Y, W, 1, FLinearColor(0, 0, 0, .045f));
    Rect(0, 0, W, 28, FLinearColor(0, 0, 0, .35f));Text(TEXT("구관 방송 시스템  /  AFTER HOURS"), 28, 5, 12, Muted);
    Text(TEXT("REC  •"), W - 105, 5, 12, FLinearColor(.7f, .2f, .15f));
    bool bPlay = D->Menu.IsEmpty() || D->Menu == TEXT("Puzzle");
    if (bPlay && D->Stage != ELBStage::Ending && D->Stage != ELBStage::Complete)
    {
        Rect(24, 53, FMath::Min(850.f, W - 48), 70, FLinearColor(.015f, .025f, .025f, .78f));
        Text(TEXT("현재 목표"), 40, 62, 12, Muted);float TY = 82;Wrapped(D->Objective(), 40, TY, FMath::Min(810.f, W - 82), 18, Cream);
        if (D->Player)
        {
            AActor* Focus = D->Player->FocusedActor();FString Prompt;
            if (ALBDoor* Door = Cast<ALBDoor>(Focus))Prompt = Door->bLocked ? TEXT("현관  [E] 확인") : TEXT("문  [E] 열기 / 닫기");
            if (ALBDevice* Device = Cast<ALBDevice>(Focus))
            {
                if (Device->Kind != ELBDevice::Speaker && Device->Kind != ELBDevice::EntranceTrigger && Device->Kind != ELBDevice::EscapeTrigger)
                    Prompt = Device->DisplayName + TEXT("  [E] ") + (Device->Kind == ELBDevice::ExitConsole ? TEXT("길게 누르기") : TEXT("상호작용"));
                if ((Device->Kind == ELBDevice::ZoneSelect || Device->Kind == ELBDevice::RadioStart || Device->Kind == ELBDevice::ExitConsole) && D->bPower)
                {
                    Rect(W - 375, 155, 350, 145, FLinearColor(.01f, .03f, .03f, .9f));
                    Text(TEXT("방송실 단말"), W - 355, 170, 18, Green);
                    Text(TEXT("선택  ") + (D->SelectedZone.IsNone() ? TEXT("없음") : D->SelectedZone.ToString()), W - 355, 206, 17);
                    Text(D->Cooldown > 0 ? FString::Printf(TEXT("재사용까지 %d초"), FMath::CeilToInt(D->Cooldown)) : TEXT("방송 준비 완료"), W - 355, 237, 17);
                    if (!D->PlayingZone.IsNone())Text(TEXT("재생 중  ") + D->PlayingZone.ToString(), W - 355, 270, 13, Green);
                }
            }
            if (!Prompt.IsEmpty() && D->Menu.IsEmpty())
            {
                Rect(W * .5f - 2, H * .5f - 2, 4, 4, Cream);
                Rect(W * .5f - 335, H - 142, 670, 48, FLinearColor(0, 0, 0, .8f));Text(Prompt, W * .5f - 310, H - 131, 18, Cream);
            }
            if (D->Player->Stamina < .999f)
            {
                Text(TEXT("지구력"), 30, H - 68, 13, Muted);Rect(30, H - 40, 210, 5, FLinearColor(.06f, .08f, .07f));Rect(30, H - 40, 210 * D->Player->Stamina, 5, Green);
            }
        }
        if (D->Stage == ELBStage::Unlocking)
        {
            float P = FMath::Clamp(D->UnlockProgress / D->Settings->UnlockTime, 0.f, 1.f);
            Text(FString::Printf(TEXT("현관 개방  %.1f초"), FMath::Max(0.f, D->Settings->UnlockTime - D->UnlockProgress)), W * .5f - 170, H - 225, 23);
            Rect(W * .5f - 200, H - 182, 400, 9, FLinearColor(.05f, .08f, .07f));Rect(W * .5f - 200, H - 182, 400 * P, 9, Green);
        }
        if (D->Stage == ELBStage::Intro || D->Stage == ELBStage::Ending)Text(TEXT("Space / Esc  건너뛰기"), W - 310, H - 50, 16, Muted);
    }
    if (D->Preferences->bSubtitles && !D->Subtitle.IsEmpty())
    {
        Rect(W * .5f - 470, H - 250, 940, 55, FLinearColor(0, 0, 0, .88f));Text(D->Subtitle, W * .5f - 446, H - 237, 21, Cream);
    }
    if (D->ToastRemaining > 0 && !D->Toast.IsEmpty())
    {
        Rect(26, H - 302, FMath::Min(950.f, W - 52), 57, FLinearColor(.01f, .04f, .035f, .9f));float Y = H - 290;Wrapped(D->Toast, 42, Y, FMath::Min(918.f, W - 84), 16, Cream);
    }
    if (D->bCaptured)Rect(0, 0, W, H, FLinearColor(0, 0, 0, D->Menu == TEXT("Failure") ? 1.f : D->CaptureFade()));
    if (D->Stage == ELBStage::Ending && (D->StageTime < 1 || D->StageTime>5))Rect(0, 0, W, H, FLinearColor(0, 0, 0, D->StageTime < 1 ? D->StageTime : FMath::Clamp((D->StageTime - 5) / 2, 0.f, 1.f)));
    if (!D->Menu.IsEmpty())
    {
        bool bModal = D->Menu == TEXT("Note") || D->Menu == TEXT("Puzzle");
        Rect(0, 0, W, H, FLinearColor(0, 0, 0, bModal ? .7f : .88f));
        float X = W * .5f - 330, Y = 180;
        if (bModal)
        {
            if (D->Menu == TEXT("Note"))
            {
                Rect(X - 25, Y - 35, 710, 490, FLinearColor(.03f, .045f, .043f, .98f));
                Text(D->ModalTitle, X, Y, 30, Cream);Y += 67;Wrapped(D->ModalText, X, Y, 650, 20, Cream);
                Button(TEXT("Close"), TEXT("닫기  [Esc]"), X, 594, 650);
            }
            else if (D->Menu == TEXT("Puzzle"))
            {
                Rect(X - 25, 110, 710, 635, FLinearColor(.03f, .045f, .043f, .98f));
                Text(D->ModalTitle, X, 132, 28, Cream);

                if (D->ActivePuzzle == TEXT("P01"))
                {
                    Text(TEXT("[정점 커버 알고리즘] 모든 송전선(6개)에 전력을 공급하세요."), X, 175, 16, Green);
                    Text(TEXT("조건: 과부하 방지를 위해 차단기는 최대 2개까지만 켤 수 있습니다."), X, 200, 15, Muted);

                    const int32 EdgeU[6] = { 0, 1, 2, 3, 1, 0 };
                    const int32 EdgeV[6] = { 1, 2, 3, 4, 4, 3 };
                    int32 Covered = 0, ActiveCnt = 0;
                    for (int32 n = 0; n < 5; ++n) if (GPuzzleRuntime.P01_Nodes[n]) ActiveCnt++;

                    for (int32 e = 0; e < 6; ++e)
                    {
                        bool bOn = GPuzzleRuntime.P01_Nodes[EdgeU[e]] || GPuzzleRuntime.P01_Nodes[EdgeV[e]];
                        if (bOn) Covered++;
                        float CX = X + (e % 3) * 220.f;
                        float CY = 240.f + (e / 3) * 36.f;
                        FString WireTxt = FString::Printf(TEXT("회로 [%d-%d] : %s"), EdgeU[e], EdgeV[e], bOn ? TEXT("통전(O)") : TEXT("단선(X)"));
                        Text(WireTxt, CX, CY, 16, bOn ? Green : FLinearColor(.9f, .35f, .3f));
                    }

                    FString StatusLine = FString::Printf(TEXT("통전 회로: %d / 6개   |   사용 차단기: %d / 2개 (2-근사해: 4개)"), Covered, ActiveCnt);
                    Text(StatusLine, X, 325, 17, (Covered == 6 && ActiveCnt > 2) ? FLinearColor(1.f, .45f, .2f) : Cream);
                    if (Covered == 6 && ActiveCnt > 2)
                        Text(TEXT("전력 과부하! 스위치를 2개 이하로 줄여야 복구됩니다."), X, 353, 15, FLinearColor(1.f, .45f, .2f));

                    for (int32 n = 0; n < 5; ++n)
                    {
                        FName BtnId(*FString::Printf(TEXT("P01_Node%d"), n));
                        FString Label = FString::Printf(TEXT("차단기 #%d  [%s]"), n, GPuzzleRuntime.P01_Nodes[n] ? TEXT("ON") : TEXT("OFF"));
                        float BX = X + (n % 2) * 330.f;
                        float BY = 390.f + (n / 2) * 58.f;
                        Button(BtnId, Label, BX, BY, 315.f);
                    }
                }
                else if (D->ActivePuzzle == TEXT("MID_A"))
                {
                    Text(TEXT("[0/1 배낭 문제 DP] 가방 용량(10kg) 내 최대 가치(60점)를 맞추세요."), X, 175, 16, Green);
                    Text(TEXT("자료실 기밀 문서를 클릭해 가방에 넣거나 뺄 수 있습니다."), X, 200, 15, Muted);

                    const TCHAR* Names[5] = { TEXT("교장실 비상 키카드"), TEXT("방송국 주파수 일지"), TEXT("구관 배선 설계도"), TEXT("야간 순찰 일지"), TEXT("방송부 출석부") };
                    const int32 Weights[5] = { 6, 5, 4, 3, 2 };
                    const int32 Values[5] = { 35, 28, 25, 18, 10 };
                    int32 SumW = 0, SumV = 0;

                    for (int32 i = 0; i < 5; ++i)
                    {
                        if (GPuzzleRuntime.MIDA_Selected[i]) { SumW += Weights[i]; SumV += Values[i]; }
                        FName BtnId(*FString::Printf(TEXT("MIDA_Item%d"), i));
                        FString Label = FString::Printf(TEXT("%s %s  (무게: %dkg | 가치: %d점)"),
                            GPuzzleRuntime.MIDA_Selected[i] ? TEXT("[담음]") : TEXT("[제외]"), Names[i], Weights[i], Values[i]);
                        Button(BtnId, Label, X, 240.f + i * 56.f, 650.f);
                    }

                    FString BagStatus = FString::Printf(TEXT("현재 적재 무게: %d / 10 kg   |   현재 가치: %d / %d 점 (DP 최적해)"), SumW, SumV, GPuzzleRuntime.MIDA_MaxValue);
                    Text(BagStatus, X, 540.f, 18, SumW > 10 ? FLinearColor(1.f, .35f, .3f) : Cream);
                    if (SumW > 10) Text(TEXT("가방 무게 한도(10kg)를 초과했습니다!"), X, 570.f, 15, FLinearColor(1.f, .35f, .3f));
                }
                else if (D->ActivePuzzle == TEXT("MID_B"))
                {
                    Text(TEXT("[백트래킹 해 탐색] 시작(S)에서 목표(E)까지 안전 회로를 연결하세요."), X, 175, 16, Green);
                    Text(TEXT("인접한 칸을 클릭해 전진하고, 막다른 길에서는 [되돌아가기]를 누르세요."), X, 200, 15, Muted);

                    FIntPoint Head = GPuzzleRuntime.MIDB_PathStack.Num() > 0 ? GPuzzleRuntime.MIDB_PathStack.Last() : FIntPoint(0, 0);
                    for (int32 r = 0; r < 4; ++r)
                    {
                        for (int32 c = 0; c < 4; ++c)
                        {
                            FName CellId(*FString::Printf(TEXT("MIDB_Cell_%d_%d"), r, c));
                            int32 Type = GPuzzleRuntime.MIDB_Grid[r][c];
                            FString CellLabel = TEXT("·");
                            if (r == Head.X && c == Head.Y) CellLabel = TEXT("현재(P)");
                            else if (r == 0 && c == 0) CellLabel = TEXT("시작(S)");
                            else if (Type == 3) CellLabel = TEXT("목표(E)");
                            else if (Type == 1) CellLabel = TEXT("벽(#)");
                            else if (Type == 2) CellLabel = TEXT("합선(X)");
                            else if (GPuzzleRuntime.MIDB_Visited[r][c]) CellLabel = TEXT("연결(*)");

                            Button(CellId, CellLabel, X + c * 165.f, 240.f + r * 58.f, 152.f);
                        }
                    }

                    Button(TEXT("MIDB_Backtrack"), TEXT("한 칸 되돌아가기 (Backtrack)"), X, 495.f, 315.f);
                    Button(TEXT("MIDB_Reset"), TEXT("경로 처음부터 초기화"), X + 335.f, 495.f, 315.f);
                    Text(FString::Printf(TEXT("현재 탐색 깊이(스택 크기): %d칸"), GPuzzleRuntime.MIDB_PathStack.Num()), X, 562.f, 16, Cream);
                }
                else
                {
                    Y = 190.f;
                    Wrapped(D->ModalText.IsEmpty() ? TEXT("미정 퍼즐 슬롯입니다.") : D->ModalText, X, Y, 650, 20, Cream);
                    if (D->PrototypePuzzlesEnabled()) Button(TEXT("Solve"), TEXT("개발용 완료  [Enter]"), X, 530, 650);
                }

                Button(TEXT("Close"), TEXT("닫기  [Esc]"), X, 665, 650);
            }
        }
        else if (D->Menu == TEXT("Settings"))
        {
            Text(TEXT("환경 설정"), X, Y, 42, Cream);Y += 85;
            const FString Labels[] = { FString::Printf(TEXT("음량     %d%%"),FMath::RoundToInt(D->Preferences->Volume * 100)),FString::Printf(TEXT("밝기     %.1f"),D->Preferences->Brightness),FString::Printf(TEXT("시야각   %.0f"),D->Preferences->FOV),FString::Printf(TEXT("감도     %.2f"),D->Preferences->Sensitivity) };
            const FString Ids[] = { TEXT("Volume"),TEXT("Brightness"),TEXT("FOV"),TEXT("Sensitivity") };
            for (int I = 0;I < 4;++I) { Text(Labels[I], X, Y + 10, 22);Button(FName(Ids[I] + TEXT("Minus")), TEXT("−"), X + 390, Y, 85);Button(FName(Ids[I] + TEXT("Plus")), TEXT("+"), X + 490, Y, 85);Y += 65; }
            Button(TEXT("Subtitles"), D->Preferences->bSubtitles ? TEXT("방송 자막 켜짐") : TEXT("방송 자막 꺼짐"), X, Y, 575);Button(TEXT("Back"), TEXT("돌아가기  [Esc]"), X, Y + 68, 575);
        }
        else
        {
            FString Title = D->Menu == TEXT("Title") ? TEXT("마지막 방송") : D->Menu == TEXT("Pause") ? TEXT("잠시 멈춤") : D->Menu == TEXT("Failure") ? TEXT("방송이 끊겼다") : TEXT("퇴실 완료");
            Text(Title, X, Y, 54, Cream);Y += 90;
            Text(D->Menu == TEXT("Title") ? TEXT("아무도 남아 있지 않아야 할 학교。") : D->Menu == TEXT("Failure") ? TEXT("마지막 체크포인트에서 다시 시작한다。") : D->Menu == TEXT("Complete") ? TEXT("그런데, 방송실에서는 아직 발소리가 들린다。") : TEXT("월드와 방송 재사용 시간이 함께 멈춰 있다。"), X, Y, 18, Muted);Y += 68;
            if (D->Menu == TEXT("Title")) { Button(TEXT("New"), TEXT("새 게임  [Enter]"), X, Y, 600);Button(TEXT("Continue"), TEXT("계속하기"), X, Y + 64, 600);Button(TEXT("Settings"), TEXT("환경 설정"), X, Y + 128, 600);Button(TEXT("Quit"), TEXT("종료"), X, Y + 192, 600); }
            if (D->Menu == TEXT("Pause")) { Button(TEXT("Resume"), TEXT("계속하기  [Esc]"), X, Y, 600);Button(TEXT("Settings"), TEXT("환경 설정"), X, Y + 64, 600);Button(TEXT("Title"), TEXT("타이틀로"), X, Y + 128, 600); }
            if (D->Menu == TEXT("Failure")) { Button(TEXT("Retry"), TEXT("체크포인트 재시작  [Enter]"), X, Y, 600);Button(TEXT("Title"), TEXT("타이틀로"), X, Y + 64, 600); }
            if (D->Menu == TEXT("Complete")) { Button(TEXT("New"), TEXT("새 게임  [Enter]"), X, Y, 600);Button(TEXT("Title"), TEXT("타이틀로"), X, Y + 64, 600); }
        }
    }
    if (!D->Menu.IsEmpty() && D->ToastRemaining > 0 && !D->Toast.IsEmpty())
    {
        Rect(26, H - 100, FMath::Min(950.f, W - 52), 65, FLinearColor(.01f, .04f, .035f, .98f));
        float Y = H - 88;Wrapped(D->Toast, 42, Y, FMath::Min(918.f, W - 84), 16, Cream);
    }
#if !UE_BUILD_SHIPPING
    if (D->bDebug)
    {
        Rect(W - 510, H - 200, 495, 180, FLinearColor(0, 0, 0, .9f));
        Text(UEnum::GetValueAsString(D->Stage), W - 495, H - 187, 14, Green);
        Text(FString::Printf(TEXT("PWR %d  EXIT %d  SOLVED %d  CD %.2f"), D->bPower, D->bExit, D->Solved.Num(), D->Cooldown), W - 495, H - 159, 14);
        Text(D->IsDebugEnemyEnabled() ? TEXT("적 ON  [F9 끄기]") : TEXT("적 OFF  [F9 켜기]"), W - 495, H - 131, 18, D->IsDebugEnemyEnabled() ? Green : FLinearColor(1, .45f, .2f));
        Text(D->Enemy ? UEnum::GetValueAsString(D->Enemy->State) : TEXT("No enemy"), W - 495, H - 99, 14);
        Text(TEXT("F8 숨기기 · 콘솔 LBEnemy 0 / LBEnemy 1"), W - 495, H - 67, 12, Muted);
        Text(TEXT("적 재활성화 시 1초 유예 · 체크포인트 재시작에도 설정 유지"), W - 495, H - 44, 12, Muted);
    }
    else if (!D->IsDebugEnemyEnabled())
    {
        Rect(W - 300, 24, 275, 42, FLinearColor(0, 0, 0, .9f));
        Text(TEXT("DEBUG · 적 OFF  [F8 설정]"), W - 285, 34, 16, FLinearColor(1, .45f, .2f));
    }
#endif
}