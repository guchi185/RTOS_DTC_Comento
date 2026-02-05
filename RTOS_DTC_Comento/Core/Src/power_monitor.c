#include "power_monitor.h"
#include "pmic_mp5475.h"

/**
 * @brief 전원 감시 Task (과제 1~2단계: PMIC 상태 읽고 UV 여부 판단)
 */
void PowerMonitor_Task(void)
{
    /* PMIC_IsUV()는 pmic_mp5475.c에 구현되어 있어야 한다 */
    bool uv = PMIC_IsUV();

    (void)uv; // 지금은 사용 안 하니까 경고 방지
}
