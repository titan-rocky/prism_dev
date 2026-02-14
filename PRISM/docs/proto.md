# Protocol Documentation

This document outlines the protocol-based variables, payload structures, and assumptions used in the PRISM project for network traffic analysis and intrusion detection.

## Supported Protocols

PRISM currently supports parsing and analysis for the following SCADA protocols:

1.  **Modbus TCP**
2.  **DNP3** (Distributed Network Protocol)
3.  **IEC 60870-5-104** (IEC 104)

## Common Data Structures

The core data structure for parsed traffic is the `ParsedRecord`, defined in `include/prism.hpp`:

| Variable | Type | Description |
| :--- | :--- | :--- |
| `timestamp` | `Timestamp` | Time of packet capture |
| `protocol` | `std::string` | Protocol name (Modbus, DNP3, IEC104) |
| `isRequest` | `bool` | True if packet is a request, False otherwise |
| `isWrite` | `bool` | True if packet modifies state (non-read op) |
| `functionCode`| `uint8_t` | Protocol-specific function code / Type ID |
| `address` | `uint16_t` | Target register or object address |

---

## Protocol Details

### 1. Modbus TCP

**Detection Criteria:**
- **Port:** Destination Port `502` (for requests)
- **Minimum Payload Size:** 12 bytes

**Payload Structure:**
The parser assumes a standard MBAP header.

| Offset | Field | Description |
| :--- | :--- | :--- |
| 7 | Function Code | `functionCode` |
| 8-9 | Reference Number | `address` (16-bit, Big Endian) |

**Assumptions & Logic:**
- **Request Direction:** Identified if `dstPort` is 502.
- **Write Operation:** `isWrite` is true if `functionCode >= 5`.
- **Parsing:** Starts at offset 7 (skipping MBAP header).

### 2. DNP3

**Detection Criteria:**
- **Port:** Source or Destination Port `20000`
- **Minimum Payload Size:** 12 bytes

**Payload Structure:**

| Offset | Field | Description |
| :--- | :--- | :--- |
| 2 | Control Byte | Bit 0x40 indicates direction (Initiating) |
| 6 | Function Code | `functionCode` |

**Assumptions & Logic:**
- **Request Direction:** `isRequest` is true if the Control Byte has the 0x40 bit set.
- **Write Operation:** Considered non-read side-effect for:
    - `0x02` (Write)
    - `0x05` (Direct Operate)
    - `0x06` (Select)
- **Address:** Not parsed for DNP3 (set to 0).

### 3. IEC 60870-5-104

**Detection Criteria:**
- **Port:** Source or Destination Port `2404`
- **Start Byte:** Payload must start with `0x68`
- **Minimum Payload Size:** 12 bytes

**Payload Structure:**

| Offset | Field | Description |
| :--- | :--- | :--- |
| 6 | Type ID | `functionCode` (ASDU Type) |
| 8 | Cause of Transmission | COT, used to determine direction |
| 9-11 | IOA | Information Object Address (24-bit, Little Endian) |

**Assumptions & Logic:**
- **Filtering:** Drops frames with `TypeID == 0` (APCI only).
- **Request Direction:** `isRequest` is true if OCT (Cause of Transmission) is 6 (Activation).
- **Write Operation:** `isWrite` is true for command types 45-50.
- **Address:** Parsed as a 24-bit integer from bytes 9-11.
---

## Testing Constraints

For accurate detection, test traffic must adhere to:

1.  **Standard Ports:** traffic must use ports 502 (Modbus), 20000 (DNP3), or 2404 (IEC 104).
2.  **Minimum Integrity:** Packets < 12 bytes or missing start bytes (IEC 104 `0x68`) are dropped.
3.  **Critical Function Codes:** Use the specific "Write" function codes listed above to trigger high-risk alerts.

## References

1.  **Cisco Snort 3 Inspector Reference - IEC104**: Validates the approach of normalizing IEC 104 traffic by focusing on ASDU Type IDs and filtering out APCI-only frames for effective intrusion detection.
    *   [PDF Reference](https://www.cisco.com/c/en/us/td/docs/security/secure-firewall/snort3-inspectors/snort-3-inspector-reference/iec104-inspector.pdf)
2.  **Modbus TCP/IP Protocol Analysis**: Provides improved understanding of the Modbus TCP/IP protocol.
    *   [ProSoft Technology Reference](https://www.prosoft-technology.com/kb/assets/intro_modbustcp.pdf)
3.  **DNP3 Overview**: Offers insights into the Distributed Network Protocol (DNP3) structure and application.
    *   [DNP.org Reference](https://www.dnp.org/About/Overview-of-DNP3-Protocol)

