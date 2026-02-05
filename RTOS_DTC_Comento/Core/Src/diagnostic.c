#include "diagnostic.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

/* 프로젝트에 생성된 핸들 이름에 맞춰 extern */
extern UART_HandleTypeDef huart4;
extern CAN_HandleTypeDef  hcan1;

/**
 * @brief UART로 문자열 출력(블로킹)
 */
void DIAG_UartPrint(const char *msg)
{
    if (msg == NULL) return;

    (void)HAL_UART_Transmit(&huart4,
                            (uint8_t*)msg,
                            (uint16_t)strlen(msg),
                            100u);
}

/**
 * @brief DTC를 보기 쉬운 형태로 UART 출력
 */
void DIAG_PrintDTC(const DTC_Frame_u *dtc)
{
    if (dtc == NULL) return;

    char buf[128];

    /* DTC 내용을 사람이 읽기 좋게 출력 */
    (void)snprintf(buf, sizeof(buf),
                   "[DTC] ID=0x%04X, type=%u, value=0x%04X, cnt=%u\r\n",
                   dtc->f.dtc_id,
                   dtc->f.fault_type,
                   dtc->f.value,
                   dtc->f.counter);

    DIAG_UartPrint(buf);
}

/**
 * @brief DTC를 CAN으로 송신 (과제 단계: "송신 가능 구조"만 갖춰도 OK)
 * @note  실제 UDS(0x19/0x14)는 다음 단계에서 확장.
 */
bool DIAG_SendDTC_Can(const DTC_Frame_u *dtc)
{
    if (dtc == NULL) return false;

    CAN_TxHeaderTypeDef txHeader;
    uint32_t txMailbox = 0;

    /* 예: 표준 ID 0x700 사용(임의). 네 과제/보드에 맞게 바꿔도 됨 */
    txHeader.StdId = 0x700;
    txHeader.ExtId = 0;
    txHeader.IDE   = CAN_ID_STD;
    txHeader.RTR   = CAN_RTR_DATA;
    txHeader.DLC   = 8;            // DTC raw 8바이트를 그대로 실어보내기
    txHeader.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, (uint8_t*)dtc->raw, &txMailbox) != HAL_OK)
        return false;

    return true;
}
