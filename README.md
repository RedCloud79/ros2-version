# state_machine_litever.cpp 기능 정리

## 1. 상태 목록

| 상태 | 역할 |
|---|---|
| `CheckMap` | 지도 및 설정 확인 |
| `CheckData` | 초기 처리 후 `Idle` 진입 |
| `Idle` | 작업 선택 대기 |
| `Work` | 순찰·작업 실행 |
| `Move_point` | 지정 포인트 이동 |
| `Stop` | 작업 정지 및 복구 선택 |
| `Home` | 홈 포인트 이동 |
| `Docking` | 충전 도크 진입 및 충전 시작 |
| `Charging` | 충전 중 대기 및 작업 요청 |
| `Undocking` | 충전 종료 및 도크 이탈 |
| `Manual` | 외부 조작기의 수동 속도 제어 |
| `Debug` | 포인트·작업·홈·영역 설정 |
| `EmergencyStop` | 비상 버튼 해제 전까지 정지 |

## 2. 시작 및 도킹 판단

부팅 시에는 도크 위에서 전원을 껐다 켜더라도 도킹 여부와 관계없이 `Idle`로 시작한다.

```text
CheckMap → CheckData → Idle
```

`startup.task`가 설정되어 있어도 부팅 직후 자동 실행하지 않는다. `Idle`에서 키 5를 입력했을 때 startup task를 실행한다.

## 3. 순찰(run_task) 흐름

### 3.1 처음 켰을 때 Idle인 경우

```text
CheckMap → CheckData → Idle
                         ↓ run_task 입력(키 3) + task명
                       Work
                         ↓
                       순찰 실행
```

`Idle`에서 키 3을 입력하면 task YAML을 확인하고 `Work`로 진입한다.
task가 없으면 `Idle`에 유지된다.

### 3.2 처음 켰을 때

부팅 시에는 항상 `Idle`로 시작한다. 도크 위에서 재시작하더라도 자동으로
`Docking`에 들어가지 않는다.

```text
CheckMap → CheckData → Idle
```

startup task는 `Idle`에서 키 5를 사용한다.

```text
Idle → 키 5(startup.task) → Work
```

순찰 중 `Home` 포인트에 도착하면 기존 동작대로 충전을 시작한다.

```text
Work → Home → Docking → Charging
```

충전 중 작업을 시작할 때는 먼저 도크에서 이탈한다.

```text
Charging → Undocking → Work / Move_point
```

### 3.3 순찰 중 Manual로 전환하는 경우

현재 구현에서는 `Work`에서 키 16을 직접 입력해 Manual로 갈 수 없다.
먼저 작업을 정지한 다음 Manual로 전환한다.

```text
Work
  ↓ 키 1 또는 장애물·안전 정지
Stop
  ↓ 키 16
Manual_Regular 또는 Manual_Assist
```

Manual 종료:

```text
Manual
  ├─ 키 1 → Idle
  └─ 키 2 → Stop
```

### 3.4 순찰 중 Move_point로 전환하는 경우

다른 상태에서 `Move_point`로 전환할 때는 포인트명을 전환 명령과 함께
전달한다. `Move_point` 진입 후 키 1을 다시 입력하지 않는다.

```text
Work
  ↓ 키 3 + 포인트명
Move_point
  ↓ 진입 즉시
포인트 이동 실행
  ├─ 성공 → Idle
  └─ 실패 → Stop
```

## 4. 주요 상태별 기능

### Idle

| 키 | 동작 |
|---:|---|
| 1 + 포인트명 | 단일 포인트 작업 실행 후 `Work` |
| 2 | 포인트 목록 표시 |
| 3 + task명 | 선택한 task 실행 후 `Work` |
| 4 | 도킹 중이면 도크 이탈, 아니면 `Home` |
| 5 | `startup.task` 실행 |
| 7 | `Debug` 진입 |
| 16 | `Manual` 진입 |
| 20 | 초기 자세 발행 |

`Idle`의 키 6으로 `Move_point`에 직접 진입하는 기능은 사용하지 않는다.

### Move_point

| 진입 상태 | 입력 | 동작 |
|---|---|---|
| `Work` | 키 3 + 포인트명 | 작업 취소 후 즉시 포인트 이동 |
| `Stop` | 키 4 + 포인트명 | 정지 후 즉시 포인트 이동 |
| `Home` | 키 2 + 포인트명 | 홈 이동 취소 후 즉시 포인트 이동 |
| `Charging` | 키 3 + 포인트명 | 충전 중 이탈 후 포인트 이동 |

`Move_point::Enter()`에서 포인트 이동 서버를 호출하며 `buffer=1`로 도착 후
추가 작업과 대기 시간을 건너뛴다.

| 조건 | 다음 상태 |
|---|---|
| 이동 성공 | `Idle` |
| 이동 실패 | `Stop` |
| 키 2 | `Stop` |
| 키 3 | `Home` |
| 키 4 이상 | `Idle` |
| 키 16 | `Manual` |

### Work / run_task

`Work`는 `multi_point_server`로 순찰 task를 실행한다. task YAML의 `loop`,
`wait` 설정에 따라 완료 후 `Home` 또는 다음 `Work`로 이어질 수 있다.
`Home` 포인트 이동 성공 후에는 `Docking`을 거쳐 `Charging`으로 진입한다.

| 키/조건 | 동작 |
|---|---|
| 키 1 | 작업 취소 후 `Stop` |
| 키 2 | 작업 취소 후 `Home` |
| 키 3 + 포인트명 | 작업 취소 후 `Move_point` 즉시 실행 |
| 단일 포인트 작업 완료 | `Idle` |
| 일반 task 완료 | 설정에 따라 `Home` 또는 다음 작업 |
| 장애물·안전 감지 | `Stop` |

### Stop

`Stop`은 정지 후 사용자가 복구 방향을 선택하는 상태다.

| 키 | 동작 |
|---:|---|
| 1 | 이전 작업 또는 이동 재개 |
| 2 | `Idle` |
| 3 | `Home` |
| 4 + 포인트명 | `Move_point` 즉시 실행 |
| 16 | `Manual` |

장애물 정지인 경우 장애물이 해제되면 이전 작업 위치에서 자동 재개될 수
있다. 경로 실패나 수동 정지는 사용자의 입력을 기다린다.

### Manual

Manual은 순찰·자율주행 작업을 잠시 중단하고 외부 조작기로 로봇을 직접
움직이는 상태다. 내부 FSM 상태명은 `Manual`이지만, 외부에 발행하는 상태명은
BasicStatus의 제어 모드에 따라 구분한다.

### 진입 조건

키 16으로 진입할 수 있는 상태는 `Idle`, `Stop`, `Home`, `Move_point`,
`Charging`이다. `Docking`과 `Undocking`에서는 Manual 진입을 차단한다. `Work`에서는 작업 중 직접 Manual로 진입하지 못한다.
순찰 중 수동 조작이 필요하면 먼저 `Stop`으로 이동한 뒤 키 16을 입력한다.

```text
Idle / Stop / Home / Move_point / Charging
                    ↓ 키 16
                  Manual
```

`manual_control_enabled` 토픽이나 별도의 Enable 플래그는 사용하지 않는다.
Manual 진입 명령인 키 16이 직접 진입 조건이다.

### 진입 직후 제어 모드 확인

Manual에 들어오면 이전에 확인된 모드를 먼저 확인한다.

1. 이전 BasicStatus가 Assist(2) 또는 Regular(0)이면 해당 모드를 유지한다.
2. 모드가 없거나 Navigation(1) 등 알 수 없는 값이면 Regular(0)을 요청한다.
3. `SetMode` 서비스를 호출한다.
4. `/robot_udp/basic_status`가 요청한 값을 다시 보내는지 확인한다.
5. 확인이 완료된 뒤에만 수동 속도 명령을 허용한다.

```text
키 16
  ↓
Manual 내부 진입
  ↓
현재 모드 확인
  ├─ 0 → Regular 유지 요청
  ├─ 2 → Assist 유지 요청
  └─ 그 외 → Regular 요청
  ↓
SetMode
  ↓
BasicStatus.control_usage_mode 확인
  ↓
Manual_Regular 또는 Manual_Assist 발행
```

제어 모드 값과 외부 상태명:

| `control_usage_mode` | 의미 | 상태 발행명 |
|---:|---|---|
| 0 | Regular mode | `Manual_Regular` |
| 1 | Navigation mode | Manual 진입 시 Regular로 정규화 |
| 2 | Assist mode | `Manual_Assist` |

BasicStatus 확인 전에는 `Manual` 상태를 발행하지 않는다. 모드 확인 후
`Manual_Regular` 또는 `Manual_Assist` 상태를 발행한다.

### Manual 내부 키 동작

| 키 | 동작 |
|---:|---|
| 1 | 모드 확인 후 Manual 종료, `Idle` 전이 |
| 2 | Regular 모드 요청 후 Manual 종료, `Stop` 전이 |
| 3 | Assist 모드 요청, Manual 유지 |
| 4 | Regular 모드 요청, Manual 유지 |
| 20 | `startup.dock` 기준으로 `initialpose` 발행 |
| 21 | Stand 모션 상태 요청 (`SetMotionState.state=1`) |
| 22 | Sitting 모션 상태 요청 (`SetMotionState.state=4`) |

키 3·4는 상태를 나가는 명령이 아니라 제어 모드만 바꾸는 명령이다.
모드 변경 후 BasicStatus가 확인되면 상태 발행명이 각각
`Manual_Assist`·`Manual_Regular`로 바뀐다.

### Manual에서 자세·모션 제어

Manual에서는 이동 속도뿐 아니라 로봇의 초기 자세와 기본 모션 상태도
제어할 수 있다.

#### 초기 자세 설정: 키 20

키 20을 입력하면 `run_data.yaml`의 `startup.dock` 좌표와 방향을 사용해
`geometry_msgs/PoseWithCovarianceStamped` 메시지를 `initialpose`로 발행한다.
즉, 현재 로봇의 위치를 임의로 읽는 기능이 아니라 설정 파일에 저장된 도크
자세를 지도 기준 초기 위치로 전달하는 기능이다.

```text
Manual + 키 20
       ↓
run_data.yaml / startup / dock
       ↓
map frame의 x, y, z, w 추출
       ↓
initialpose 발행
```

초기 자세 발행은 `Move_point`, `Work`, `Home`에서는 차단되며, Manual에서는
허용된다. 따라서 수동 조작 전에 로봇 위치를 도크 기준으로 재설정할 수 있다.

#### Stand / Sitting: 키 21·22

키 21과 키 22는 `SetMotionState` 서비스를 호출한다.

```text
키 21 → SetMotionState.state = 1 → Stand
키 22 → SetMotionState.state = 4 → Sitting
```

서비스 호출 성공 여부를 확인해 로그로 남기며, 상태 머신 자체가 자세 완료를
기다리는 구조는 아니다. 실제 Stand/Sitting 동작 완료는 BasicStatus 등 로봇
상태 피드백으로 확인해야 한다.

### Manual 종료 처리

키 1 또는 키 2가 들어오면 새 속도 입력을 받지 않도록 Manual 요청을
비활성화하고 Regular(0) 모드를 요청한다.

```text
Manual
  ├─ 키 1 → Regular 확인 → Idle
  └─ 키 2 → Regular 확인 → Stop
```

이미 Regular 모드가 확인된 경우에는 중복 요청을 줄이고 바로 전이할 수 있다.
Manual을 나갈 때 내부 수동 플래그와 입력 데이터는 초기화한다.

### 안전 관련 예외

Manual에서는 비상 버튼과 안전 감지에 의한 자동 `EmergencyStop`·`Stop`
전이를 수행하지 않는다. 즉 Manual 진입 후에는 해당 두 안전 조건만으로
자동 상태 전이를 하지 않는다. 다만 배터리 강제 복귀는 별도 전역 로직이므로
조건에 따라 Manual 종료 후 `Home`으로 이동할 수 있다.

## 5. 도킹·홈 복귀

이동 명령 전 `UndockOrDocking()`이 `/dock/is_docked`를 확인한다.

```text
도킹되지 않음 → 이동 명령 실행
도킹됨       → 도크 이탈 시도
                ├─ 성공 → 이동 명령 실행
                └─ 실패 → 현재 상태 유지
```

홈 도착 후에는 `Docking`으로 전이하여 충전소로 진입하고 충전을 시작한다.
충전 중이거나 배터리가 100%가 되어 보호를 위해 충전 전류가 차단된 경우에도
`Charging`에서 대기한다. 작업이나 포인트 이동 요청이 들어오면
`Undocking`으로 전이하여 도크에서 나온 뒤 작업을 수행한다.

## 6. EmergencyStop 동작

Emergency button 입력은 `is_emergency_button_pressed`(`std_msgs/Bool`)로
수신한다. 다음 상태에서는 버튼이 눌려도 전역 자동 전이를 수행하지 않는다.

현재 운용에서는 NATS의 stop 명령이 이 입력을 `true`로 만드는 경로로 사용될
수 있다. 상태 머신 입장에서는 물리 버튼인지 NATS 명령인지 구분하지 않고
Bool 값만 처리한다.

| 현재 상태 | Emergency button 처리 |
|---|---|
| `CheckMap` | 자동 전이하지 않음 |
| `CheckData` | 자동 전이하지 않음 |
| `Manual` | 자동 `EmergencyStop` 전이하지 않음 |
| `EmergencyStop` | 이미 해당 상태이므로 유지 |
| 그 외 운용 상태 | `EmergencyStop`으로 전이 |

운용 상태에서 `true`가 들어오면 현재 작업을 취소하고 경고등을 점멸시킨다.
`false`가 들어오기 전까지 `EmergencyStop`에 머물며, `false`가 들어오면
`Stop`으로 전이한다.

```text
운용 상태
  ↓ Bool = true (Emergency button 또는 NATS stop)
EmergencyStop
  ↓ Bool = false
Stop
```

주의: 현재 구현에서 `Manual`은 Emergency button 자동 전이 대상에서 제외되어
있다. Manual 중 비상 버튼 동작까지 보장해야 한다면 이 조건을 별도로 변경해야 한다.

## 7. 주요 기능 연결 구조

순찰 시작, 도킹 복귀, 포인트 이동, 정지 복구 및 Manual 전환의 전체 연결
관계는 다음과 같다.

```mermaid
flowchart LR
    subgraph START[시작]
        CM[CheckMap] --> CD[CheckData]
        CD -->|항상| IDLE[Idle]
    end

    subgraph PATROL[순찰]
        IDLE -->|키 3 task명| WORK[Work]
        IDLE -->|키 5 startup.task| WORK
        WORK -->|run_task 실행| RUN[순찰 task]
        RUN -->|단일 포인트 완료| IDLE
        RUN -->|일반 task 완료| HOME[Home]
        HOME -->|도착| DK
        DK -->|충전 진입 완료| CH[Charging]
        CH -->|키 1 작업| UD[Undocking]
        CH -->|키 2| IDLE
        CH -->|키 5 startup.task| UD
        UD -->|이탈 완료 작업| WORK
    end

    subgraph POINT[포인트 이동]
        WORK -->|키 3 포인트명| MP[Move_point]
        STOP[Stop] -->|키 4 포인트명| MP
        HOME -->|키 2 포인트명| MP
        CH -->|키 3 포인트명| UD
        MP -->|진입 즉시| MP_RUN[포인트 이동 실행]
        MP_RUN -->|성공| IDLE
        MP_RUN -->|실패| STOP
        MP -->|키 2| STOP
        MP -->|키 3| HOME
        MP -->|키 4 이상| IDLE
    end

    subgraph RECOVERY[정지·복구]
        WORK -->|키 1 장애물 안전| STOP
        STOP -->|키 1 재개| WORK
        STOP -->|키 2| IDLE
        STOP -->|키 3| HOME
    end

    subgraph SAFETY[비상 정지]
        NATS_STOP[NATS stop 명령]
        ESTOP[EmergencyStop]
        NATS_STOP -->|Bool true| ESTOP
        WORK -.->|Emergency button| ESTOP
        IDLE -.->|Emergency button| ESTOP
        MP -.->|Emergency button| ESTOP
        STOP -.->|Emergency button| ESTOP
        HOME -.->|Emergency button| ESTOP
        DK -.->|Emergency button| ESTOP
        CH -.->|Emergency button| ESTOP
        ESTOP -->|Bool=false| STOP
    end

    subgraph MANUAL[수동 조작]
        MAN[Manual]
        MODE[Manual_Regular<br/>또는 Manual_Assist]
        MAN -->|키 3/4| MODE
        MAN -->|키 1| IDLE
        MAN -->|키 2| STOP
    end

    IDLE -.->|키 16| MAN
    STOP -.->|키 16| MAN
    HOME -.->|키 16| MAN
    MP -.->|키 16| MAN
        CH -.->|키 16| MAN

    classDef normal fill:#e8f1ff,stroke:#2563eb,color:#111;
    classDef control fill:#fff4d6,stroke:#d97706,color:#111;
    classDef safety fill:#ffe4e6,stroke:#e11d48,color:#111;
    class IDLE,WORK,RUN,HOME,MP,MP_RUN,CM,CD normal;
    class DK,CH,UD charge;
    classDef charge fill:#e8fff0,stroke:#16a34a,color:#111;
    class MAN,MODE control;
    class STOP,ESTOP safety;
```
