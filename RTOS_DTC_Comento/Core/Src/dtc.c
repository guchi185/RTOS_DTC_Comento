#include "dtc.h"
#include <stddef.h>   // NULL 정의
#include <stdbool.h>

/* ============================================================
 * 과제 단계에서는 “DTC ID 매핑”을 단순하게 가져가도 충분함
 * - 나중에 요구사항이 있으면 여기만 고치면 됨(유지보수 포인트)
 * ============================================================ */
static uint16_t DTC_MapId(DTC_FaultType_t type)
{
    switch (type)
    {
        case DTC_FAULT_UV:   return 0x1001;
        case DTC_FAULT_OV:   return 0x1002;
        case DTC_FAULT_OC:   return 0x1003;
        case DTC_FAULT_TEMP: return 0x1004;
        default:             return 0x0000;
    }
}

bool DTC_Build(DTC_Frame_u *out, DTC_FaultType_t type, uint16_t value)
{
    if (out == NULL) return false;

    /* 1) raw를 0으로 초기화 (안전: 쓰레기값 방지) */
    for (int i = 0; i < 8; i++) out->raw[i] = 0;

    /* 2) 필드 채우기 */
    out->f.dtc_id     = DTC_MapId(type);
    out->f.fault_type = (uint8_t)type;
    out->f.subcode    = 0;          // 과제 단계: 미사용
    out->f.value      = value;      // 과제 단계: 0이어도 OK
    out->f.counter    = 1;          // 최초 발생 1회로 시작
    out->f.checksum   = 0;          // 과제 단계: 미사용

    return true;
}
