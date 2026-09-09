# state_machine_litever.cpp 기능 정리

대상 파일: [`src/state_machine_litever.cpp`](src/state_machine_litever.cpp)

`state_machine_litever.cpp`는 로봇의 작업, 수동 조작, 정지, 복귀, 도킹 및
비상 정지 상태를 관리하는 ROS 상태 머신이다. 외부에서는
`state_machine_control` 토픽으로 키 입력을 전달하고, 현재 상태는
`state_machine_status` 토픽으로 발행한다.

## 1. 상태 목록

| 내부 상태명 | 기능 |
|---|---|
| `CheckMap` | 지도 파일과 지도 관련 설정 확인 |
| `CheckData` | 시작 작업 및 도킹 상태 확인 |
| `Idle` | 대기 상태 |
| `Manual` | 외부 수동 조작 입력을 받아 로봇을 제어 |
| `Debug` | 포인트, 작업, 홈 위치, 영역 등을 저장·삭제 |
| `Work` | 작업 실행 및 작업 중 이동 |
| `Move_point` | 저장된 포인트 선택 및 이동 |
| `Stop` | 작업·이동을 정지하고 재개/복귀 여부 선택 |
| `Home` | 홈 위치로 이동 |
| `Docking` | 충전 도크 진입 또는 도크 이탈 후 작업 재개 |
| `EmergencyStop` | 비상 버튼 해제 전까지 정지 유지 |

## 2. 시작 흐름

```text
CheckMap
   ↓
CheckData
   ├─ startup.task가 있고 도크에 있음 → Docking
   └─ 그 외                              → Idle
```

도크 여부는 충전 상태가 아니라 `/dock/is_docked` 토픽을 기준으로 판단한다.
도크 상태가 2초 이상 갱신되지 않으면 도크에 있지 않은 것으로 처리한다.

## 3. 외부 입력

입력 토픽:

```text
state_machine_control : market_state_machine/StateMachine
```

주요 필드는 다음과 같다.

```text
int32 key
string name
string[] points
int32 pan
int32 tilt
int32 zoom
int32 rtime
int32 buffer
```

상태 머신은 주로 `key`를 사용하며, 포인트·작업 실행 시 나머지 필드를 함께
사용한다.

## 4. 상태별 주요 키

### `Idle`

| 키 | 동작 |
|---:|---|
| 1 | 지정 포인트로 이동하고 `Work` 진입 |
| 2 | 현재 상태 유지 |
| 3 | 작업 실행 후 `Work` 진입 |
| 4 | 도크에 있으면 이탈, 아니면 `Home` 이동 |
| 5 | 시작 작업 실행. 도크 상태에 따라 `Docking` 또는 `Work` |
| 7 | `Debug` 진입 |
| 8 | `Idle` 재진입 |
| 16 | `Manual` 진입 |
| 20 | 시작 설정의 초기 자세 발행 |

`Idle`의 키 6(`Move_point` 직접 진입)은 현재 사용하지 않는다. 포인트 이동은
키 1을 통해 처리하며, `Move_point` 상태 자체는 `Work`, `Stop`, `Home`,
`Docking`에서 계속 사용된다.

### `Manual`

| 키 | 동작 |
|---:|---|
| 1 | 수동 조작 종료 후 `Idle` |
| 2 | 수동 조작 종료 후 `Stop` |
| 3 | Assist 모드 요청 (`control_usage_mode = 2`) |
| 4 | Regular 모드 요청 (`control_usage_mode = 0`) |
| 20 | 초기 자세 발행 |
| 21 | Stand 모션 상태 요청 |
| 22 | Sitting 모션 상태 요청 |

수동 조작 입력은 다음 토픽으로 받는다.

```text
cmd_vel_manual : geometry_msgs/Twist
```

수동 모드가 활성화되고 BasicStatus로 모드 전환이 확인된 경우에만 입력을
`/cmd_vel`로 전달한다. `manual_control_enabled` 토픽은 사용하지 않는다.

## 5. Manual 진입 및 모드 확인

전체 흐름은 다음과 같다.

```text
key 16
  ↓
Manual 진입
  ↓
현재 BasicStatus 모드 확인
  ├─ 2 → Assist 유지
  ├─ 0 → Regular 유지
  └─ 그 외/미확인 → Regular(0) 요청
  ↓
SetMode 서비스 요청
  ↓
BasicStatus.control_usage_mode 확인
  ↓
상태 발행
```

`BasicStatus.control_usage_mode`의 의미는 다음과 같다.

| 값 | 의미 |
|---:|---|
| 0 | Regular mode |
| 1 | Navigation mode |
| 2 | Assist mode |

상태 토픽에는 내부 상태명 `Manual`을 직접 발행하지 않는다.
BasicStatus 확인 결과에 따라 다음 중 하나를 발행한다.

```text
Manual_Regular
Manual_Assist
```

BasicStatus가 아직 없거나 값이 0·2가 아니면 상태 발행을 잠시 보류한다.

## 6. Manual 진입 제한 및 안전 동작

- `Work`에서는 키 16을 무시한다. 작업 중에는 직접 `Manual`로 진입할 수 없다.
- `Manual`에서는 키 16을 다시 처리하지 않는다.
- 비상 버튼이 눌린 상태에서 `Manual`로 자동 전이하지 않는다.
- 안전 상태가 활성화되어도 `Manual`에서 자동으로 `Stop`으로 전이하지 않는다.
- Manual 종료는 키 1 또는 키 2로 명시적으로 수행한다.
- 배터리 강제 복귀 조건은 별도 전역 보호 로직이므로 `Home` 전이를 요청할 수
  있다.

## 7. 도킹 처리

`UndockOrDocking()`은 이동 명령을 실행하기 전에 도크 상태를 확인한다.

```text
도크에 있지 않음 → 바로 다음 동작 수행
도크에 있음     → getOutDock()으로 도크 이탈 시도
                 성공 시 다음 동작 수행
                 실패 시 현재 상태 유지
```

도킹 진입은 `getInDock()` 액션 서버를 사용하고, 도크 이탈은
`getOutDock()` 액션 서버를 사용한다.

## 8. 정지와 재개

`Stop`은 수동 정지, 장애물 정지, 안전 정지, 비상 정지 등의 원인을 구분한다.

| 키 | 동작 |
|---:|---|
| 1 | 이전 작업 또는 이동 재개 |
| 2 | `Idle` |
| 3 | `Home` |
| 4 | `Move_point` |
| 5 | `Stop` 상태 유지 |
| 16 | `Manual` |

작업 중 장애물 또는 안전 상태가 감지되면 `Stop`으로 전이하고, 정지 원인과
이전 상태에 따라 작업 재개 위치를 결정한다.

## 9. 상태 발행

발행 토픽:

```text
state_machine_status : market_state_machine/StateMachineStatus
```

메시지 필드:

```text
string prevState
string curState
```

일반 상태는 내부 상태명과 동일하게 발행한다. 단, 내부 `Manual` 상태는
BasicStatus 확인 후 `Manual_Assist` 또는 `Manual_Regular`로 변환해서 발행한다.

## 10. 주요 기능 연결 구조

상태 머신의 실제 운용 흐름은 `Idle`에서 작업을 선택하고, 작업 중에는
`Work`·`Move_point`·`Stop`·`Home`으로 분기하는 구조다.

```text
                         ┌──────────────┐
                         │    Manual    │
                         │ Assist/Regular│
                         └──────┬───────┘
                                │ 1: Idle / 2: Stop
                                │
┌────────┐  작업 선택       ┌───▼────┐  작업 중  ┌────────────┐
│  Idle  ├──────────────────►│ Work  ├─────────►│Move_point  │
└──┬─────┘                   └──┬────┘          └─────┬──────┘
   │                            │ 1: Stop              │ 완료
   │ 4: Home                    │ 2: Home              ▼
   │ 16: Manual                 ▼                    Idle
   │                       ┌────────┐
   └──────────────────────►│  Stop  │◄── 장애물/안전/수동 정지
                            └─┬──┬───┘
                         1: 재개 │ 3: Home
                         2: Idle  │ 4: Move_point
                                 ▼
                              Home
                                 │ 도착
                                 ▼
                              Docking
```

## 11. Move_point 기능

`Move_point`는 저장된 포인트를 선택해서 이동하는 상태다. 현재 `Idle`의
키 6으로 직접 들어가지 않고, 주로 `Work`, `Stop`, `Home`, `Docking`에서
진입한다.

| 진입 경로 | 의미 |
|---|---|
| `Work` 키 3 | 작업 중 포인트 이동으로 전환 |
| `Stop` 키 4 | 정지 상태에서 포인트 이동 선택 |
| `Home` 키 2 | 홈 이동 중 포인트 이동 선택 |
| `Docking` 키 3 | 도크 상태에서 포인트 이동 선택 |

| 키/조건 | 동작 |
|---|---|
| 1 | 선택한 포인트로 이동 시작 |
| 2 | 이동 취소 후 `Stop` |
| 3 | 이동 취소 후 `Home` |
| 4 이상 | 이동 취소 후 `Idle` |
| 이동 성공 | `Idle` |
| 이동 실패 | `Stop` |
| 16 | `Manual` 진입 요청. 단, 실제 전이는 전역 조건에서 처리 |

포인트 이동을 시작할 때는 포인트 이동 서버를 호출하며, 도착 후 추가 작업과
대기 시간을 건너뛰도록 `buffer=1`을 사용한다.

## 12. Run task 기능

Run task는 `Idle`에서 작업 이름을 선택하고 YAML에 정의된 작업 설정을
적용한 뒤 `Work`로 진입하는 흐름이다.

```text
Idle 키 3
  ↓
선택한 task를 YAML에서 조회
  ├─ task 없음 → Idle 유지
  └─ task 존재
       ↓
     도크 상태 확인 및 필요 시 도크 이탈
       ↓
     task 설정(loop/wait 등) 적용
       ↓
     Work 진입
```

Run task와 관련된 주요 입력은 다음과 같다.

| 위치 | 키 | 동작 |
|---|---:|---|
| `Idle` | 3 | 선택한 일반 task 실행 |
| `Idle` | 5 | `startup.task` 실행 |
| `Work` | 1 | 작업 정지 후 `Stop` |
| `Work` | 2 | 작업 취소 후 `Home` |
| `Work` | 3 | 현재 작업을 멈추고 `Move_point` |
| `Work` | 4 | 작업 반복/다음 작업/복귀 흐름 처리 |

`Idle` 키 1의 포인트 이동은 일반 task와 구분된다. 키 1은 선택한 포인트를
단일 이동 작업으로 `Work`에 전달하고, `point_task=true`로 설정한다.

## 13. Stop 기능

`Stop`은 단순 대기 상태가 아니라 정지 원인과 직전 상태를 이용해 다음 동작을
결정하는 복구 상태다.

```text
Work/Home/Move_point
          │ 장애물·안전·수동 정지
          ▼
        Stop
       ├─ 1: 이전 작업/이동 재개
       ├─ 2: Idle
       ├─ 3: Home
       ├─ 4: Move_point
       └─ 16: Manual
```

정지 원인별 처리:

| 정지 원인 | 처리 |
|---|---|
| 수동 정지 | 사용자가 재개·복귀·대기 중 하나를 선택 |
| 장애물 정지 | 장애물이 사라지면 이전 작업 위치부터 재개 가능 |
| 안전 정지 | 안전 상태가 해제된 뒤 정지 상태에서 재개 |
| 비상 정지 | `EmergencyStop`을 거친 경우 정지 원인을 Emergency로 유지 |
| 경로 실패 | 자동 재개하지 않고 `Stop`에서 사용자 판단 |

## 14. Manual 기능

`Manual`은 외부 조작기에서 속도 명령을 전달하는 상태다. 작업 실행 중인
`Work`에서는 수동 진입을 막고, `Idle`, `Stop`, `Home`, `Move_point`,
`Docking` 등에서 키 16으로 진입할 수 있다.

```text
키 16
  ↓
Manual 내부 진입
  ↓
SetMode 요청
  ↓
/robot_udp/basic_status 확인
  ├─ control_usage_mode=0 → Manual_Regular
  └─ control_usage_mode=2 → Manual_Assist
```

수동 속도 명령의 연결은 다음과 같다.

```text
외부 컨트롤러
      │
      ▼
cmd_vel_manual (geometry_msgs/Twist)
      │ Manual 활성 및 모드 확인
      ▼
cmd_vel_manual 콜백
      │
      ▼
/cmd_vel
      │
      ▼
로봇 속도 제어 계층
```

Manual 내부 키 동작:

| 키 | 동작 |
|---:|---|
| 1 | Manual 종료 후 `Idle` |
| 2 | Manual 종료 후 `Stop` |
| 3 | Assist 모드 요청 |
| 4 | Regular 모드 요청 |

Manual 상태에서는 비상 버튼과 안전 감지에 의한 자동 `EmergencyStop`·`Stop`
전이를 수행하지 않는다. 다만 배터리 강제 복귀 로직은 별도 전역 로직이므로
조건에 따라 Manual 종료 후 `Home`으로 보낼 수 있다.

## 15. 기능별 진입 가능 여부

| 현재 상태 | Move_point | Run task | Stop | Manual |
|---|---:|---:|---:|---:|
| `Idle` | 직접 진입 안 함 | 키 3·5 | - | 키 16 |
| `Work` | 키 3 | 작업 실행 중 | 키 1/장애물/안전 | 진입 불가 |
| `Move_point` | 포인트 선택 | - | 키 2/이동 실패 | 키 16 |
| `Stop` | 키 4 | 재개 키 1 | 유지 | 키 16 |
| `Home` | 키 2 | - | 키 1/실패 | 키 16 |
| `Docking` | 키 3 | 작업 재개 | 도킹 흐름에 따라 처리 | 키 16 |
| `Manual` | - | - | 키 2 | 이미 Manual |

핵심 구조는 `Idle`에서 작업을 시작하고, 작업 중 문제가 생기면 `Stop`에서
복구 방향을 선택하며, 수동 조작은 `Manual`에서만 외부 속도 입력을 허용하는
것이다.
