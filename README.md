# AbyssCrawler-2026GraduationProject

2026 한국공학대 게임공학과 졸업작품(AbyssCrawler)



\# 🌊 Abyss Crawler (어비스 크롤러)



!\[Unreal Engine 5](https://img.shields.io/badge/Unreal%20Engine-5.0+-black?style=for-the-badge\&logo=unrealengine)

!\[C++](https://img.shields.io/badge/Language-C++-blue?style=for-the-badge\&logo=cplusplus)

!\[Multiplayer](https://img.shields.io/badge/Type-Online%20Co--op-green?style=for-the-badge)

!\[Status](https://img.shields.io/badge/Status-In%20Development-orange?style=for-the-badge)



> "심해의 끝없는 어둠 속에서, 당신의 숨소리만이 유일한 생존의 증거입니다."



Abyss Crawler는 2XXX년, 대홍수로 인해 수몰된 지구를 배경으로 하는 4인 온라인 협동 생존 호러 게임입니다. 플레이어는 심해 탐사 기업의 사원이 되어, 산소와 배터리라는 극한의 제약 속에서 침몰한 구조물을 탐사하고 유물을 회수하여 살아남아야 합니다.



---



\## 📖 Table of Contents

1\. \[Project Overview](#-project-overview)

2\. \[Key Features](#-key-features)

3\. \[Technical Deep Dive (Client)](#-technical-deep-dive-client)

4\. \[Characters \& Creatures](#-characters--creatures)

5\. \[Installation \& Requirements](#-installation--requirements)

6\. \[Team](#-team)



---



\## 🎬 Project Overview



\* 장르 1인칭 생존 호러 / 4인 온라인 협동 (PvE)

\* 플랫폼: PC (Windows)

\* 개발 엔진: Unreal Engine 5

\* 개발 언어: C++ (Visual Studio 2022)

\* 개발 기간: 2025.01 ~ 2025.08 (졸업작품)



\### 🗺️ Story

2XXX년, 대홍수로 인해 지상의 대부분이 바다에 잠겼습니다. 인류는 인공 섬에서 살아가며 과거의 영광을 되찾기 위해 심해를 탐사합니다. 당신은 바다 속 폐허에서 값비싼 유물과 단서를 회수하는 임무를 맡았습니다. 하지만 심해에는 당신뿐만 아니라, 설명할 수 없는 공포가 도사리고 있습니다.



---



\## 🎮 Key Features



\### 1. 3D Underwater Exploration (6DOF)

\* 단순한 평면 이동이 아닌, 상하좌우 전후 모든 방향으로 자유롭게 이동하는 수중 6자유도(6DOF) 플레이를 제공합니다.

\* 수직적인 구조의 침몰 건물을 탐험하며 입체적인 공포를 경험할 수 있습니다.



\### 2. Strategic Resource Management

\* Oxygen (산소): 생존을 위한 절대적인 리미트.

\* Battery (배터리): 손전등, 스캐너, 방어 장비 등 모든 도구 사용의 동력원.

\* 팀원과 자원을 공유하고 배분하는 전략적 협동이 필수적입니다.



\### 3. High-Stakes Co-op Gameplay

\* Death Penalty: 팀원의 죽음은 탐사 실패 확률을 급격히 높입니다. 죽은 팀원의 시신을 수습하거나, 남은 자원을 챙겨 탈출해야 합니다.



---



\## 💻 Technical Deep Dive (Client)



> 본 프로젝트는 Unreal Engine 5와 Modern C++을 활용하여 최상의 퍼포먼스와 최적화를 목표로 개발되었습니다.



\### ⚙️ Core Architecture: Gameplay Ability System (GAS)

\* Modular Design: 체력, 산소, 배터리 소모, 상태 이상(중독, 기절) 등의 모든 상호작용을 `GameplayAbility`, `GameplayEffect`, `AttributeSet`으로 모듈화하여 설계했습니다.

\* Network Replication: 멀티플레이 환경에서의 상태 동기화 비용을 최소화하고 확장성을 확보했습니다.



\### 🌊 Advanced Physics \& Movement (C++)

\* Custom CharacterMovementComponent: 언리얼 기본 무브먼트를 확장하여 수중 환경에 맞는 물리 기반 6DOF 이동 시스템을 직접 구현했습니다.

\* Fluid Dynamics: 수중 마찰력, 부력, 관성을 시뮬레이션하여 사실적인 유영 감각(Swimming/Dashing)을 제공합니다.



\### 🎨 Rendering \& Shaders (HLSL)

\* Custom HLSL Pipeline: 심해의 공포스러운 분위기를 연출하기 위해 `Caustics`(물결 일렁임)와 `Volumetric Fog`(체적 안개) 쉐이더를 직접 작성하여 적용했습니다.

\* Optimization: Deferred Rendering 파이프라인 기반의 조명 처리와 Bloom 효과 최적화를 통해 높은 비주얼 퀄리티와 프레임 방어를 동시에 달성했습니다.



\### 🧠 AI System (EQS)

\* Environment Query System (EQS): 몬스터가 단순히 플레이어를 추적하는 것을 넘어, 지형지물을 활용해 숨거나 우회하는 지능적인 사냥 패턴을 구현했습니다.

\* Behavior Tree: 다양한 몬스터(추적형, 매복형, 군집형)의 복잡한 행동 로직을 계층적으로 설계했습니다.



---



\## 🦈 Characters \& Creatures



| 크리처 (Creature) | 특징 (Features) | 공략 포인트 (Strategy) |

| :--- | :--- | :--- |

| 독수리발상어 <br> \*(Eagle Ray Shark)\* | 시야에 포착되면 직선으로 급가속(Dash하여 공격 | 엄폐물을 활용하여 시야를 차단하고 돌진을 회피 |

| 대왕오징어 <br> \*(Giant Squid)\* | 촉수를 이용한 속박 및 중독 공격 | 잡혔을 때 팀원의 도움이나 빠른 상호작용으로 탈출 |

| 피라냐 무리 <br> \*(Piranha Swarm)\* | 군집 지능(Swarm AI을 이용해 플레이어를 포위 | 개체 수가 불어나기 전에 신속하게 제압하거나 도주 |

| 심해의 망령 <br> \*(Water Ghost)\* | 느리지만 멈추지 않고 다가오는 무적의 추적자 | 공격이 통하지 않음. 교란형 아이템을 사용하여 따돌려야 함 |



---



\## 🛠️ Installation \& Requirements



\### Minimum Requirements

\* OS: Windows 10/11 (64-bit)

\* Processor: Intel Core i5-8400 / AMD Ryzen 5 2600

\* Memory: 16 GB RAM

\* Graphics: NVIDIA GeForce GTX 1060 6GB

\* DirectX: Version 12

\* Storage: 50 GB available space



\### How to Build (For Developers)

1\.  Clone the repository:

&nbsp;   ```bash

&nbsp;   git clone \[https://github.com/your-username/AbyssCrawler.git](https://github.com/your-username/AbyssCrawler.git)

&nbsp;   ```

2\.  Right-click `AbyssCrawler.uproject` and select Generate Visual Studio project files.

3\.  Open `AbyssCrawler.sln` with Visual Studio 2022.

4\.  Set build configuration to Development Editor and build.

5\.  Launch the project via local debugger or `.uproject` file.



---



\## 👥 Team



| Name | Role | Responsibility |

| :---: | :---: | :--- |

| 박지성 | Client Lead | • Core Gameplay System (C++), GAS Implementation<br>• Character Control \& 6DOF Physics<br>• Rendering (Shader/VFX), UI System |

| 박병일 | Server Lead | • Multi-threaded TCP Server Implementation<br>• Packet Handling \& Synchronization<br>• Lobby \& Database System |

| 김범년 | Art Lead | • 3D Modeling (Character/Creature/Environment)<br>• Animation \& Rigging<br>• Level Design \& Texturing |



---



<p align="center">

&nbsp; Built with ❤️ by Team Abyss using Unreal Engine 5

</p>

