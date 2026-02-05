#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include <stdint.h>
#include <stdbool.h>
#include "dtc.h"

/* ============================================================
 * Diagnostic 역할
 * - UART: 사람이 보기 쉽게 로그 출력
 * - CAN : 외부 장비(진단기)로 DTC 전달 (과제 단계에서는 "송신 함수 뼈대"면 OK)
 * ============================================================ */

/* UART 로그 출력 */
void DIAG_UartPrint(const char *msg);

/* DTC를 UART로 보기 쉽게 출력 */
void DIAG_PrintDTC(const DTC_Frame_u *dtc);

/* DTC를 CAN으로 송신(과제 단계: raw 8바이트 payload 송신 뼈대) */
bool DIAG_SendDTC_Can(const DTC_Frame_u *dtc);

#endif
