#include "GimbalPID.h"
#include "Servo.h"

/* =========================
   1) 舵机角度范围
   ========================= */
#define X_ANGLE_MIN     65.0f
#define X_ANGLE_MID     90.0f
#define X_ANGLE_MAX     110.0f

#define Y_ANGLE_MIN     150.0f
#define Y_ANGLE_MID     164.0f
#define Y_ANGLE_MAX     176.0f

/* =========================
   2) 死区
   ========================= */
#define X_DEAD_ZONE     2
#define Y_DEAD_ZONE     2

/* =========================
   3) 单次输出限幅
   ========================= */
#define X_PID_OUT_LIMIT 1.6f
#define Y_PID_OUT_LIMIT 1.2f

/* =========================
   4) 积分限幅
   ========================= */
#define X_I_LIMIT       30.0f
#define Y_I_LIMIT       30.0f

static PID_TypeDef pid_x;
static PID_TypeDef pid_y;

static float g_x_angle = X_ANGLE_MID;
static float g_y_angle = Y_ANGLE_MID;

/* 调试量 */
static float g_x_err_dbg = 0.0f;
static float g_y_err_dbg = 0.0f;
static float g_x_out_dbg = 0.0f;
static float g_y_out_dbg = 0.0f;

static float PID_Calc(PID_TypeDef *pid, float target, float measure)
{
    float derivative;

    pid->err = target - measure;
    pid->integral += pid->err;

    if (pid->integral > pid->integral_limit)  pid->integral = pid->integral_limit;
    if (pid->integral < -pid->integral_limit) pid->integral = -pid->integral_limit;

    derivative = pid->err - pid->err_last;

    pid->out = pid->kp * pid->err
             + pid->ki * pid->integral
             + pid->kd * derivative;

    if (pid->out > pid->out_limit)  pid->out = pid->out_limit;
    if (pid->out < -pid->out_limit) pid->out = -pid->out_limit;

    pid->err_last = pid->err;
    return pid->out;
}

static float LimitFloat(float val, float min, float max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

void GimbalPID_Init(void)
{
    pid_x.kp = 0.1f;
    pid_x.ki = 0.002f;
    pid_x.kd = 0.03f;
    pid_x.err = 0;
    pid_x.err_last = 0;
    pid_x.integral = 0;
    pid_x.out = 0;
    pid_x.out_limit = X_PID_OUT_LIMIT;
    pid_x.integral_limit = X_I_LIMIT;

    pid_y.kp = 0.1f;
    pid_y.ki = 0.002f;
    pid_y.kd = 0.03f;
    pid_y.err = 0;
    pid_y.err_last = 0;
    pid_y.integral = 0;
    pid_y.out = 0;
    pid_y.out_limit = Y_PID_OUT_LIMIT;
    pid_y.integral_limit = Y_I_LIMIT;

    g_x_angle = X_ANGLE_MID;
    g_y_angle = Y_ANGLE_MID;

    g_x_err_dbg = 0;
    g_y_err_dbg = 0;
    g_x_out_dbg = 0;
    g_y_out_dbg = 0;

    Servo_SetAngle_X(g_x_angle);
    Servo_SetAngle_Y(g_y_angle);
}

void GimbalPID_Reset(void)
{
    pid_x.err = 0;
    pid_x.err_last = 0;
    pid_x.integral = 0;
    pid_x.out = 0;

    pid_y.err = 0;
    pid_y.err_last = 0;
    pid_y.integral = 0;
    pid_y.out = 0;

    g_x_angle = X_ANGLE_MID;
    g_y_angle = Y_ANGLE_MID;

    g_x_err_dbg = 0;
    g_y_err_dbg = 0;
    g_x_out_dbg = 0;
    g_y_out_dbg = 0;

    Servo_SetAngle_X(g_x_angle);
    Servo_SetAngle_Y(g_y_angle);
}

void GimbalPID_Update(int16_t dx, int16_t dy)
{
    float out_x = 0;
    float out_y = 0;

    /* X轴 */
    if (dx > -X_DEAD_ZONE && dx < X_DEAD_ZONE)
    {
        pid_x.integral *= 0.8f;
        pid_x.err = 0;
        out_x = 0;
    }
    else
    {
        out_x = PID_Calc(&pid_x, 0.0f, (float)dx);
    }

    /* Y轴 */
    if (dy > -Y_DEAD_ZONE && dy < Y_DEAD_ZONE)
    {
        pid_y.integral *= 0.8f;
        pid_y.err = 0;
        out_y = 0;
    }
    else
    {
        out_y = PID_Calc(&pid_y, 0.0f, (float)dy);
    }

    /* 保存调试量 */
    g_x_err_dbg = pid_x.err;
    g_y_err_dbg = pid_y.err;
    g_x_out_dbg = out_x;
    g_y_out_dbg = out_y;

    g_x_angle -= out_x;
    g_y_angle += out_y;

    g_x_angle = LimitFloat(g_x_angle, X_ANGLE_MIN, X_ANGLE_MAX);
    g_y_angle = LimitFloat(g_y_angle, Y_ANGLE_MIN, Y_ANGLE_MAX);

    Servo_SetAngle_X(g_x_angle);
    Servo_SetAngle_Y(g_y_angle);
}

float GimbalPID_GetXAngle(void)
{
    return g_x_angle;
}

float GimbalPID_GetYAngle(void)
{
    return g_y_angle;
}

float GimbalPID_GetXErr(void)
{
    return g_x_err_dbg;
}

float GimbalPID_GetYErr(void)
{
    return g_y_err_dbg;
}

float GimbalPID_GetXOut(void)
{
    return g_x_out_dbg;
}

float GimbalPID_GetYOut(void)
{
    return g_y_out_dbg;
}

void GimbalPID_SetParams(float kp_x, float ki_x, float kd_x,
                         float kp_y, float ki_y, float kd_y,
                         float i_lim_x, float i_lim_y)
{
    pid_x.kp = kp_x;
    pid_x.ki = ki_x;
    pid_x.kd = kd_x;
    pid_x.integral_limit = i_lim_x;

    pid_y.kp = kp_y;
    pid_y.ki = ki_y;
    pid_y.kd = kd_y;
    pid_y.integral_limit = i_lim_y;
}
