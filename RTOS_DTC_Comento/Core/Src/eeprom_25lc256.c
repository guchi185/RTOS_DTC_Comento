#include "eeprom_25lc256.h"
#include "main.h"     // GPIO 포트/핀 정의, HAL 포함
#include <string.h>   // memcpy 등(필요 시)

/* ============================================================
 * SPI 핸들: 프로젝트에 맞게 맞추기
 * - CubeMX가 생성한 SPI 핸들 이름이 hspi1이 아닐 수도 있음
 * ============================================================ */
extern SPI_HandleTypeDef hspi1;

/* ============================================================
 * EEPROM CS 핀 설정 (너 보드에 맞게 수정!)
 * 예:
 * - EEPROM_CS_GPIO_Port / EEPROM_CS_Pin 이 main.h에 생성돼 있으면 그대로 쓰면 됨
 * - 없다면, GPIO 포트/핀을 너가 직접 정의해야 함
 * ============================================================ */
#ifndef EEPROM_CS_GPIO_Port
#define EEPROM_CS_GPIO_Port GPIOB
#endif

#ifndef EEPROM_CS_Pin
#define EEPROM_CS_Pin GPIO_PIN_0
#endif

/* CS 제어 함수 */
static inline void EEPROM_CS_Low(void)
{
    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_RESET);
}

static inline void EEPROM_CS_High(void)
{
    HAL_GPIO_WritePin(EEPROM_CS_GPIO_Port, EEPROM_CS_Pin, GPIO_PIN_SET);
}

/* SPI 한 바이트 전송/수신 헬퍼 */
static uint8_t SPI_TxRxByte(uint8_t tx)
{
    uint8_t rx = 0;
    /* HAL_SPI_TransmitReceive: tx 1바이트 보내고 rx 1바이트 받기 */
    (void)HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1u, 100u);
    return rx;
}

/* Write Enable */
static void EEPROM_WriteEnable(void)
{
    EEPROM_CS_Low();
    (void)SPI_TxRxByte((uint8_t)EEPROM_CMD_WREN);
    EEPROM_CS_High();  // IMPORTANT: CS High에서 WEL latch가 확정됨
}

/* Status Register 읽기 */
static uint8_t EEPROM_ReadStatus(void)
{
    uint8_t sr = 0;

    EEPROM_CS_Low();
    (void)SPI_TxRxByte((uint8_t)EEPROM_CMD_RDSR);
    sr = SPI_TxRxByte(0xFF); // dummy write로 status read
    EEPROM_CS_High();

    return sr;
}

/* Write 완료 대기(WIP=0 될 때까지) */
static bool EEPROM_WaitWriteComplete(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    while (1)
    {
        uint8_t sr = EEPROM_ReadStatus();

        /* WIP가 0이면 쓰기 완료 */
        if ((sr & EEPROM_SR_WIP) == 0u)
            return true;

        if ((HAL_GetTick() - start) > timeout_ms)
            return false;
    }
}

void EEPROM_Init(void)
{
    /* 과제 단계에선 특별히 할 건 없지만,
       CS는 기본 High(비선택) 상태여야 함 */
    EEPROM_CS_High();
}

/* 실제 WRITE 명령(페이지 단위로 안전하게 쓰기 위해 내부에서 쪼개어 호출) */
static bool EEPROM_WritePage(uint16_t addr, const uint8_t *data, uint16_t len)
{
    /* len은 반드시 페이지 경계를 넘지 않도록 호출측에서 보장 */
    if (data == NULL || len == 0) return false;

    EEPROM_WriteEnable();

    EEPROM_CS_Low();

    /* 1) WRITE opcode */
    (void)SPI_TxRxByte((uint8_t)EEPROM_CMD_WRITE);

    /* 2) Address: 16-bit (상위바이트 먼저 전송) */
    (void)SPI_TxRxByte((uint8_t)((addr >> 8) & 0xFF));
    (void)SPI_TxRxByte((uint8_t)(addr & 0xFF));

    /* 3) Data bytes 전송 */
    (void)HAL_SPI_Transmit(&hspi1, (uint8_t*)data, len, 100u);

    /* 4) CS High: 이 순간에 내부 write 사이클이 시작됨 */
    EEPROM_CS_High();

    /* 5) WIP가 내려갈 때까지 대기 */
    return EEPROM_WaitWriteComplete(500u);
}

bool EEPROM_WriteBytes(uint16_t addr, const uint8_t *data, uint16_t len)
{
    if (data == NULL) return false;
    if (len == 0) return true;

    /* 주소 범위 체크 */
    if ((uint32_t)addr + (uint32_t)len > EEPROM_SIZE_BYTES)
        return false;

    /* 페이지 경계 고려해서 쪼개 쓰기 */
    uint16_t remaining = len;
    uint16_t cur_addr = addr;
    const uint8_t *cur_data = data;

    while (remaining > 0)
    {
        /* 현재 주소가 속한 페이지의 남은 공간 계산 */
        uint16_t page_offset = (uint16_t)(cur_addr % EEPROM_PAGE_SIZE);
        uint16_t page_space  = (uint16_t)(EEPROM_PAGE_SIZE - page_offset);

        /* 이번에 쓸 길이: 페이지 공간과 remaining 중 작은 값 */
        uint16_t chunk = (remaining < page_space) ? remaining : page_space;

        if (!EEPROM_WritePage(cur_addr, cur_data, chunk))
            return false;

        /* 다음 chunk로 이동 */
        cur_addr = (uint16_t)(cur_addr + chunk);
        cur_data = (const uint8_t*)(cur_data + chunk);
        remaining = (uint16_t)(remaining - chunk);
    }

    return true;
}

bool EEPROM_ReadBytes(uint16_t addr, uint8_t *data, uint16_t len)
{
    if (data == NULL) return false;
    if (len == 0) return true;

    /* 주소 범위 체크 */
    if ((uint32_t)addr + (uint32_t)len > EEPROM_SIZE_BYTES)
        return false;

    EEPROM_CS_Low();

    /* 1) READ opcode */
    (void)SPI_TxRxByte((uint8_t)EEPROM_CMD_READ);

    /* 2) Address 16-bit */
    (void)SPI_TxRxByte((uint8_t)((addr >> 8) & 0xFF));
    (void)SPI_TxRxByte((uint8_t)(addr & 0xFF));

    /* 3) Data 수신 */
    (void)HAL_SPI_Receive(&hspi1, data, len, 100u);

    EEPROM_CS_High();

    return true;
}
