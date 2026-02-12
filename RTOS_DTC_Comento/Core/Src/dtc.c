#include "dtc.h"
#include <stddef.h>   // NULL 정의
#include <stdbool.h>

bool DTC_Build(DTC_Frame_u *out, DTC_Id_e id, DTC_FaultType_t type, uint16_t value)
{
    if (out == NULL) return false;

    /* 1) raw 초기화 */
    for (int i = 0; i < 8; i++) out->raw[i] = 0;

    /* 2) 필드 채우기 */
    out->f.dtc_id     = (uint16_t)id;      /* enum에서 바로 가져옴(하드코딩 제거) */
    out->f.fault_type = (uint8_t)type;     /* 타입 유지 */
    out->f.subcode    = 0;
    out->f.value      = value;
    out->f.counter    = 1;
    out->f.checksum   = 0;

    return true;
}
