function dp = drive_params()
%% drive_params.m — TB67H450 驱动 + PWM 参数
% 来源: tb67h450_usr.c 常量 + require.md（VM=24V 用户确认）
% 固件算法: USR_TB67H450_SetFocCurrentVector 1:1 复刻见 model/tb67h450_drive.m

dp.vm            = 24;       % 电机供电电压 (V) —— 用户确认 24V
dp.dac_bits      = 12;       % PWM 占空比 12bit
dp.dac_full      = 4095;     % 12bit 满量程 = 100% 占空比
dp.cur_full_mA   = 3300;     % 满量程对应电流 (mA)：固件注释 4095↔3.3A（电压模式等效）
dp.cur_coef      = 5083;     % mA→DAC 系数（dac = mA*5083>>12，tb67h450_usr.c:17）
dp.sin_points    = 1024;     % 正弦表点数/电周期（USR_sin_pi_m2）
dp.sin_amp       = 4096;     % 正弦表幅值（int16 定点，2^12）
dp.phase_offset  = 256;      % A/B 相 90° 电角相差（1024/4，tb67h450_usr.c:16）
dp.pwm_model     = 'average';% 平均电压模型（PWM 载波纹波忽略；50µs 控制周期 >> 载波周期）
end
