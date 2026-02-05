#ifndef PMIC_MP5475_H
#define PMIC_MP5475_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * 1) I2C Slave Address (매크로로 고정)
 * - HAL_I2C_xxx 함수는 7-bit 주소를 <<1 해서 넣는 스타일을 많이 씀
 * - 너가 이미 쓰던 (0x60 << 1) 유지
 * ========================================================================== */
#define PMIC_I2C_ADDR        (0x60U << 1)   /* MP5475 7-bit addr=0x60 가정 */

HAL_StatusTypeDef PMIC_ReadVoutA_Code(uint8_t *reg13, uint8_t *reg14);

/* ============================================================================
 * 2) Register Address (enum으로 정리)  ← 과제 요구사항 포인트
 * - Fault Status: UV/OV/OC/PG 레지스터 주소는 데이터시트에 명시 :contentReference[oaicite:4]{index=4}
 * - VOUT 설정은 Register Map에서 A 채널 기준:
 *   0x13: VOUT_SELECTA + V_REFA_HIGH
 *   0x14: V_REFA_LOW     (Register map 발췌) :contentReference[oaicite:5]{index=5}
 * ========================================================================== */
typedef enum
{
    /* ---- Fault status (Read) ---- */
    PMIC_REG_PG_STATUS   = 0x06,  /* PG Fault Status :contentReference[oaicite:6]{index=6} */
    PMIC_REG_UV_STATUS   = 0x07,  /* UV Fault Status :contentReference[oaicite:7]{index=7} */
    PMIC_REG_OV_STATUS   = 0x08,  /* OV Fault Status :contentReference[oaicite:8]{index=8} */
    PMIC_REG_OC_STATUS   = 0x09,  /* OC Fault Status :contentReference[oaicite:9]{index=9} */

    /* ---- VOUT setting (Write) : Buck A 예시 ---- */
    PMIC_REG_VOUTA_SELECT_REFA_HIGH = 0x13, /* VOUT_SELECTA + V_REFA_HIGH :contentReference[oaicite:10]{index=10} */
    PMIC_REG_VREFA_LOW              = 0x14  /* V_REFA_LOW :contentReference[oaicite:11]{index=11} */

} PMIC_Reg_t;

/* ============================================================================
 * 3) 8바이트 프레임(Union) 정의 (과제 요구사항)
 * - 과제에서 “Data는 8바이트 기능의 데이터를 쓰기 쉽게 Union/비트필드로 정의”
 * - 실제 칩 레지스터가 “항상 8바이트”는 아닐 수 있는데,
 *   과제 요구사항이 8바이트 기준 프레임을 원하니 “프레임 컨테이너”로 잡아둠.
 * ========================================================================== */
typedef union
{
    uint8_t raw[8];

    /* 예시: raw[0]의 bit0/1/2를 UV/OV/OC로 간단 매핑(과제용) */
    struct
    {
        uint8_t uv : 1;   /* Under Voltage fault */
        uint8_t ov : 1;   /* Over Voltage fault */
        uint8_t oc : 1;   /* Over Current fault */
        uint8_t rsv0 : 5;

        uint8_t rsv_rest[7]; /* 나머지 바이트는 확장용 */
    } bit;
} PMIC_FaultFrame_u;

/* ============================================================================
 * 4) PMIC 제어 API (pmic.c에서 구현)
 * ========================================================================== */
HAL_StatusTypeDef PMIC_ReadReg(PMIC_Reg_t reg, uint8_t *data, uint16_t len);
HAL_StatusTypeDef PMIC_WriteReg(PMIC_Reg_t reg, const uint8_t *data, uint16_t len);

HAL_StatusTypeDef PMIC_ReadFaultFrame(PMIC_FaultFrame_u *frame);

/* Fault 판정 helper */
bool PMIC_IsUV(void);
bool PMIC_IsOV(void);
bool PMIC_IsOC(void);

/* VOUT 제어 (Buck A 예시) */
HAL_StatusTypeDef PMIC_SetVoutA_mV(uint16_t vout_mV);

#endif /* PMIC_MP5475_H */
