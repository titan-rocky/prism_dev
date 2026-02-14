# Heuristic Parameters

I specifically tuned these values to balance false positives against detection sensitivity for the prototype.

| Parameter | Type | Value | Justification |
| :--- | :--- | :--- | :--- |
| **Window Size** | `chrono::ms` | `5000` | Captures enough context to see bursts, but short enough to react in seconds. |
| **Window Step** | `chrono::ms` | `1000` | Provides a sliding update every second for near real-time assessment. |
| **Safety Baseline (z)** | `double` | `10.0` | Defines the score at which the risk probability is 0.5 (inflection point). |
| **Sigmoid Steepness (k)** | `double` | `2.0` | Controls how fast risk escalates after crossing the safety baseline. |
| **Hysteresis ON** | `double` | `0.7` | Requires significant evidence (p>0.7) to trigger an alert, reducing "flapping". |
| **Hysteresis OFF** | `double` | `0.5` | Requires the threat to subside completely before clearing the alert. |
| **Write Risk Coeff** | `double` | `50.0` | Heavily penalizes write operations as they carry actual kinetic risk. |
| **Func Risk Coeff** | `double` | `5.0` | Reduced leverage on concentration to allow specific normal polling loops. |
| **Spray Risk Coeff** | `double` | `20.0` | High penalty for addressing multiple targets rapidly (reconnaissance/spray). |
| **Burst Volume Req** | `double` | `5.0` | Minimum packet count in a window to consider timing analysis relevant. |
| **Burst Speed Limit** | `double` | `0.1s` | Inter-arrival mean below this (100ms) is flagged as machine-speed anomaly. |
