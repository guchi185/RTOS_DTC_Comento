#ifndef DTC_H
#define DTC_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * [피드백 반영] DTC 자체를 enum으로 정의
 * - 기존: fault type -> 함수에서 dtc_id 하드코딩 매핑
 * - 변경: dtc_id(실제 고장코드)를 enum으로 관리 (0x1001 등)
 * ============================================================ */

/* 실제 DTC Code(고장코드) enum */
typedef enum
{
    DTC_ID_NONE = 0x0000,

    DTC_ID_UV   = 0x1001,  // Under Voltage
    DTC_ID_OV   = 0x1002,  // Over Voltage
    DTC_ID_OC   = 0x1003,  // Over Current
    DTC_ID_TEMP = 0x1004   // Temperature
} DTC_Id_e;

/* 원인 타입을 따로 유지하고 싶으면 남겨도 됨 */
typedef enum
{
    DTC_FAULT_NONE = 0,
    DTC_FAULT_UV   = 1,
    DTC_FAULT_OV   = 2,
    DTC_FAULT_OC   = 3,
    DTC_FAULT_TEMP = 4
} DTC_FaultType_t;

/* 8바이트 DTC 프레임 (고정 길이) */
typedef struct
{
    uint16_t dtc_id;      /* [0..1] 실제 DTC 코드 */
    uint8_t  fault_type;  /* [2] (선택) UV/OV/OC... */
    uint8_t  subcode;     /* [3] severity 등 확장 */
    uint16_t value;       /* [4..5] 측정값/레지스터값 */
    uint8_t  counter;     /* [6] 발생 횟수 */
    uint8_t  checksum;    /* [7] reserved */
} DTC_Frame_t;

/* EEPROM/CAN 전송을 위해 raw[8] 접근 가능 */
typedef union
{
    DTC_Frame_t f;
    uint8_t     raw[8];
} DTC_Frame_u;

/**
 * @brief  DTC 프레임 생성(8바이트)
 * @param  out   결과
 * @param  id    DTC enum (DTC_ID_UV 등)
 * @param  type  fault 타입
 * @param  value 부가값
 */
bool DTC_Build(DTC_Frame_u *out, DTC_Id_e id, DTC_FaultType_t type, uint16_t value);

#endif /* DTC_H */
