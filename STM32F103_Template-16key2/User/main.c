#include "stm32f10x.h"
#include "delay.h"
#include "lcd.h"
#include "IOput.h"
#include "timer.h"
#include "usart.h"
#include "TX510.h"
#include "ds1302.h"
#include "SR501.h"

#include <stdio.h>
#include <string.h>

// 设备ID
#define DEVICE_ID          "02ED"

// ==================== 常量定义 ====================
#define PASSWORD_LEN                6
#define VERIFY_TIMEOUT              0
#define VERIFY_SUCCESS              1
#define VERIFY_DENY                 2

#define OPEN_SRC_NONE               0
#define OPEN_SRC_FACE               1
#define OPEN_SRC_PWD                2

#define LINGER_THRESHOLD_DEFAULT_S  30
#define DOOR_OPEN_MS                3000
#define FACE_WAIT_MS                5000
#define ADMIN_KEEP_MS               30000
#define ALARM_HOLD_MS               2000
#define UI_REFRESH_MS               200
#define RTC_REFRESH_MS              200
#define CLOUD_REPORT_MS             2000
#define FACE_FAIL_INTERVAL_MS       1500

extern u16 time_count;

// ==================== 状态定义 ====================
typedef enum {
    UI_PAGE_IDLE = 0,      // 空闲主页
    UI_PAGE_ADMIN,         // 管理菜单
    UI_PAGE_ADMIN_TIME,    // 管理-时间设置
    UI_PAGE_ADMIN_PWD,     // 管理-密码修改
    UI_PAGE_ADMIN_FACE_ADD, // 管理-人脸录入
    UI_PAGE_ADMIN_FACE_DEL, // 管理-人脸删除
    UI_PAGE_ADD,           // 人脸录入中
    UI_PAGE_DELETE,        // 人脸删除中
    UI_PAGE_PWD_INPUT      // 密码输入中
} ui_page_t;

// ==================== 静态变量 ====================
static u8 Code[PASSWORD_LEN] = {1, 2, 3, 4, 5, 6};
static DS1302_TIME RtcTime;

static u8 door_state = 0;
static u8 door_open_source = OPEN_SRC_NONE;
static ui_page_t ui_page = UI_PAGE_IDLE;
static u8 pir_state = 0;
static u8 pir_last_state = 0;
static u8 face_state = K210_FACE_NONE;
static u8 face_id = 0xFF;
static u8 face_total = 0;
static u8 face_fail_count = 0;
static u8 password_fail_count = 0;
static u8 admin_verified = 0;
static u8 admin_registered = 0;
static u8 linger_triggered = 0;
static u8 alarm_active = 0;
static u8 unlock_latched = 0;
static u8 linger_threshold_s = LINGER_THRESHOLD_DEFAULT_S;
static u8 oled_page_index = 0;
static u8 alarm_reason = 0;
static u8 op_face_result = 0;
static u8 op_pwd_result = 0;
static u8 op_face_manage = 0;

static u16 door_open_tick = 0;
static u16 pir_start_tick = 0;
static u16 last_ui_tick = 0;
static u16 last_rtc_tick = 0;
static u16 last_cloud_tick = 0;
static u16 last_face_fail_tick = 0;
static u16 admin_verify_tick = 0;
static u16 alarm_start_tick = 0;

static char last_open_stamp[20] = "00-00-00 00:00:00";
static char last_linger_stamp[20] = "00-00-00 00:00:00";
static char last_alarm_stamp[20] = "00-00-00 00:00:00";

static char oled_line_cache[4][40];
static u8 oled_line_valid[4] = {0};

#define ALARM_REASON_NONE        0
#define ALARM_REASON_PASSWORD    1
#define ALARM_REASON_FACE        2
#define ALARM_REASON_LINGER      3

#define OP_RESULT_NONE           0
#define OP_RESULT_SUCCESS        1
#define OP_RESULT_FAIL           2

#define OP_FACE_MNG_NONE         0
#define OP_FACE_MNG_ADD          1
#define OP_FACE_MNG_DEL          2

static u8 OLED_LineSlot(u8 y)
{
    if(y == 0) return 0;
    if(y == 2) return 1;
    if(y == 4) return 2;
    if(y == 6) return 3;
    return 0xFF;
}

static void OLED_InvalidateCache(void)
{
    memset(oled_line_valid, 0, sizeof(oled_line_valid));
}

static void OLED_FormatLine(const u8 *text, char *out, u8 out_len)
{
    u8 width = 0;
    u8 idx = 0;

    if(out_len == 0)
    {
        return;
    }

    while(*text != '\0' && idx < (u8)(out_len - 1))
    {
        if(*text > 0xA0)
        {
            if((width + 16) > 128 || idx + 2 >= out_len)
            {
                break;
            }
            out[idx++] = *text++;
            out[idx++] = *text++;
            width += 16;
        }
        else
        {
            if((width + 8) > 128)
            {
                break;
            }
            out[idx++] = *text++;
            width += 8;
        }
    }

    while((width + 8) <= 128 && idx < (u8)(out_len - 1))
    {
        out[idx++] = ' ';
        width += 8;
    }
    out[idx] = '\0';
}

static void OLED_ClearPage(void)
{
    OLED_Clear();
    OLED_InvalidateCache();
}

static void OLED_ShowLineCH(u8 y, u8 *text)
{
    char line[40];
    u8 slot;

    OLED_FormatLine(text, line, sizeof(line));
    slot = OLED_LineSlot(y);
    if(slot < 4)
    {
        if(oled_line_valid[slot] && strcmp(oled_line_cache[slot], line) == 0)
        {
            return;
        }
        strncpy(oled_line_cache[slot], line, sizeof(oled_line_cache[slot]) - 1);
        oled_line_cache[slot][sizeof(oled_line_cache[slot]) - 1] = '\0';
        oled_line_valid[slot] = 1;
    }

    OLED_ShowCH(0, y, (u8 *)line);
}

static void OLED_ShowLineASCII(u8 y, const char *text)
{
    OLED_ShowLineCH(y, (u8 *)text);
}

static void BuildStamp(char *buf, const DS1302_TIME *time)
{
    if(buf == 0 || time == 0)
    {
        return;
    }

    snprintf(buf, 20, "%02d-%02d-%02d %02d:%02d:%02d",
             time->year,
             time->month,
             time->day,
             time->hour,
             time->minute,
             time->second);
}

static void SaveCurrentStamp(char *buf)
{
    BuildStamp(buf, &RtcTime);
}

// ==================== 蜂鸣器(报警)控制 ====================
static void Alarm_Set(u8 on)
{
    if(on)
    {
        ALARM_IO = 0;
        alarm_active = 1;
        alarm_start_tick = time_count;
    }
    else
    {
        ALARM_IO = 1;
        alarm_active = 0;
    }
}

static void TriggerAlarm(void)
{
    SaveCurrentStamp(last_alarm_stamp);
    Alarm_Set(1);
}

static void Door_Set(u8 open)
{
    if(open)
    {
        DOOR_LOCK = 0;
        door_state = 1;
        door_open_tick = time_count;
        SaveCurrentStamp(last_open_stamp);
    }
    else
    {
        DOOR_LOCK = 1;
        door_state = 0;
        door_open_source = OPEN_SRC_NONE;
    }
}

static void Door_OpenBy(u8 source)
{
    door_open_source = source;
    Door_Set(1);
}

static u8 Door_OpenAllowed(void)
{
    if(ui_page == UI_PAGE_ADD || ui_page == UI_PAGE_DELETE)
    {
        return 0;
    }
    return 1;
}

static u8 DaysInMonth(u8 year, u8 month)
{
    if(month == 2)
    {
        u16 full_year = (u16)(2000 + year);
        if((full_year % 400 == 0) || ((full_year % 4 == 0) && (full_year % 100 != 0)))
        {
            return 29;
        }
        return 28;
    }

    if(month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12)
    {
        return 31;
    }

    return 30;
}

static void ClampTime(DS1302_TIME *time)
{
    u8 max_day;

    if(time == 0)
    {
        return;
    }

    if(time->month < 1)
    {
        time->month = 12;
    }
    if(time->month > 12)
    {
        time->month = 1;
    }

    max_day = DaysInMonth(time->year, time->month);
    if(time->day < 1)
    {
        time->day = max_day;
    }
    if(time->day > max_day)
    {
        time->day = 1;
    }

    if(time->hour > 23)
    {
        time->hour = 0;
    }
    if(time->minute > 59)
    {
        time->minute = 0;
    }
    if(time->second > 59)
    {
        time->second = 0;
    }
    if(time->week < 1 || time->week > 7)
    {
        time->week = 1;
    }
}

static void ShowTimedMessage(u8 *line2, u8 *line4, u16 hold_ms)
{
    OLED_ClearPage();
    OLED_ShowCH(0, 0, "   智能门禁 ");
    if(line2 != 0)
    {
        OLED_ShowCH(0, 2, line2);
    }
    if(line4 != 0)
    {
        OLED_ShowCH(0, 4, line4);
    }
    if(hold_ms > 0)
    {
        delay_ms(hold_ms);
    }
    OLED_ClearPage();
}

static void RTC_Task(void)
{
    if((u16)(time_count - last_rtc_tick) >= RTC_REFRESH_MS)
    {
        last_rtc_tick = time_count;
        DS1302_ReadTime(&RtcTime);
    }
}

static u16 GetLingerSeconds(void)
{
    if(!pir_state)
    {
        return 0;
    }

    return (u16)((u16)(time_count - pir_start_tick) / 1000);
}

static void PIR_Task(void)
{
    pir_state = (HC_SR501_Status() == ENABLE) ? 1 : 0;

    if(pir_state)
    {
        if(!pir_last_state)
        {
            pir_start_tick = time_count;
            linger_triggered = 0;
        }

        if(!linger_triggered && GetLingerSeconds() >= linger_threshold_s)
        {
            linger_triggered = 1;
            SaveCurrentStamp(last_linger_stamp);
            alarm_reason = ALARM_REASON_LINGER;
            TriggerAlarm();
        }
    }
    else
    {
        linger_triggered = 0;
        unlock_latched = 0;
    }

    pir_last_state = pir_state;
}

static void Door_Task(void)
{
    if(door_state && (u16)(time_count - door_open_tick) >= DOOR_OPEN_MS)
    {
        Door_Set(0);
    }
}

static void Alarm_Task(void)
{
    if(alarm_active && (u16)(time_count - alarm_start_tick) >= ALARM_HOLD_MS)
    {
        Alarm_Set(0);
        face_fail_count = 0;
        password_fail_count = 0;
    }
}

static void Cloud_ReportTask(void)
{
    char now_stamp[20];

    if((u16)(time_count - last_cloud_tick) < CLOUD_REPORT_MS)
    {
        return;
    }

    last_cloud_tick = time_count;
    BuildStamp(now_stamp, &RtcTime);
    UsartPrintf(USART1,
                "@S,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s\r\n",
                door_state,
                pir_state,
                linger_triggered,
                GetLingerSeconds(),
                admin_verified,
                face_state,
                face_fail_count,
                alarm_active,
                face_total,
                now_stamp);
}

static void Face_BackgroundTask(void)
{
    u8 state;
    u8 id;
    u8 total;
    u8 op;
    u8 result;
    u8 slot;

    while(K210_FetchFaceUpdate(&state, &id, &total))
    {
        face_state = state;
        face_id = id;
        face_total = total;
        admin_registered = (face_total > 0) ? 1 : 0;

        if(state == K210_FACE_NONE)
        {
            unlock_latched = 0;
        }
        else if(state == K210_FACE_KNOWN)
        {
            face_fail_count = 0;
            op_face_result = OP_RESULT_SUCCESS;
            if(!door_state && !unlock_latched && !alarm_active && Door_OpenAllowed())
            {
                Door_OpenBy(OPEN_SRC_FACE);
                unlock_latched = 1;
            }
        }
        else if(state == K210_FACE_UNKNOWN)
        {
            op_face_result = OP_RESULT_FAIL;
            if(!alarm_active)
            {
                if((u16)(time_count - last_face_fail_tick) >= FACE_FAIL_INTERVAL_MS)
                {
                    last_face_fail_tick = time_count;
                    if(face_fail_count < 3)
                    {
                        face_fail_count++;
                    }
                    if(face_fail_count >= 3)
                    {
                        alarm_reason = ALARM_REASON_FACE;
                        TriggerAlarm();
                    }
                }
            }
        }
    }

    while(K210_FetchOpResult(&op, &result, &slot, &total))
    {
        face_total = total;
        admin_registered = (face_total > 0) ? 1 : 0;
        (void)op;
        (void)result;
        (void)slot;
    }
}

// ==================== 辅助函数 ====================
// 生成星号字符串
static const char* StringOfStars(u8 count)
{
    static char stars[7] = "******";
    if(count > 6) count = 6;
    stars[count] = '\0';
    return stars;
}

// ==================== 显示界面 ====================
static void ShowIdlePage(void)
{
    char line0[20];
    char line2[24];
    char line4[24];
    char line6[32];

    snprintf(line0, sizeof(line0), "%02d-%02d-%02d%02d:%02d:%02d",
                         RtcTime.year,
             RtcTime.month,
             RtcTime.day,
             RtcTime.hour,
             RtcTime.minute,RtcTime.second);

    if(oled_page_index == 0)
    {
        OLED_ShowLineASCII(0, line0);
        if(pir_state)
        {
            snprintf(line2, sizeof(line2), "红外状态:有人 ");
        }
        else
        {
            snprintf(line2, sizeof(line2), "红外状态:无人 ");
        }
        OLED_ShowLineASCII(2, line2);
        OLED_ShowLineASCII(4, "A切换");
        OLED_ShowLineASCII(6, "D注册人脸");
    }
    else if(oled_page_index == 1)
    {
        OLED_ShowLineASCII(0, line0);
        OLED_ShowLineASCII(2, "页面2");
        OLED_ShowLineASCII(4, "A切换");
        OLED_ShowLineASCII(6, "D修改密码");
    }
        else if(oled_page_index == 2)
    {
        u8 show_reason = ALARM_REASON_NONE;
        u8 pwd_alarm = (password_fail_count >= 3) ? 1 : 0;
        u8 face_alarm = (face_fail_count >= 3) ? 1 : 0;
        u8 linger_alarm = (linger_triggered && (GetLingerSeconds() >= linger_threshold_s)) ? 1 : 0;

        if(alarm_active)
        {
            if(alarm_reason == ALARM_REASON_PASSWORD) pwd_alarm = 1;
            else if(alarm_reason == ALARM_REASON_FACE) face_alarm = 1;
            else if(alarm_reason == ALARM_REASON_LINGER) linger_alarm = 1;
        }

        if(pwd_alarm)
        {
            show_reason = ALARM_REASON_PASSWORD;
        }
        else if(face_alarm)
        {
            show_reason = ALARM_REASON_FACE;
        }
        else if(linger_alarm)
        {
            show_reason = ALARM_REASON_LINGER;
        }

        OLED_ShowLineASCII(0, "系统状态");
        if(show_reason == ALARM_REASON_PASSWORD)
        {
            OLED_ShowLineASCII(2, "报警:密码错误");
        }
        else if(show_reason == ALARM_REASON_FACE)
        {
            OLED_ShowLineASCII(2, "报警:人脸识别失败");
        }
        else if(show_reason == ALARM_REASON_LINGER)
        {
            OLED_ShowLineASCII(2, "报警:逗留超时");
        }
        else
        {
            OLED_ShowLineASCII(2, "无报警");
        }
        OLED_ShowLineASCII(4, " ");
        OLED_ShowLineASCII(6, "A切换");
    }
    else
    {
        OLED_ShowLineASCII(0, line0);

        if(op_face_result == OP_RESULT_SUCCESS) snprintf(line2, sizeof(line2), "人脸识别:成功");
        else if(op_face_result == OP_RESULT_FAIL) snprintf(line2, sizeof(line2), "人脸识别:失败");
        else snprintf(line2, sizeof(line2), "人脸识别:无");

        snprintf(line4, sizeof(line4), "密码失败:%d次", password_fail_count);

        if(op_face_manage == OP_FACE_MNG_ADD)
        {
            snprintf(line6, sizeof(line6), "逗留:%ds", (int)GetLingerSeconds());
        }
        else if(op_face_manage == OP_FACE_MNG_DEL)
        {
            snprintf(line6, sizeof(line6), "逗留:%ds", (int)GetLingerSeconds());
        }
        else
        {
            snprintf(line6, sizeof(line6), "逗留:%ds", (int)GetLingerSeconds());
        }

        OLED_ShowLineASCII(2, line2);
        OLED_ShowLineASCII(4, line4);
        OLED_ShowLineASCII(6, line6);
    }
}
static void ShowAdminPage(void)
{
    OLED_ClearPage();
    OLED_ShowCH(0, 0, "   管理菜单 ");
    
    switch(ui_page)
    {
        case UI_PAGE_ADMIN:
            OLED_ShowCH(0, 2, "   B录入 C删除 ");
            OLED_ShowCH(0, 6, "  *退出 ");
            break;
        case UI_PAGE_ADMIN_TIME:
            OLED_ShowCH(0, 2, "   >时间设置 ");
            OLED_ShowCH(0, 4, "   A密码修改 ");
            OLED_ShowCH(0, 6, "  #修改 *退出 ");
            break;
        case UI_PAGE_ADMIN_PWD:
            OLED_ShowCH(0, 2, "   A时间设置 ");
            OLED_ShowCH(0, 4, "   >密码修改 ");
            OLED_ShowCH(0, 6, "  #修改 *退出 ");
            break;
        default:
            break;
    }
}

static void ShowFaceManagePage(void)
{
    char line[17];
    
    OLED_ClearPage();
    OLED_ShowCH(0, 0, "   人脸管理 ");
    OLED_ShowCH(0, 2, "   B录入 C删除 ");
    snprintf(line, sizeof(line), "  人脸数:%d ", face_total);
    OLED_ShowLineASCII(4, line);
    OLED_ShowCH(0, 6, "*退出 ");
}

static void Password_DrawMask(u8 count, u8 blink)
{
    char mask[] = "PWD:______";
    u8 i;

    for(i = 0; i < count && i < PASSWORD_LEN; i++)
    {
        mask[4 + i] = '*';
    }

    if(count < PASSWORD_LEN && blink)
    {
        mask[4 + count] = ' ';
    }

    OLED_ShowLineASCII(6, mask);
}

static u8 Password_InputToBuffer(u8 *line2, u8 *out_buf)
{
    u8 count = 0;
    u8 blink = 0;
    u16 blink_tick = time_count;

    memset(out_buf, 0, PASSWORD_LEN);

    OLED_ClearPage();
    OLED_ShowCH(0, 0, "   输入密码 ");
    OLED_ShowCH(0, 2, line2);
    OLED_ShowCH(0, 4, "  *退出  #确认 ");
    Password_DrawMask(count, blink);

    while(1)
    {
        key_num = key_scan(1);

        if((u16)(time_count - blink_tick) >= 250)
        {
            blink_tick = time_count;
            blink = !blink;
            Password_DrawMask(count, blink);
        }

        if(key_num <= KEY_9)
        {
            if(count < PASSWORD_LEN)
            {
                out_buf[count++] = key_num;
                Password_DrawMask(count, 0);
            }
        }
        else if(key_num == KEY_Asterisk)
        {
            OLED_ClearPage();
            return VERIFY_TIMEOUT;
        }
        else if(key_num == KEY_Hashtag)
        {
            if(count == PASSWORD_LEN)
            {
                OLED_ClearPage();
                return VERIFY_SUCCESS;
            }
        }
    }
}

static u8 Password_VerifyLine(u8 *line2)
{
    u8 in_code[PASSWORD_LEN];

    if(Password_InputToBuffer(line2, in_code) != VERIFY_SUCCESS)
    {
        return VERIFY_TIMEOUT;
    }

    if(memcmp(in_code, Code, PASSWORD_LEN) == 0)
    {
        password_fail_count = 0;
        op_pwd_result = OP_RESULT_SUCCESS;
        return VERIFY_SUCCESS;
    }

    op_pwd_result = OP_RESULT_FAIL;

    if(password_fail_count < 3)
    {
        password_fail_count++;
    }
    if(password_fail_count >= 3)
    {
        alarm_reason = ALARM_REASON_PASSWORD;
        TriggerAlarm();
    }

    return VERIFY_DENY;
}

static u8 Password_VerifyCurrent(void)
{
    return Password_VerifyLine((u8 *)"   请输入密码 ");
}

static void PasswordChange_Process(void);

static void PasswordModifyWithOld_Process(void)
{
    u8 state;

    state = Password_VerifyLine((u8 *)"   请输入原密码 ");
    if(state == VERIFY_SUCCESS)
    {
        PasswordChange_Process();
        return;
    }

    if(state == VERIFY_DENY)
    {
        ShowTimedMessage((u8 *)"   原密码错误 ", 0, 800);
    }
}

static u8 WaitAdminFace(void)
{
    u16 start_tick;
    u8 state;
    u8 id;
    u8 total;

    K210_ClearFaceUpdate();
    OLED_ClearPage();
    OLED_ShowCH(0, 0, "   输入密码 ");
    OLED_ShowCH(0, 2, "   人脸识别中 ");
    OLED_ShowCH(0, 4, "    请稍候中 ");

    start_tick = time_count;
    while((u16)(time_count - start_tick) < FACE_WAIT_MS)
    {
        if(K210_FetchFaceUpdate(&state, &id, &total))
        {
            face_state = state;
            face_id = id;
            face_total = total;
            admin_registered = (face_total > 0) ? 1 : 0;

            if(state == K210_FACE_KNOWN)
            {
                OLED_ClearPage();
                if(id == 0)
                {
                    return VERIFY_SUCCESS;
                }
                return VERIFY_DENY;
            }
        }
        delay_ms(20);
    }

    OLED_ClearPage();
    return VERIFY_TIMEOUT;
}

static u8 WaitK210Register(u8 slot, u8 *actual_slot)
{
    u16 start_tick;
    u8 op;
    u8 result;
    u8 temp_slot;
    u8 total;

    K210_ClearOpResult();
    K210_SendRegister(slot);

    start_tick = time_count;
    while((u16)(time_count - start_tick) < FACE_WAIT_MS)
    {
        if(K210_FetchOpResult(&op, &result, &temp_slot, &total))
        {
            face_total = total;
            admin_registered = (face_total > 0) ? 1 : 0;
            if(op == K210_OP_REGISTER)
            {
                if(result)
                {
                    if(actual_slot != 0)
                    {
                        *actual_slot = temp_slot;
                    }
                    return VERIFY_SUCCESS;
                }
                return VERIFY_DENY;
            }
        }
        delay_ms(20);
    }

    return VERIFY_TIMEOUT;
}

static u8 WaitK210Delete(u8 slot)
{
    u16 start_tick;
    u8 op;
    u8 result;
    u8 temp_slot;
    u8 total;

    K210_ClearOpResult();
    K210_SendDelete(slot);

    start_tick = time_count;
    while((u16)(time_count - start_tick) < FACE_WAIT_MS)
    {
        if(K210_FetchOpResult(&op, &result, &temp_slot, &total))
        {
            face_total = total;
            admin_registered = (face_total > 0) ? 1 : 0;
            if(op == K210_OP_DELETE && temp_slot == slot)
            {
                if(result)
                {
                    return VERIFY_SUCCESS;
                }
                return VERIFY_DENY;
            }
        }
        delay_ms(20);
    }

    return VERIFY_TIMEOUT;
}

static void StartAdminEnroll(void)
{
    u8 actual_slot = 0xFF;
    u8 state;

    OLED_ClearPage();
    OLED_ShowCH(0, 0, "   注册人脸 ");
    OLED_ShowCH(0, 2, "   人脸识别中 ");
    OLED_ShowCH(0, 4, "    请稍候中 ");

    state = WaitK210Register(0, &actual_slot);
    if(state == VERIFY_SUCCESS && actual_slot == 0)
    {
        admin_verified = 1;
        admin_registered = 1;
        admin_verify_tick = time_count;
        ShowTimedMessage((u8 *)"   注册成功 ", (u8 *)"   保存人脸 ", 800);
    }
    else if(state == VERIFY_DENY)
    {
        ShowTimedMessage((u8 *)"   注册失败 ", 0, 800);
    }
    else
    {
        ShowTimedMessage((u8 *)"    超时 ", 0, 800);
    }
}

// ==================== 管理认证流程 ====================
// D键 → 输入密码 → 验证通过 → 进入管理菜单
static void AdminAuth_Process(void)
{
    u8 state;
    
    ui_page = UI_PAGE_PWD_INPUT;
    state = Password_VerifyCurrent();
    ui_page = UI_PAGE_IDLE;
    
    if(state == VERIFY_SUCCESS)
    {
        face_fail_count = 0;
        password_fail_count = 0;
        admin_verified = 1;
        admin_verify_tick = time_count;
        ui_page = UI_PAGE_ADMIN;
        ShowTimedMessage((u8 *)"   认证成功 ", 0, 600);
    }
    else if(state == VERIFY_DENY)
    {
        ShowTimedMessage((u8 *)"   密码错误 ", 0, 800);
    }
}

// 管理菜单内按A键切换选项
static void AdminMenu_Switch(void)
{
    if(ui_page == UI_PAGE_ADMIN)
    {
        ui_page = UI_PAGE_ADMIN_TIME;  // 默认进入时间设置
        ShowTimedMessage((u8 *)"   时间设置 ", 0, 600);
    }
    else if(ui_page == UI_PAGE_ADMIN_TIME)
    {
        ui_page = UI_PAGE_ADMIN_PWD;
        ShowTimedMessage((u8 *)"   密码修改 ", 0, 600);
    }
    else if(ui_page == UI_PAGE_ADMIN_PWD)
    {
        ui_page = UI_PAGE_ADMIN_TIME;
        ShowTimedMessage((u8 *)"   时间设置 ", 0, 600);
    }
}

// B键进入人脸录入
static void FaceRegister_Process(void)
{
    u8 actual_slot = 0xFF;
    u8 state;
    char line[17];

    if(!admin_verified)
    {
        ShowTimedMessage((u8 *)"   请先认证 ", 0, 800);
        return;
    }

    ui_page = UI_PAGE_ADD;
    OLED_ClearPage();
    OLED_ShowCH(0, 0, "   注册人脸 ");
    OLED_ShowCH(0, 2, "   人脸识别中 ");
    OLED_ShowCH(0, 4, "    请稍候中 ");

    state = WaitK210Register(0xFF, &actual_slot);
    if(state == VERIFY_SUCCESS)
    {
        op_face_manage = OP_FACE_MNG_ADD;
        ShowTimedMessage((u8 *)"   注册成功 ", (u8 *)"   保存人脸 ", 600);
        snprintf(line, sizeof(line), "ID:%02d", actual_slot);
        OLED_ClearPage();
        OLED_ShowCH(0, 0, "   注册人脸 ");
        OLED_ShowLineASCII(4, line);
        delay_ms(600);
        OLED_ClearPage();
    }
    else if(state == VERIFY_DENY)
    {
        ShowTimedMessage((u8 *)"   注册失败 ", 0, 800);
    }
    else
    {
        ShowTimedMessage((u8 *)"    超时 ", 0, 800);
    }
    ui_page = UI_PAGE_ADMIN;
}

// C键进入人脸删除
static void FaceDelete_Process(void)
{
    u8 face_slot = 0;
    u8 state;
    char line[17];

    if(!admin_verified)
    {
        ShowTimedMessage((u8 *)"   请先认证 ", 0, 800);
        return;
    }

    ui_page = UI_PAGE_DELETE;
    OLED_ClearPage();
    while(1)
    {
        OLED_ShowCH(0, 0, "   删除人脸 ");
        OLED_ShowCH(0, 2, "   B加  C减 ");
        OLED_ShowCH(0, 4, "  D确认 *退出 ");
        snprintf(line, sizeof(line), "ID:%02d", face_slot);
        OLED_ShowLineASCII(6, line);

        key_num = key_scan(1);
        if(key_num == KEY_ADD)
        {
            if(face_slot < 9)
            {
                face_slot++;
            }
        }
        else if(key_num == KEY_DEC)
        {
            if(face_slot > 0)
            {
                face_slot--;
            }
        }
        else if(key_num == KEY_Asterisk)
        {
            OLED_ClearPage();
            ui_page = UI_PAGE_IDLE;
            return;
        }
        else if(key_num == KEY_SAVE)
        {
            OLED_ClearPage();
            OLED_ShowCH(0, 0, "   删除人脸 ");
            OLED_ShowCH(0, 2, "   人脸识别中 ");
            OLED_ShowCH(0, 4, "    请稍候中 ");
            state = WaitK210Delete(face_slot);
            if(state == VERIFY_SUCCESS)
            {
                op_face_manage = OP_FACE_MNG_DEL;
                ShowTimedMessage((u8 *)"   删除成功 ", 0, 800);
            }
            else if(state == VERIFY_DENY)
            {
                ShowTimedMessage((u8 *)"   删除失败 ", 0, 800);
            }
            else
            {
                ShowTimedMessage((u8 *)"    超时 ", 0, 800);
            }
            ui_page = UI_PAGE_IDLE;
            return;
        }
    }
}

// #键开门
static void DoorOpen_Process(void);

static void DoorOpenByPassword_Process(void)
{
    u8 state;

    state = Password_VerifyCurrent();
    if(state == VERIFY_SUCCESS)
    {
        DoorOpen_Process();
    }
    else if(state == VERIFY_DENY)
    {
        ShowTimedMessage((u8 *)"   密码错误 ", 0, 800);
    }
}
static void DoorOpen_Process(void)
{
    if(!Door_OpenAllowed())
    {
        return;
    }
    
    if(alarm_active)
    {
        Alarm_Set(0);
    }
    face_fail_count = 0;
    password_fail_count = 0;
    Door_OpenBy(OPEN_SRC_PWD);
    ShowTimedMessage((u8 *)"   开门成功 ", 0, 600);
}



static void TimeEdit_Draw(const DS1302_TIME *edit, u8 field, u8 blink)
{
    char year[3];
    char month[3];
    char day[3];
    char hour[3];
    char minute[3];
    char second[3];
    char line2[17];
    char line4[17];

    snprintf(year, sizeof(year), "%02d", edit->year);
    snprintf(month, sizeof(month), "%02d", edit->month);
    snprintf(day, sizeof(day), "%02d", edit->day);
    snprintf(hour, sizeof(hour), "%02d", edit->hour);
    snprintf(minute, sizeof(minute), "%02d", edit->minute);
    snprintf(second, sizeof(second), "%02d", edit->second);

    if(blink)
    {
        if(field == 0) strcpy(year, "  ");
        else if(field == 1) strcpy(month, "  ");
        else if(field == 2) strcpy(day, "  ");
        else if(field == 3) strcpy(hour, "  ");
        else if(field == 4) strcpy(minute, "  ");
        else if(field == 5) strcpy(second, "  ");
    }

    snprintf(line2, sizeof(line2), "20%s-%s-%s", year, month, day);
    snprintf(line4, sizeof(line4), "%s:%s:%s", hour, minute, second);

    OLED_ShowCH(0, 0, " A切换 B加 C减 ");
    OLED_ShowLineASCII(2, line2);
    OLED_ShowLineASCII(4, line4);
    OLED_ShowCH(0, 6, "  D确认 *退出 ");
}

static void TimeEdit_Inc(DS1302_TIME *edit, u8 field)
{
    if(field == 0)
    {
        edit->year = (u8)((edit->year + 1) % 100);
    }
    else if(field == 1)
    {
        edit->month++;
        if(edit->month > 12) edit->month = 1;
    }
    else if(field == 2)
    {
        edit->day++;
        if(edit->day > DaysInMonth(edit->year, edit->month)) edit->day = 1;
    }
    else if(field == 3)
    {
        edit->hour = (u8)((edit->hour + 1) % 24);
    }
    else if(field == 4)
    {
        edit->minute = (u8)((edit->minute + 1) % 60);
    }
    else if(field == 5)
    {
        edit->second = (u8)((edit->second + 1) % 60);
    }
    ClampTime(edit);
}

static void TimeEdit_Dec(DS1302_TIME *edit, u8 field)
{
    if(field == 0)
    {
        edit->year = (u8)((edit->year == 0) ? 99 : (edit->year - 1));
    }
    else if(field == 1)
    {
        edit->month = (u8)((edit->month <= 1) ? 12 : (edit->month - 1));
    }
    else if(field == 2)
    {
        edit->day = (u8)((edit->day <= 1) ? DaysInMonth(edit->year, edit->month) : (edit->day - 1));
    }
    else if(field == 3)
    {
        edit->hour = (u8)((edit->hour == 0) ? 23 : (edit->hour - 1));
    }
    else if(field == 4)
    {
        edit->minute = (u8)((edit->minute == 0) ? 59 : (edit->minute - 1));
    }
    else if(field == 5)
    {
        edit->second = (u8)((edit->second == 0) ? 59 : (edit->second - 1));
    }
    ClampTime(edit);
}

static void TimeEdit_Process(void)
{
    DS1302_TIME edit;
    u8 field = 0;
    u8 blink = 0;
    u16 blink_tick = time_count;

    edit = RtcTime;
    if(!DS1302_TimeIsValid(&edit))
    {
        edit.year = 24;
        edit.month = 1;
        edit.day = 1;
        edit.hour = 0;
        edit.minute = 0;
        edit.second = 0;
        edit.week = 1;
    }

    OLED_ClearPage();
    while(1)
    {
        if((u16)(time_count - blink_tick) >= 250)
        {
            blink_tick = time_count;
            blink = !blink;
        }

        TimeEdit_Draw(&edit, field, blink);
        key_num = key_scan(1);

        if(key_num == KEY_SET)
        {
            field = (u8)((field + 1) % 6);
            blink = 0;
        }
        else if(key_num == KEY_ADD)
        {
            TimeEdit_Inc(&edit, field);
            blink = 0;
        }
        else if(key_num == KEY_DEC)
        {
            TimeEdit_Dec(&edit, field);
            blink = 0;
        }
        else if(key_num == KEY_SAVE)
        {
            DS1302_WriteTime(&edit);
            RtcTime = edit;
            ShowTimedMessage((u8 *)"   保存成功 ", 0, 600);
            return;
        }
        else if(key_num == KEY_Asterisk)
        {
            OLED_ClearPage();
            ui_page = UI_PAGE_IDLE;
            return;
        }
    }
}

// 密码修改流程
static void PasswordChange_Process(void)
{
    u8 new_code[PASSWORD_LEN];
    u8 confirm_code[PASSWORD_LEN];
    u8 count = 0;
    u8 blink = 0;
    u16 blink_tick = time_count;

    memset(new_code, 0, sizeof(new_code));
    OLED_ClearPage();
    OLED_ShowCH(0, 0, "   修改密码 ");
    OLED_ShowCH(0, 2, "   输入新密码 ");
    OLED_ShowCH(0, 4, "  *退出 #确认 ");
    Password_DrawMask(count, blink);

    while(1)
    {
        key_num = key_scan(1);

        if((u16)(time_count - blink_tick) >= 250)
        {
            blink_tick = time_count;
            blink = !blink;
            Password_DrawMask(count, blink);
        }

        if(key_num <= KEY_9)
        {
            if(count < PASSWORD_LEN)
            {
                new_code[count++] = key_num;
                Password_DrawMask(count, 0);
            }
        }
        else if(key_num == KEY_Asterisk)
        {
            return;
        }
        else if(key_num == KEY_Hashtag && count == PASSWORD_LEN)
        {
            break;
        }
    }

    memset(confirm_code, 0, sizeof(confirm_code));
    OLED_ClearPage();
    OLED_ShowCH(0, 0, "   修改密码 ");
    OLED_ShowCH(0, 2, "   确认新密码 ");
    OLED_ShowCH(0, 4, "  *退出 #确认 ");
    count = 0;
    blink = 0;
    blink_tick = time_count;
    Password_DrawMask(count, blink);

    while(1)
    {
        key_num = key_scan(1);

        if((u16)(time_count - blink_tick) >= 250)
        {
            blink_tick = time_count;
            blink = !blink;
            Password_DrawMask(count, blink);
        }

        if(key_num <= KEY_9)
        {
            if(count < PASSWORD_LEN)
            {
                confirm_code[count++] = key_num;
                Password_DrawMask(count, 0);
            }
        }
        else if(key_num == KEY_Asterisk)
        {
            return;
        }
        else if(key_num == KEY_Hashtag && count == PASSWORD_LEN)
        {
            break;
        }
    }

    if(memcmp(new_code, confirm_code, PASSWORD_LEN) == 0)
    {
        memcpy(Code, new_code, PASSWORD_LEN);
        ShowTimedMessage((u8 *)"   修改成功 ", 0, 800);
    }
    else
    {
        ShowTimedMessage((u8 *)"   两次不同 ", 0, 800);
    }
}
int main(void)
{
    delay_init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    output_init();
    input_init();
    OLED_Init();
    delay_ms(20);
    OLED_ClearPage();

    Usart1_Init(9600);
    USART3_Init(115200);

    DS1302_Init();
    HC_SR501Configuration();
    DS1302_ReadTime(&RtcTime);

    TIM2_Int_Init(71, 999);
    TIM_Cmd(TIM2, ENABLE);

    Alarm_Set(0);
    Door_Set(0);

    while(1)
    {
        RTC_Task();
        PIR_Task();
        Face_BackgroundTask();
        Door_Task();
        Alarm_Task();
        Cloud_ReportTask();

        // 管理认证超时
        if(admin_verified && (u16)(time_count - admin_verify_tick) >= ADMIN_KEEP_MS)
        {
            admin_verified = 0;
            if(ui_page >= UI_PAGE_ADMIN && ui_page <= UI_PAGE_ADMIN_FACE_DEL)
            {
                ui_page = UI_PAGE_IDLE;
            }
        }

        // UI刷新
        if((u16)(time_count - last_ui_tick) >= UI_REFRESH_MS)
        {
            last_ui_tick = time_count;
            
            switch(ui_page)
            {
                case UI_PAGE_IDLE:
                    ShowIdlePage();
                    break;
                case UI_PAGE_ADMIN:
                case UI_PAGE_ADMIN_TIME:
                case UI_PAGE_ADMIN_PWD:
                    ShowAdminPage();
                    break;
                default:
                    break;
            }
        }

        key_num = key_scan(1);
        
        if(key_num != KEY_NONE)
        {
            switch(ui_page)
            {
                case UI_PAGE_IDLE:
                    if(key_num == KEY_SET)          // A切换页面
                    {
                        oled_page_index = (u8)((oled_page_index + 1) % 4);
                        OLED_ClearPage();
                    }
                    else if(key_num == KEY_Hashtag)      // #开门
                    {
                        DoorOpenByPassword_Process();
                    }
                    else if(key_num == KEY_SAVE)   // D按页面执行
                    {
                        if(oled_page_index == 0)
                        {
                            admin_verified = 1;
                            admin_verify_tick = time_count;
                            ui_page = UI_PAGE_ADMIN;
                            OLED_ClearPage();
                        }
                        else if(oled_page_index == 1)
                        {
                            PasswordModifyWithOld_Process();
                            ui_page = UI_PAGE_IDLE;
                        }
                    }
                    break;
                    
                case UI_PAGE_ADMIN:
                    if(key_num == KEY_Asterisk)    // *退出
                    {
                        admin_verified = 0;
                        ui_page = UI_PAGE_IDLE;
                    }
                    else if(key_num == KEY_ADD)    // B录入
                    {
                        FaceRegister_Process();
                    }
                    else if(key_num == KEY_DEC)     // C删除
                    {
                        FaceDelete_Process();
                    }
                    
                    break;
                    
                case UI_PAGE_ADMIN_TIME:
                    if(key_num == KEY_Asterisk)    // *退出
                    {
                        ui_page = UI_PAGE_ADMIN;
                    }
                    else if(key_num == KEY_Hashtag) // #执行时间设置
                    {
                        TimeEdit_Process();
                        ui_page = UI_PAGE_ADMIN;
                    }
                    break;
                    
                case UI_PAGE_ADMIN_PWD:
                    if(key_num == KEY_Asterisk)    // *退出
                    {
                        ui_page = UI_PAGE_ADMIN;
                    }
                    else if(key_num == KEY_Hashtag) // #修改密码
                    {
                        PasswordChange_Process();
                        ui_page = UI_PAGE_ADMIN;
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
}













































