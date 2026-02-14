# PRISM Architecture & Role

I designed PRISM to operate as a passive, out-of-band Intrusion Detection System (IDS) specifically for OT networks. It sits on a SPAN/mirror port, so it does not introduce latency to the control loop. My parsing engine disassembles Modbus, DNP3, and IEC-104 payload headers to extract semantic features without needing deep stateful inspection of the entire sessions, which keeps processing logic lightweight and fast. It focuses on detecting "Inter-arrival" regularities and "Write Intent" anomalies that characterize both sabotage attempts and flooding attacks.
