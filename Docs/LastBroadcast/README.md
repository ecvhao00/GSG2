# 마지막 방송 프로토타입

EX_1의 기존 C++ 1인칭 템플릿을 상속해 만든 학교 탈출 호러 프로토타입이다. 기존 샘플 레벨과 애셋은 보존했다.

## 실행

- 프로젝트 루트의 `Play_LastBroadcast.cmd`를 실행하면 게임 창이 열린다.
- 에디터에서는 `EX_1.uproject`를 열고 기본 맵 `Content/LastBroadcast/Maps/L_LastBroadcast`에서 Play를 누른다.
- WASD 이동, 마우스 시선, Shift 달리기, E 상호작용, F 손전등, Esc 메뉴, Space 컷신 건너뛰기.
- 현관 개방 콘솔은 E를 **6초 동안 누른 채 같은 장치를 바라봐야** 한다. 입력을 놓거나 시선·거리를 벗어나면 처음부터 다시 시작한다.
- 달리기는 최대 4초다. 1.5초 후 회복을 시작하고 약 6초에 전량 회복한다. 소진 후에는 Shift를 놓았다가 다시 눌러 달린다.

## 플레이 순서

1. 남쪽 현관으로 들어가면 문이 닫히고 잠긴다. 조명이 꺼지고 교내 방송이 나온다.
2. 교실 1에서 전력 복구 안내를 찾고, 배전실 P01 장치를 완료한다.
3. 복도에 순찰자가 나타난다. 교무실에서 방송 사용법을 확인한다.
4. 방송실에서 교실 1·교실 2·자료실 중 한 곳을 선택하고 별도 시작 버튼으로 방송한다. 모든 구역은 20초 쿨타임을 공유한다.
5. 방송으로 적을 유인하거나 복도와 실내 관통 동선을 이용해 자료실 MID_A, 교실 1 MID_B의 중간 작업을 완료한다. 순서는 자유다.
6. 방송실 비상 개방 콘솔을 6초간 조작하면 현관이 열리고 경보가 울린다. 적은 그 순간 위치에서 강제 최종 추격을 시작해 현재 플레이어만 계속 따라간다. 남쪽 현관 바깥의 탈출 영역까지 이동한다.
7. 엔딩에서 빈 방송실과 직전 탈출 중의 발소리를 재생한다.

## 아직 미정인 퍼즐

P01·MID_A·MID_B는 **정답을 임의로 만든 퍼즐이 아니다.** 현재 개발 빌드에서는 각 장치의 ‘개발용 완료’ 버튼으로 실제 진행 연결을 확인한다. 중간 장치 화면에서도 적과 게임 시간은 계속 흐른다. 배포용 Shipping 빌드에서는 이 임시 완료 기능을 허용하지 않는다.

실제 퍼즐은 `BP_LBDevice`를 상속해 입력·정답·오답을 구현하고, 성공 시 `SubmitSolved()` 또는 진행 관리자의 `OnPuzzleSolved(PuzzleId)`를 호출하면 된다. `Prerequisites`, 전력 요구 여부, 완료 복원, 필수 ID 목록은 이미 연결되어 있다. 실 퍼즐 구성이 끝나면 `bPuzzleContentReady`를 활성화한다. 미구성 빈 목록을 자동 완료로 처리하지 않는다.

## 편집할 곳

| 대상 | 위치와 방법 |
|---|---|
| 맵과 가구 | `Content/LastBroadcast/Maps/L_LastBroadcast`, Outliner의 LastBroadcast 폴더 |
| 시간·속도·감지 거리·필수 퍼즐 | `Content/LastBroadcast/DA_PrototypeTuning` |
| 확장용 블루프린트 | `Content/LastBroadcast/Blueprints/BP_LB*` |
| 대사·효과음 | `Content/LastBroadcast/Audio` |
| 진행 상태·저장 | `Source/EX_1/LastBroadcast/LBDirector.cpp` |
| 캐릭터·문·장치·적 경로 | `Source/EX_1/LastBroadcast/LBActors.cpp` |
| HUD·메뉴·개발 명령 | `Source/EX_1/LastBroadcast/LBUI.cpp` |
| 실제 맵 회귀 검증 | `Source/EX_1/LastBroadcast/LBTests.cpp` |

핵심 규칙은 C++에 두고 장치와 조정값을 블루프린트·데이터 애셋에서 확장할 수 있게 했다. 기존 1인칭 캐릭터 클래스 `AEX_1Character`를 상속한다. Python은 에디터에서 맵·애셋을 생성하는 제작 도구로만 사용하며 게임 실행에 필요하지 않다. [Epic Python 에디터 스크립팅 문서](https://dev.epicgames.com/documentation/unreal-engine/scripting-the-unreal-editor-using-python).

## 구현 규칙

- 진행은 Outside → Intro → Explore → PowerReveal → Objectives → Ready → Unlocking → Escape → Ending → Complete로 관리한다. 중복 입장·퍼즐 성공·개방·탈출 통보는 진행을 두 번 바꾸지 않는다.
- 첫 컷신과 엔딩은 1초 이후 건너뛰어도 동일한 종료 상태를 확정한다. 첫 컷신은 로컬 한국어 합성 대사의 길이에 맞춰 **13초**로 조정했다.
- 방송 대상 선택과 실제 재생을 구분한다. 재생 중 선택을 바꿔도 방송 위치는 바뀌지 않으며 실패 요청은 쿨타임을 소모하지 않는다.
- 음향은 실제 구역 스피커에서 나고 적은 같은 방의 도달 가능한 조사 지점으로 간다. 도착 후 8초 조사한다.
- 현관 개방 전에는 순찰·조사·일반 추격·수색을 구분한다. 직접 시야가 소리보다 우선하고, 시야를 2초 잃으면 마지막 확인 위치를 수색한다. 현관 개방 후에는 FinalChase가 이 규칙보다 우선한다.
- 적은 정적 벽·가구를 검사한 60cm 격자의 A* 경로와 몸체 폭 검사로 이동한다. 일반 문은 접근 시 열고 통과하며, 실제 잠긴 문은 경로에서 제외한다. 순간이동으로 경로 실패를 감추지 않는다.
- 일반 구간에서 같은 시각의 방송은 달리기 소음보다 우선하고 나중의 달리기는 목표를 바꿀 수 있다. S60에서는 현재 플레이어 위치만 계속 갱신하며 모든 소리 자극을 무시한다. 시야·거리와 관계없이 추격을 유지하고, 조사·수색·순찰로 복귀하지 않는다. 발견 대기 없이 기본 추격 속도 320cm/s로 이동하며 경로는 약 0.55초마다 갱신한다.
- 안내문과 일시정지는 월드·적·음향·쿨타임을 함께 멈춘다. 메뉴를 열면 개방 진행은 취소되며 새 E 입력이 필요하다.
- C0 첫 컷신 이후, C1 전력 연출 이후, C2 각 중간 목표 완료, C3 현관 개방 완료에 저장한다. 재시작은 지정 위치·완료 목록·전력·문·조명을 복원하고 지구력과 방송 상태를 초기화한다.
- C3 재시작은 현관이 열린 최종 탈출 상태로 돌아간다. 적을 지정 위치의 FinalChase로 복원하고 1초간 이동·포획을 유예한 뒤 현재 플레이어 추격을 재개한다. 퍼즐과 개방 조작을 반복하지 않는다. 일반 개방 순간에는 적의 위치를 유지한다.
- 포획과 탈출이 겹치면 먼저 확정된 결과가 유지된다. 포획 시 약 1초 암전 후 재시작 메뉴를 표시한다.
- 밝기·음량·시야각·감도·방송 자막 설정을 제공한다. 설정과 체크포인트는 별도 슬롯이다.

## 빌드와 검증

에디터/게임 창을 닫은 뒤 PowerShell에서 실행한다.

```powershell
# 코드만 빌드. 수동 편집한 맵을 유지한다.
& '.\Scripts\LastBroadcast\build_and_generate.ps1' -SkipScene

# 코드를 빌드하고 생성 맵을 다시 배치한다.
& '.\Scripts\LastBroadcast\build_and_generate.ps1'

# 실제 맵에서 진행·시야·경로·저장 동작을 회귀 검증한다.
& '.\Scripts\LastBroadcast\run_checks.ps1'
```

맵 재생성은 Outliner의 `LastBroadcast/` 아래 생성 액터를 교체한다. 이전 맵은 `Backup/LastBroadcast_AutoRevisions`에 보관한다. 기존 프로젝트 코드·설정·프로젝트 파일의 작업 전 백업은 `Backup/LastBroadcast_before_20261106`에 있다.

검증은 전용 `LastBroadcast_Automation_Isolated` 저장 슬롯을 사용하고 종료 시 정리한다. 사용자 플레이 저장은 건드리지 않는다. 결과는 `Saved/LastBroadcast/functional_report.json`, 상세 로그는 `Saved/Logs/LastBroadcast_Functional.log`에 기록한다. 화면을 그리지 않는 자동 검증과 실제 렌더링/조작 점검을 구분한다.

개발 빌드의 콘솔 명령은 `LBStatus`, `LBSolve P01`, `LBTeleport power|guide|radio|archive|class1|class2|console|exit`, `LBWalk 2`, `LBPhoto 이름`이다. F8로 디버그 표시를 켠다. 진행용 우회 명령은 Shipping에서 동작하지 않는다.

## 제작 자료

학교 가구와 건축물은 편집 가능한 단순 메시, 재질은 직접 만든 저해상도 텍스처다. AI 생성 이미지는 사용하지 않았다. 한국어 안내 방송은 설치된 Windows Heami로 로컬 합성했고, 차임·발소리·경보·문·잡음은 결정적으로 합성한 임시 음향이다. 폰트는 엔진에 포함된 DroidSansFallback을 사용한다. 최종 연기·캐릭터 모델·개별 퍼즐 미술은 추후 교체할 수 있다.

기존 `VisualStudioTools` 플러그인은 설치 파일이 없어서 프로젝트에서 비활성화했다. 게임 실행에는 필요하지 않다. 기존 샘플 레벨은 삭제하지 않았으며, 시작 맵과 기본 게임 모드만 LastBroadcast로 변경했다.
