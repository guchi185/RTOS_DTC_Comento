#include "fault_dtc.h"
#include "pmic_mp5475.h"
#include "dtc.h"
#include "eeprom_25lc256.h"
#include "diagnostic.h"

/* EEPROM에 DTC 저장할 주소(임의). 과제 단계에서는 고정 주소 하나로 충분 */
#define EEPROM_DTC_ADDR  (0x0000u)

void FaultDTC_Task(void)
{
    /* 1) PMIC에서 fault 상태 확인 (예: UV) */
    bool uv = PMIC_IsUV();

    if (!uv)
        return; // fault 없으면 아무 것도 안 함

    /* 2) DTC 생성(8바이트) */
    DTC_Frame_u dtc;
    (void)DTC_Build(&dtc, DTC_FAULT_UV, 0x0000); // value는 과제 단계 0이어도 OK

    /* 3) EEPROM에 저장 */
    (void)EEPROM_WriteBytes(EEPROM_DTC_ADDR, dtc.raw, 8u);

    /* 4) 읽어서 확인(Read-back) */
    DTC_Frame_u dtc_rb;
    (void)EEPROM_ReadBytes(EEPROM_DTC_ADDR, dtc_rb.raw, 8u);

    /* 5) UART로 결과 출력 */
    DIAG_UartPrint("[FAULT] UV detected. Stored DTC:\r\n");
    DIAG_PrintDTC(&dtc_rb);

    /* 6) CAN으로도 전송(과제 단계: raw 8바이트 송신) */
    (void)DIAG_SendDTC_Can(&dtc_rb);
}
