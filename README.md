# state_machine_litever 동작 정리

대상 파일은 `src/state_machine_litever.cpp`이다.

## 1. 상태 목록

| 상태 | 역할 |
|---|---|
| `CheckMap` | 지도 및 관련 노드 시작 |
| `CheckData` | 초기 처리 후 `Idle` 진입 |
| `Idle` | 작업 선택 대기 |
| `Work` | 순찰·작업 실행 |
| `Move_point` | 지정 포인트 즉시 이동 |
| `Stop` | 작업 정지 및 복구 선택 |
| `Home` | 홈 포인트 이동 |
| `Docking` | 충전소 진입 및 충전 시작 |
| `Charging` | 충전 중 대기 및 작업 요청 |
| `Undocking` | 충전 종료 및 도크 이탈 |
| `Manual` | 외부 조작기를 이용한 수동 제어 |
| `Debug` | 포인트·작업·도크 설정 |
| `EmergencyStop` | 비상 입력 해제 전까지 정지 |

## 2. 부팅 동작

부팅 시 도크 위에서 재시작하더라도 도크·충전 상태를 확인해 자동으로
도킹 시퀀스를 시작하지 않는다. 항상 다음 순서로 시작한다.

```text
CheckMap → CheckData → Idle
```

startup task가 설정되어 있어도 부팅 직후 실행하지 않는다. startup task는
`Idle` 또는 `Charging`에서 키 5를 입력했을 때 실행한다.

## 3. 충전·도킹 상태

`state_machine_litever`는 `/robot_udp/basic_status`의 `BasicStatus.charge`를
구독한다. 도크 장착 여부는 `/dock/is_docked`를 사용하며, 이 토픽은
`robot_udp_connect`의 `docking_manager`가 UDP 상태를 종합해 발행한다.

### BasicStatus.charge 값

| 값 | 의미 | 상태 머신 처리 |
|---:|---|---|
| 0 | 충전하지 않음 | 도크 이탈 완료 판단 |
| 1 | 충전소 진입 중 | `Docking` 유지 |
| 2 | 충전 중 | `Charging` 전이 |
| 3 | 충전소 이탈 중 | `Undocking` 유지 |
| 4 | 로봇 오류 | 도킹 동작 실패 가능 |
| 5 | 도크 위지만 미충전·보호 충전 종료 | 도크 위에서 `Charging` 유지, 재시도하지 않음 |

`Charge` 명령값은 상태값과 다르다.

| Charge 명령 | 의미 |
|---:|---|
| 0 | 충전 중단 |
| 1 | 충전 시작 |
| 2 | 충전 상태 초기화 |

### Home 복귀 후 충전

`Home` 포인트 이동이 성공하면 기존 동작대로 자동으로 `Docking`으로 전이한다.

```text
Home
  ↓ tp_status=3 && action goal=SUCCEEDED
Docking
  ↓ Charge(1)
  ↓ charge=1: 충전소 진입 중
  ↓ charge=2: 충전 중
Charging
```

부팅 시에는 이 흐름을 수행하지 않고 `Idle`로 시작한다.

### Charging에서 작업 요청

충전 중 이동이 필요한 작업은 먼저 `Undocking`을 거친다. 단순히 `Idle`로
돌아가는 키 2는 도크를 유지한 채 바로 `Idle`로 전이한다.

| 입력 | 흐름 |
|---|---|
| 키 1 | `Charging → Undocking → Work` |
| 키 2 | `Charging → Idle` |
| 키 3 + 포인트명 | `Charging → Undocking → Move_point` |
| 키 5 | `Charging → Undocking → Work` (startup task) |

### Undocking

```text
Undocking
  ↓ Charge(0)
  ↓ charge=2: 충전 중단 요청 전 상태
  ↓ charge=3: 도크 이탈 중
  ↓ charge=0 && /dock/is_docked=false
Work / Move_point
```

`Docking`과 `Undocking`에서는 다음 키를 무시한다.

- 키 16: Manual
- 키 20: Initial Pose
- 키 21: Stand
- 키 22: Sitting

## 4. Idle 기능

| 키 | 동작 |
|---:|---|
| 1 + 포인트명 | 지정 포인트 작업 실행 후 `Work` |
| 2 | 포인트 목록 표시 |
| 3 + task명 | 선택 task 실행 후 `Work` |
| 4 | 도킹 중이면 도크 이탈, 아니면 `Home` |
| 5 | `startup.task` 실행 |
| 7 | `Debug` 진입 |
| 16 | `Manual` 진입 |
| 20 | 초기 자세 발행 |
| 21 | Stand 요청 |
| 22 | Sitting 요청 |

Idle에는 별도의 키 6 `Move_point` 진입 기능을 사용하지 않는다.

## 5. Work와 run_task

```text
Idle + 키 3 + task명 → Work → run_task 실행
Idle + 키 5           → Work → startup.task 실행
```

| 조건 | 동작 |
|---|---|
| 키 1 | 작업 정지 후 `Stop` |
| 키 2 | 작업 정지 후 `Home` |
| 키 3 + 포인트명 | 작업 정지 후 `Move_point` |
| 단일 포인트 작업 완료 | `Idle` |
| 일반 task 완료 | `Home` |
| 장애물·안전 감지 | `Stop` |

순찰 task가 `Home`으로 복귀하고 이동이 성공하면 `Docking`을 거쳐
`Charging`으로 들어간다.

## 6. Move_point

`Move_point`는 진입 명령에 포인트명을 함께 전달한다. 진입 후 키 1을 다시
입력하지 않고 즉시 포인트 이동을 시작한다.

```text
Work / Stop / Home / Idle
        ↓ 포인트명 포함 명령
Move_point → 포인트 이동 실행
```

| 조건 | 다음 상태 |
|---|---|
| 이동 성공 | `Idle` |
| 이동 실패 | `Stop` |
| 키 2 | `Stop` |
| 키 3 | `Home` |
| 키 4 이상 | `Idle` |

충전 중 포인트 이동은 `Charging → Undocking → Move_point` 순서로 처리한다.

## 7. Stop

| 키 | 동작 |
|---:|---|
| 1 | 이전 작업 또는 이동 재개 |
| 2 | `Idle` |
| 3 | `Home` |
| 4 + 포인트명 | `Move_point` |
| 16 | `Manual` |

장애물 정지인 경우 조건이 해제되고 관련 피드백이 정상이라면 이전 작업 위치에서
자동 재개될 수 있다.

## 8. Manual

`Work`에서 직접 Manual로 진입하지 않는다. 작업 중 수동 조작이 필요하면
먼저 `Stop`으로 이동한 뒤 키 16을 입력한다.

```text
Idle / Stop / Home / Move_point / Charging
        ↓ 키 16
      Manual
```

`Docking`과 `Undocking`에서는 Manual 진입이 차단된다.

Manual 진입 후 `SetMode`를 요청하고 `/robot_udp/basic_status`의
`control_usage_mode`를 확인한다.

| control_usage_mode | 의미 | 외부 발행 상태명 |
|---:|---|---|
| 0 | Regular | `Manual_Regular` |
| 1 | Navigation | Regular로 정규화 |
| 2 | Assist | `Manual_Assist` |

| 키 | 동작 |
|---:|---|
| 1 | Manual 종료 후 `Idle` |
| 2 | Manual 종료 후 `Stop` |
| 3 | Assist mode 전환, Manual 유지 |
| 4 | Regular mode 전환, Manual 유지 |
| 20 | `startup.dock` 기준 `initialpose` 발행 |
| 21 | Stand 요청 |
| 22 | Sitting 요청 |

수동 속도 입력은 `cmd_vel_manual`로 받고, Manual 진입 및 제어 모드 확인이
완료된 경우에만 `/cmd_vel`로 전달한다. Manual 자체에서는 주기적으로 0 속도를
발행하지 않는다.

## 9. Initial pose 및 모션 제어

```text
키 20 → startup.dock 기준 initialpose 발행
키 21 → SetMotionState.state=1 → Stand
키 22 → SetMotionState.state=4 → Sitting
```

이 기능은 `Charging`과 `Manual`에서 사용할 수 있고, `Docking`과 `Undocking`에서는
차단된다.

## 10. EmergencyStop

상태 머신은 `is_emergency_button_pressed`(`std_msgs/Bool`)를 구독한다.
입력값의 실제 출처가 물리 버튼인지 NATS인지 상태 머신은 구분하지 않는다.

```text
일반 운용 상태 + Bool=true → EmergencyStop → Bool=false → Stop
```

EmergencyStop에서는 작업을 취소하고 경고등을 점멸한다. `Manual`에서는 기존
설계대로 비상 입력만으로 자동 EmergencyStop 전이를 수행하지 않는다.

## 11. 주요 ROS 인터페이스

### 구독

| 토픽 | 용도 |
|---|---|
| `state_machine_control` | 키 및 작업 제어 입력 |
| `cmd_vel_manual` | Manual 수동 속도 입력 |
| `/robot_udp/basic_status` | 모션·충전·제어 모드 상태 |
| `/dock/is_docked` | 도크 장착 여부 |
| `/safety/status` | 안전 상태 |
| `/safety_obstacle_status` | 장애물 상태 |
| `/battery/return_home` | 배터리 복귀 요청 |

### 발행

| 토픽 | 용도 |
|---|---|
| `state_machine_status` | 현재·이전 상태 |
| `/cmd_vel` | Manual 속도 명령 |
| `initialpose` | 초기 자세 |
| `move_base/cancel` | 이동 취소 |
| `multipoint_server/stop` | task 정지·일시정지 |
| `sound/play_once` | 음성 출력 |

### 서비스·액션

| 인터페이스 | 용도 |
|---|---|
| `SetMode` | Regular/Assist 제어 모드 변경 |
| `SetMotionState` | Stand/Sitting 변경 |
| `set_gait` | 보행 모드 변경 |
| `dock_action` | 도킹·도크 이탈 및 충전 명령 |
| `multi_point_server` | task 실행 |
| `teach_point_server` | 포인트 실행·저장·삭제 |

## 12. 주요 기능 연결 구조

```mermaid
flowchart TD
    CM[CheckMap] --> CD[CheckData]
    CD -->|항상| IDLE[Idle]

    IDLE -->|키 1 + 포인트명| WORK[Work]
    IDLE -->|키 3 + task명| WORK
    IDLE -->|키 5 startup.task| WORK
    IDLE -->|키 4 + 도크 아님| HOME[Home]
    IDLE -->|키 4 + 도크 장착: 직접 이탈 후 Idle| IDLE
    IDLE -->|키 7| DEBUG[Debug]
    IDLE -.->|키 16| MAN[Manual]

    WORK -->|run_task| RUN[순찰 실행]
    WORK -->|키 1 / 장애물 / 안전| STOP[Stop]
    WORK -->|키 2| HOME
    WORK -->|키 3 + 포인트명| MP[Move_point]
    RUN -->|단일 포인트 완료| IDLE
    RUN -->|일반 task 완료| HOME

    HOME -->|키 1| STOP
    HOME -->|키 2 + 포인트명| MP
    HOME -->|이동 성공| DK[Docking]
    DK -->|Charge(1), charge=1| DKWAIT[충전소 진입 중]
    DKWAIT -->|charge=2| CH[Charging]

    CH -->|키 1| UD[Undocking]
    CH -->|키 2| IDLE
    CH -->|키 3 + 포인트명| UD
    CH -->|키 5| UD
    CH -.->|키 16/20/21/22 허용| MAN

    UD -->|Charge(0): charge=2 → 3| UWAIT[도크 이탈 중]
    UWAIT -->|Work 대상| WORK
    UWAIT -->|Move_point 대상| MP

    MP -->|진입 즉시| MPRUN[포인트 이동 실행]
    MPRUN -->|성공| IDLE
    MPRUN -->|실패| STOP
    MP -->|키 2| STOP
    MP -->|키 3| HOME
    MP -.->|키 16| MAN

    STOP -->|키 1| WORK
    STOP -->|키 2| IDLE
    STOP -->|키 3| HOME
    STOP -->|키 4 + 포인트명| MP
    STOP -.->|키 16| MAN

    MAN -->|키 1| IDLE
    MAN -->|키 2| STOP
    MAN -->|키 3/4| MODE[Manual_Assist / Manual_Regular]

    DK -.->|Manual/pose/motion 차단| BLOCK[입력 무시]
    UD -.->|Manual/pose/motion 차단| BLOCK

    ESTOP[Emergency button Bool=true] --> E[EmergencyStop]
    WORK -.-> E
    IDLE -.-> E
    HOME -.-> E
    MP -.-> E
    STOP -.-> E
    DK -.-> E
    CH -.-> E
    E -->|Bool=false| STOP

    classDef normal fill:#e8f1ff,stroke:#2563eb,color:#111;
    classDef charge fill:#e8fff0,stroke:#16a34a,color:#111;
    classDef control fill:#fff4d6,stroke:#d97706,color:#111;
    classDef safety fill:#ffe4e6,stroke:#e11d48,color:#111;
    class IDLE,WORK,RUN,HOME,MP,MPRUN,CM,CD,DEBUG normal;
    class DK,DKWAIT,CH,UD,UWAIT charge;
    class MAN,MODE control;
    class STOP,E,BLOCK safety;
```
