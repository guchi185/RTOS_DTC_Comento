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

/* 25LC256 명령어(opcode) */
typedef enum
{
    EEPROM_CMD_READ  = 0x03,
    EEPROM_CMD_WRITE = 0x02,
    EEPROM_CMD_WREN  = 0x06,
    EEPROM_CMD_RDSR  = 0x05,
    EEPROM_CMD_WRSR  = 0x01
} EEPROM_Cmd_t;

/* Status Register 비트(일반적으로) */
#define EEPROM_SR_WIP   (1u << 0)  // Write-In-Progress
#define EEPROM_SR_WEL   (1u << 1)  // Write Enable Latch

/* 메모리/페이지 관련(25LC256은 32KB = 0x0000~0x7FFF) */
#define EEPROM_SIZE_BYTES      (32768u)
#define EEPROM_PAGE_SIZE       (64u)

/* ============================================================
 * API
 * ============================================================ */

/**
 * @brief  EEPROM 초기화(과제 단계: 실질 내용 없어도 됨)
 * @note   CS 핀 초기 상태 High 유지가 중요(선택 해제)
 */
void EEPROM_Init(void);

/**
 * @brief  EEPROM에 데이터 쓰기(내부에서 WREN + page split + WIP 대기)
 * @param  addr 시작 주소(0x0000~0x7FFF)
 * @param  data 쓸 데이터 포인터
 * @param  len  길이(바이트)
 * @return true 성공, false 실패
 */
bool EEPROM_WriteBytes(uint16_t addr, const uint8_t *data, uint16_t len);

/**
 * @brief  EEPROM에서 데이터 읽기
 * @param  addr 시작 주소
 * @param  data 읽어서 담을 버퍼
 * @param  len  길이
 * @return true 성공, false 실패
 */
bool EEPROM_ReadBytes(uint16_t addr, uint8_t *data, uint16_t len);

#endif
