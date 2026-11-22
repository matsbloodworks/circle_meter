# 14. インタフェース制御文書 — Link G4X CAN (ICD)

| 項目 | 内容 |
|---|---|
| 文書ID | `DOC-14` |
| プロセス | SYS.3（外部インタフェース定義） |
| 版 | 0.1 (Draft) |
| 最終更新 | 2026-11-22 |
| 一次情報源 | 作者の車両の PCLink（G4X）の CAN Setup 画面（Stream 1 の Frame 1–14 と Mode タブ。2026-11-22 に画面で確認） |

Link G4X の ECU から受信するときのフレーム定義（`SYS-63`）。rusEFI のときの定義は `DOC-13`。
どちらを使うかはビルド時の設定で選ぶ（`SWR-104`）。**本機は送信しない**（`SYS-07`。rusEFI と同じ）。

---

## 1. 物理層・データリンク層

| 項目 | 値 | 備考 |
|---|---|---|
| 規格 | ISO 11898-2 High-Speed CAN | |
| ビットレート | **500 kbps**（作者の車両の PCLink の設定） | PCLink: CAN Setup → Mode → Bit Rate。本機の `USER_CAN_BITRATE_KBPS` と一致させる |
| ID 形式 | **11 bit 標準 ID**（PCLink の Format = Normal） | Extended は扱わない |
| ID | **1000（`0x3E8`）**（既定。PCLink の Channel の CAN ID） | 本機の `USER_LINK_CAN_ID` で変えられる |
| 全フレーム DLC | 8 バイト固定 | |
| バイトオーダ | **リトルエンディアン**（PCLink の Byte Order = LS First） | |
| 本機の送信 | **なし**（Listen Only） | `SYS-07` |

### 1.1 ECU 側の設定（PCLink）

| 場所 | 設定 | 備考 |
|---|---|---|
| CAN Setup → Mode → CAN Module | 本機をつなぐポート（作者の車両は CAN 1） | |
| 同 → Mode | User Defined | |
| 同 → Bit Rate | 500 kbit/s | |
| 同 → Data → Channel 1 | **Transmit User Stream 1**、ID 1000、Format Normal | 2026-11-22 時点で作者の車両は全 Channel が OFF（何も送っていない） |
| 同 → Streams → Stream 1 | §2 の Generic Dash 形式（Frame 1–14） | 作者の車両には入っている |
| 送信周期 | **20 Hz 以上** | 鮮度切れの判定（500 ms。`SYS-40`）に余裕を持たせる |

## 2. フレームの形式（Generic Dash 形式の多重化フレーム）

1 つの ID で、0 バイト目の番号を変えて複数のフレームを順に送る（PCLink の ID Position = 0）。

| バイト | 内容 |
|---|---|
| 0 | **フレーム番号**（PCLink の Frame の「ID (decimal)」。Frame 1 が 0） |
| 1 | 未使用（0） |
| 2–3 | 値 A（uint16 LE） |
| 4–5 | 値 B（uint16 LE） |
| 6–7 | 値 C（uint16 LE） |

PCLink は **生値 = 表示値 × Multiplier ÷ Divider + Offset** を整数に切り捨てて送る。
G4X は PCLink に表示されている単位のまま送るため、**PCLink の単位は °C・kPa・km/h にしておく**（作者の車両はこの設定。2026-11-22 確認）。
本機は逆算して **表示値 = (生値 − Offset) × Divider ÷ Multiplier** とする。

## 3. フレーム一覧（作者の車両の Stream 1）

「本機」の列: ● = 表示に使う / ○ = 受信するが使わない。

| 番号 | PCLink | バイト | 値 | 倍率 | Offset | 単位 | 本機の信号 |
|---|---|---|---|---|---|---|---|
| 0 | Frame 1 | 2–3 | Engine Speed | ×1 | 0 | rpm | ● `Rpm` |
| | | 4–5 | MAP | ×1 | 0 | kPa | ● `Map` |
| | | 6–7 | MGP | ×1 | 100 | kPa | ○ |
| 1 | Frame 2 | 2–3 | BAP | ×10 | 0 | kPa | ● `Baro` |
| | | 4–5 | TPS (Main) | ×10 | 0 | % | ○ |
| | | 6–7 | Injector Duty Cycle | ×10 | 0 | % | ● `InjDuty` |
| 2 | Frame 3 | 2–3 | Injector Duty Cycle (Sec) | ×10 | 0 | % | ○ |
| | | 4–5 | Injection Actual PW | ×1000 | 0 | ms | ● `InjPulseMs` |
| | | 6–7 | ECT | ×1 | 50 | °C | ● `Clt` |
| 3 | Frame 4 | 2–3 | IAT | ×1 | 50 | °C | ● `Iat` |
| | | 4–5 | Batt Voltage | ×100 | 0 | V | ● `BattVolt` |
| | | 6–7 | Mass Air Flow | ×10 | 0 | g/s | ○ |
| 4 | Frame 5 | 2–3 | Gear (Status) | ×1 | 0 | — | ● `Gear` |
| | | 4–5 | Injection Timing | ×1 | 0 | deg | ● `InjTiming` |
| | | 6–7 | Ignition Angle | ×10 | 1000 | deg | ● `IgnitionTiming` |
| 5 | Frame 6 | 2–7 | Inlet Bank 1/2・Exhaust Bank 1 Position | — | — | deg | ○ |
| 6 | Frame 7 | 2–3 | Exhaust Bank 2 Position | ×−10 | 0 | deg | ○ |
| | | 4–5 | **Lambda 1** | ×1000 | 0 | λ | ● `WboLambda`（§4.1） |
| | | 6–7 | Lambda 2 | ×1000 | 0 | λ | ○ `Lambda2` |
| 7 | Frame 8 | 2–3 | Trig1 Err Counter | ×1 | 0 | — | ○ |
| | | 4–5 | Fault Codes (Status) | ×1 | 0 | — | ● `StatusFlags`（§4.2） |
| | | 6–7 | Fuel Pressure | ×1 | 0 | kPa | ○ |
| 8 | Frame 9 | 2–3 | Oil Temperature | ×1 | 50 | °C | ● `OilTemp` |
| | | 4–5 | Oil Pressure | ×1 | 0 | kPa | ● `OilPressure` |
| | | 6–7 | LF Wheel Speed | ×10 | 0 | km/h | ● 車速（§4.3） |
| 9 | Frame 10 | 2–3 / 4–5 / 6–7 | LR / RF / RR Wheel Speed | ×10 | 0 | km/h | ● 車速（§4.3） |
| 10–12 | Frame 11–13 | — | Knock Table 1–8 Level、Limits Flags Word | — | — | — | ○ |
| 13 | Frame 14 | — | APS (Main)、% Ethanol、各種状態ビット | — | — | — | ○ |

### 3.1 追加フレーム（本機が定義する。PCLink で後から足す）

Generic Dash に無い値を送るため、本機は番号 14・15 を次の内容で受ける（`SWR-103`）。
**PCLink に追加するまでは届かないので、その値は `--` 表示になる**（`SYS-40` / `SYS-41`。値は作らない）。
追加するときは Stream 1 に Frame を足し、ID (decimal) を 14 / 15、各値を下表の位置・倍率で入れる。

| 番号 | PCLink | バイト（Start Position） | 値 | Multiplier | Divider | Offset | 単位 | 本機の信号 |
|---|---|---|---|---|---|---|---|---|
| 14 | Frame 15 | 2–3（23） | EGT 1 | 1 | 1 | 0 | °C | `Egt1` |
| | | 4–5（39） | EGT 2 | 1 | 1 | 0 | °C | `Egt2` |
| | | 6–7（55） | Fuel Level | 10 | 1 | 0 | % | `FuelLevel` |
| 15 | Frame 16 | 2–3（23） | Fuel Temperature | 1 | 1 | 50 | °C | `FuelTemp` |
| | | 4–5（39） | Ignition Dwell | 100 | 1 | 0 | ms | `DwellMs` |
| | | 6–7（55） | Idle Control Duty（ISC） | 10 | 1 | 0 | % | `IdlePosition` |

Width は 16、Byte Order は LS First、Type は Unsigned（Frame 1–14 と同じ）。
燃費（`FuelEcoInstant` / `FuelEcoAvg`）は Link に相当する値があるか未確認のため、まだ定義しない（`OPN-22`）。

## 4. 変換の規則

### 4.1 λ

- λ = 生値 ÷ 1000。`0.30 ≤ λ ≤ 6.5` のときだけ有効（`SYS-42`。rusEFI のときと同じ範囲）。
- 有効なら `WboLambda` に入れ、`WboValid` = 1。無効なら `WboLambda` は入れず `WboValid` = 0。
  λ の表示は rusEFI のときと同じく `WboLambda` を使う（`SYS-03` の表示経路をそのまま使う）。
- Link はセンサの状態（加熱中・故障）と温度を Generic Dash で送らない。このため **λ が無効なときの理由は出せず、
  状態は `Check` になる**（`SWR-29` の (4)。`OPN-23`）。
- 診断用に `Lambda1`（無効値は入れない）と `Lambda1Valid` も入れる。

### 4.2 状態フラグ

- Fault Codes が 0 以外なら `StatusFlags` の CEL ビット（`status_bit::kCheckEngine`）を立てる。それ以外のビットは 0。
- Limits Flags Word（番号 12）はビットの意味を確認していないので使わない（`OPN-23`）。

### 4.3 車速

作者の車両の車速はトランスミッションの速度計用センサから取っている。PCLink でそのセンサが
LF / LR / RF / RR のどの Wheel Speed に割り当てられているかは車両ごとに違うため、
**4 つのうち最大の値を車速とする**（使っていない車輪は 0 が送られる）。
番号 8 と 9 のどちらかを受けるたびに、直近の 4 値から最大値を出す。

### 4.4 その他

- 温度: 生値 − 50。
- 点火時期: (生値 − 1000) ÷ 10。
- 大気圧が 0（読めない）のときは入れない（rusEFI のときと同じ）。
- EGT が 0 以下のときは入れない（`RSK-09`。受信タスクで判定する）。

## 5. 不正なフレーム

- ID が設定値と違う、DLC が 8 未満、番号が 0–15 以外 → 捨てる（`SWR-06` と同じ扱い）。
- 番号が 0–15 で本機が使わないもの（5、10–13） → 受理するが信号は取り出さない。

## 6. 変更管理

PCLink の Stream 1 の中身を変えたら、本書 §3 と `lib/link_can/include/link_can_spec.h` を同時に更新する。
