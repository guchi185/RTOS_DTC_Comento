#ifndef EEPROM_25LC256_H
#define EEPROM_25LC256_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * 25LC256 (SPI EEPROM) 기본 개념
 * - WRITE 전에 반드시 WREN(Write Enable) 필요
 * - Write 진행 중에는 WIP(Status Register bit)로 완료 확인 가능
 * - Page write(보통 64바이트 단위)를 넘기면 자동 wrap될 수 있어
 *   → 안전하게는 page boundary를 지켜서 나눠 쓰는게 좋음
 * ============================================================ */

/* ============================================================
 * 25LC256 (SPI EEPROM) - Command/Status 정의
 * [피드백 반영] Command는 enum이 아니라 매크로(#define)로 정의
 * ============================================================ */

/* 25LC256 명령어(opcode) - macro */
#define EEPROM_CMD_READ   (0x03u)
#define EEPROM_CMD_WRITE  (0x02u)
#define EEPROM_CMD_WREN   (0x06u)
#define EEPROM_CMD_RDSR   (0x05u)
#define EEPROM_CMD_WRSR   (0x01u)

/* Status Register bit */
#define EEPROM_SR_WIP     (1u << 0)  /* Write-In-Progress */
#define EEPROM_SR_WEL     (1u << 1)  /* Write Enable Latch */

/* 메모리/페이지 관련 */
#define EEPROM_SIZE_BYTES        (32768u)
#define EEPROM_PAGE_SIZE         (64u)

/* 하드코딩 줄이기용 공통 상수 */
#define EEPROM_SPI_TIMEOUT_MS    (100u)
#define EEPROM_WRITE_TIMEOUT_MS  (500u)
#define EEPROM_DUMMY_BYTE        (0xFFu)

void EEPROM_Init(void);
bool EEPROM_WriteBytes(uint16_t addr, const uint8_t *data, uint16_t len);
bool EEPROM_ReadBytes(uint16_t addr, uint8_t *data, uint16_t len);

#endif /* EEPROM_25LC256_H */
