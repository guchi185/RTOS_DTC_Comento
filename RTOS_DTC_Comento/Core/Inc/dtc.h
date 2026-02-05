#ifndef DTC_H
#define DTC_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * [과제 친화] DTC를 8바이트로 정의
 * - EEPROM에 저장하기 쉽고(고정 길이)
 * - CAN/UART로 전송/출력하기도 쉬움
 * ============================================================ */

typedef enum
{
    DTC_FAULT_NONE = 0,
    DTC_FAULT_UV   = 1,  // Under Voltage
    DTC_FAULT_OV   = 2,  // Over Voltage
    DTC_FAULT_OC   = 3,  // Over Current
    DTC_FAULT_TEMP = 4   // Temperature (warn/shutdown)
} DTC_FaultType_t;

/* 8바이트 DTC 프레임 (고정 길이) */
typedef struct
{
    /* [0..1] DTC ID (예: 0x1234 같은 고장코드) */
    uint16_t dtc_id;

    /* [2] Fault Type (UV/OV/OC/TEMP) */
    uint8_t  fault_type;

    /* [3] Severity / Subcode (과제 단계에선 0으로 둬도 됨) */
    uint8_t  subcode;

    /* [4..5] Optional value (예: 읽은 레지스터 값 일부, 또는 0) */
    uint16_t value;

    /* [6] counter (반복 발생 횟수 등. 과제 단계는 1로 고정해도 OK) */
    uint8_t  counter;

    /* [7] checksum/reserved (과제 단계는 0으로 둬도 OK) */
    uint8_t  checksum;
} DTC_Frame_t;

/* DTC를 “바로 EEPROM에 쓰기 위해” 8바이트 배열로도 보기 쉽게 union 제공 */
typedef union
{
    DTC_Frame_t f;
    uint8_t     raw[8];
} DTC_Frame_u;

/* ============================================================
 * API
 * ============================================================ */

/**
 * @brief  Fault Type에 맞는 DTC 프레임 생성(8바이트)
 * @param  out   생성 결과를 담을 DTC_Frame_u 포인터
 * @param  type  fault 타입(UV/OV/OC/TEMP)
 * @param  value 디버그용 값(레지스터 값 일부 등). 몰라도 0 넣으면 됨.
 * @return true면 생성 성공
 */
bool DTC_Build(DTC_Frame_u *out, DTC_FaultType_t type, uint16_t value);

#endif
