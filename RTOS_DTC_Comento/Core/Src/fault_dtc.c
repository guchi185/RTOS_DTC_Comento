#include "fault_dtc.h"
#include "pmic_mp5475.h"
#include "dtc.h"
#include "eeprom_25lc256.h"
#include "diagnostic.h"

/* ============================================================
 * [하드코딩 제거] 반복 상수는 매크로로 정의
 * ============================================================ */

/* DTC 프레임은 과제 기준 8바이트 고정 */
#define DTC_FRAME_LEN_BYTES      (8u)

/* EEPROM에 DTC 저장할 시작 주소 (과제 단계: 고정 주소 1개 사용)
 * - 향후 확장 시: 링버퍼/인덱스 방식으로 여러 개 저장 가능
 */
#define EEPROM_ADDR_DTC_BASE     (0x0000u)

/* 과제 단계에서 value(부가값)를 사용하지 않을 때의 기본값 */
#define DTC_VALUE_UNUSED         (0x0000u)

/* ============================================================
 * FaultDTC_Task
 * - PMIC Fault(UV 등) 발생 시 DTC 생성 → EEPROM 저장 → Read-back 검증
 *   → UART 로그 출력 → CAN 송신
 * ============================================================ */
void FaultDTC_Task(void)
{
    /* 1) PMIC에서 fault 상태 확인 (현재: UV만 예시 구현)
     * - 요구사항 확장 시: OV/OC/TEMP도 동일 패턴으로 추가 가능
     */
    const bool uv = PMIC_IsUV();
    if (!uv)
    {
        /* fault 없으면 아무 작업도 하지 않음 */
        return;
    }

    /* 2) DTC 생성(8바이트) */
    DTC_Frame_u dtc;
    if (!DTC_Build(&dtc, DTC_ID_UV, DTC_FAULT_UV, DTC_VALUE_UNUSED))
    {
        DIAG_UartPrint("[FAULT] DTC_Build failed\r\n");
        return;
    }

    /* 3) EEPROM에 저장 (SPI) */
    if (!EEPROM_WriteBytes((uint16_t)EEPROM_ADDR_DTC_BASE, dtc.raw, DTC_FRAME_LEN_BYTES))
    {
        DIAG_UartPrint("[FAULT] EEPROM write failed\r\n");
        return;
    }

    /* 4) Read-back으로 저장 검증 */
    DTC_Frame_u dtc_rb;
    if (!EEPROM_ReadBytes((uint16_t)EEPROM_ADDR_DTC_BASE, dtc_rb.raw, DTC_FRAME_LEN_BYTES))
    {
        DIAG_UartPrint("[FAULT] EEPROM readback failed\r\n");
        return;
    }

    /* 5) UART 로그 출력 (Polling 방식) */
    DIAG_UartPrint("[FAULT] UV detected. Stored DTC (readback):\r\n");
    DIAG_PrintDTC(&dtc_rb);

    /* 6) CAN 송신 (과제 단계: raw 8바이트 송신) */
    (void)DIAG_SendDTC_Can(&dtc_rb);
}
