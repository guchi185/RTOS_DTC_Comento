#include "pmic_mp5475.h"
#include <string.h>   /* memset */
#include <stddef.h>   /* NULL */

/* ============================================================================
 * i2c.h가 없을 때의 “현실적인 해결”
 * - 프로젝트 어디엔가 hi2c1이 정의돼 있으니 extern으로 가져다 씀
 * - 만약 네 프로젝트가 hi2c2를 쓰면 &hi2c2로 바꿔도 됨
 * ========================================================================== */
extern I2C_HandleTypeDef hi2c1;

/* ============================================================================
 * 내부 헬퍼: 1바이트 레지스터 Read/Write를 깔끔하게
 * - MP5475 레지스터는 8-bit address로 접근하는 형태로 구현
 * ========================================================================== */
HAL_StatusTypeDef PMIC_ReadReg(PMIC_Reg_t reg, uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) return HAL_ERROR;

    /* HAL_I2C_Mem_Read:
     * - DevAddress: 7-bit<<1 형태(PMIC_I2C_ADDR)
     * - MemAddress: reg(8-bit)
     */
    return HAL_I2C_Mem_Read(&hi2c1,
                            PMIC_I2C_ADDR,
                            (uint16_t)reg,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            len,
                            100);
}

HAL_StatusTypeDef PMIC_WriteReg(PMIC_Reg_t reg, const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) return HAL_ERROR;

    return HAL_I2C_Mem_Write(&hi2c1,
                             PMIC_I2C_ADDR,
                             (uint16_t)reg,
                             I2C_MEMADD_SIZE_8BIT,
                             (uint8_t*)data,
                             len,
                             100);
}

/* ============================================================================
 * Fault frame 읽기 (과제에서 8바이트 union을 원하니 8바이트 read로 “형태”를 맞춤)
 * - 실제 칩의 특정 fault status는 1바이트일 수도 있지만,
 *   과제 요구사항이 “8바이트 데이터” 프레임을 강조하니 컨테이너로 읽는 버전 제공
 * - 여기서는 UV/OV/OC 레지스터를 각각 1바이트씩 읽고 raw[0]에 합쳐 넣는 방식으로
 *   “8바이트 프레임”을 구성(과제 친화)
 * ========================================================================== */
HAL_StatusTypeDef PMIC_ReadFaultFrame(PMIC_FaultFrame_u *frame)
{
    if (frame == NULL) return HAL_ERROR;

    memset(frame->raw, 0, sizeof(frame->raw));

    uint8_t uv = 0, ov = 0, oc = 0;

    /* 각 Fault Status 레지스터 주소(0x07/0x08/0x09)는 데이터시트에 명시 :contentReference[oaicite:12]{index=12} */
    if (PMIC_ReadReg(PMIC_REG_UV_STATUS, &uv, 1) != HAL_OK) return HAL_ERROR;
    if (PMIC_ReadReg(PMIC_REG_OV_STATUS, &ov, 1) != HAL_OK) return HAL_ERROR;
    if (PMIC_ReadReg(PMIC_REG_OC_STATUS, &oc, 1) != HAL_OK) return HAL_ERROR;

    /* 과제용 단순 매핑:
     * - uv/ov/oc 값을 “0 아니면 fault 발생”으로 간주해서 bit로 넣음
     * - 실제 제품이라면 각 레지스터의 비트 정의에 맞춰 정확히 파싱해야 함
     */
    frame->bit.uv = (uv != 0) ? 1 : 0;
    frame->bit.ov = (ov != 0) ? 1 : 0;
    frame->bit.oc = (oc != 0) ? 1 : 0;

    return HAL_OK;
}

HAL_StatusTypeDef PMIC_ReadVoutA_Code(uint8_t *reg13, uint8_t *reg14)
{
    if (reg13 == NULL || reg14 == NULL) return HAL_ERROR;

    /* 0x13: VOUT_SELECTA + V_REFA_HIGH */
    if (PMIC_ReadReg(PMIC_REG_VOUTA_SELECT_REFA_HIGH, reg13, 1) != HAL_OK)
        return HAL_ERROR;

    /* 0x14: V_REFA_LOW */
    if (PMIC_ReadReg(PMIC_REG_VREFA_LOW, reg14, 1) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}


/* ============================================================================
 * Fault 판정 함수들
 * ========================================================================== */
bool PMIC_IsUV(void)
{
    PMIC_FaultFrame_u f;
    if (PMIC_ReadFaultFrame(&f) != HAL_OK) return false;
    return (f.bit.uv == 1);
}

bool PMIC_IsOV(void)
{
    PMIC_FaultFrame_u f;
    if (PMIC_ReadFaultFrame(&f) != HAL_OK) return false;
    return (f.bit.ov == 1);
}

bool PMIC_IsOC(void)
{
    PMIC_FaultFrame_u f;
    if (PMIC_ReadFaultFrame(&f) != HAL_OK) return false;
    return (f.bit.oc == 1);
}

/* ============================================================================
 * VOUT 제어 핵심 개념 (데이터시트 기반)
 *
 * 1) VOUT_SELECTx:
 *    - 0: VFB = VREF
 *    - 1: VFB = 2*VREF (0.6~4.096V 범위) :contentReference[oaicite:13]{index=13}
 *
 * 2) VREF 설정:
 *    - V_REFx_HIGH 먼저 설정 후 V_REFx_LOW 설정해야 실행됨 :contentReference[oaicite:14]{index=14}
 *    - V_REFx_LOW는 300mV~2.048V, 2mV/step :contentReference[oaicite:15]{index=15}
 *
 * 3) Register:
 *    - Buck A 기준
 *      0x13: VOUT_SELECTA + V_REFA_HIGH
 *      0x14: V_REFA_LOW :contentReference[oaicite:16]{index=16}
 * ========================================================================== */

/* VREF 코드 변환(과제용 단순 모델)
 * - 데이터시트 예시: {HIGH,LOW} = 00_1001_0110 이 300mV :contentReference[oaicite:17]{index=17}
 * - 여기서 LOW=0x26, HIGH=0
 * - 그래서 "code10bit - 0x026" 만큼 2mV씩 증가한다고 가정 (과제용)
 */
static bool PMIC_EncodeVref_mV(uint16_t vref_mV, uint8_t *out_high2bit, uint8_t *out_low8bit)
{
    if (out_high2bit == NULL || out_low8bit == NULL) return false;

    if (vref_mV < 300 || vref_mV > 2048) return false;

    uint16_t step = (uint16_t)((vref_mV - 300) / 2); /* 2mV per step */
    uint16_t code10 = (uint16_t)(0x026 + step);      /* 300mV가 0x026 기준 */

    if (code10 > 0x3FF) return false; /* 10bit max */

    *out_high2bit = (uint8_t)((code10 >> 8) & 0x03); /* 상위 2비트 */
    *out_low8bit  = (uint8_t)(code10 & 0xFF);        /* 하위 8비트 */
    return true;
}

/* Buck A의 VOUT(mV) 설정 */
HAL_StatusTypeDef PMIC_SetVoutA_mV(uint16_t vout_mV)
{
    /* VOUT_SELECT=0이면 VOUT = VREF (최대 2048mV)
     * VOUT_SELECT=1이면 VOUT = 2*VREF (최대 4096mV)
     */
    uint8_t vout_select = 0;  /* 0 or 1 */
    uint16_t vref_mV = 0;

    if (vout_mV <= 2048)
    {
        vout_select = 0;
        vref_mV = vout_mV;
    }
    else if (vout_mV <= 4096)
    {
        vout_select = 1;
        vref_mV = (uint16_t)(vout_mV / 2);
    }
    else
    {
        return HAL_ERROR; /* 범위 밖 */
    }

    uint8_t vref_high = 0;
    uint8_t vref_low  = 0;

    if (!PMIC_EncodeVref_mV(vref_mV, &vref_high, &vref_low))
    {
        return HAL_ERROR;
    }

    /* ---------------------------
     * 0x13 레지스터 구성:
     * - bit7: VOUT_SELECTA
     * - bit1:0: V_REFA_HIGH (2비트)
     * - 중간 RESERVED는 0으로 둠 (과제용 안전값)
     * --------------------------- */
    uint8_t reg13 = 0;
    reg13 |= (uint8_t)(vout_select << 7);     /* VOUT_SELECTA */
    reg13 |= (uint8_t)(vref_high & 0x03);     /* V_REFA_HIGH 2비트 */

    /* 데이터시트 권고: V_REFx_HIGH 먼저, 그다음 V_REFx_LOW 설정해야 실행됨 :contentReference[oaicite:18]{index=18} */
    if (PMIC_WriteReg(PMIC_REG_VOUTA_SELECT_REFA_HIGH, &reg13, 1) != HAL_OK)
        return HAL_ERROR;

    /* 0x14: V_REFA_LOW */
    if (PMIC_WriteReg(PMIC_REG_VREFA_LOW, &vref_low, 1) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}
