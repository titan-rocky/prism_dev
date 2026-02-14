# Feature Taxonomy Evaluation

I evaluated the implemented codebase against the proposed feature taxonomy. Only features present in the C++ code are listed as "Implemented".

### Common Features
| Feature Class | Implemented Metric | Code Reference |
| :--- | :--- | :--- |
| **Timing** | Inter-arrival Mean | `fv.values[5]` (FeatureExtractor) |
| **Timing** | Inter-arrival Std Dev | `fv.values[6]` (FeatureExtractor) |
| **Traffic Shape** | Packet Count (Window) | `fv.values[0]` (FeatureExtractor) |
| **Entropy** | Address Entropy (Shannon) | `fv.values[3]` (calculates entropy of target distribution) |
| **Endpoint Consistency** | Target Density | `fv.values[7]` (max count per address / total) |

### Protocol-Specific Logic
| Feature Class | Implemented Metric | Code Reference |
| :--- | :--- | :--- |
| **Function Codes** | Write Ratio | `fv.values[1]` (counts `isWrite` flags) |
| **Function Codes** | Function Concentration | `fv.values[2]` (counts max recurring type ID) |
| **Protocol Mix** | Protocol Dominance | `fv.values[4]` (identifies if one protocol floods the link) |

*Note: Complex TCP signals (SYN/FIN) and RTT measurements listed in the broader taxonomy were not implemented in this version to maintain the "lightweight" constraint.*
