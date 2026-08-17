function [v_ab, sin_a, sin_b] = tb67h450_drive(dp, foc_position, current_mA)
%% tb67h450_drive.m — 复刻固件 USR_TB67H450_SetFocCurrentVector（tb67h450_usr.c:54）
% 输入: dp 驱动参数（drive_params.m）
%       foc_position  FOC 电角度位置（细分步，0~51199；51200 细分步=50 电周期）
%       current_mA    FOC 电流指令幅度 (mA)
% 输出: v_ab = [v_a; v_b] 相电压 (V)；sin_a/sin_b 归一化正弦值（诊断用）
%
% 算法（1:1 复刻固件）：
%   1. 电角度指针：B 相 = foc_position & 0x3FF；A 相 = B + 256（90° 电气）
%   2. 查 1024 点正弦表（幅值 4096，USR_sin_pi_m2）
%   3. 电流幅度 mA -> 12bit DAC：dac = mA*5083>>12（满量程 3300mA->4095）
%   4. 各相占空比 = dac*|sin|>>12
%   5. 相电压 = sign(sin)*duty*VM（方向脚决定极性，P=1/M=0 -> 正向）
%   注意: TB67H450 为电压模式 H 桥，固件将"电流指令"线性映射为 PWM 占空比，
%         真实相电流由电机模型（plant_step_motor）从电压/反电动势推出。

persistent sin_tab
if isempty(sin_tab)
    % 复刻 USR_sin_pi_m2：1024 点、幅值 4096、int16 取整
    idx = (0:dp.sin_points-1).';
    sin_tab = round(4096 * sin(2*pi*idx/dp.sin_points));
end

mask = dp.sin_points - 1;                 % 0x3FF 电角度掩码

ptr_b = mod(foc_position, dp.sin_points);
ptr_a = mod(ptr_b + dp.phase_offset, dp.sin_points);

s_a = sin_tab(ptr_a+1) / dp.sin_amp;      % 归一化 -1..1
s_b = sin_tab(ptr_b+1) / dp.sin_amp;

% 电流幅度 -> 12bit DAC（dac = mA*5083>>12，clamp 4095）
dac = min(floor(abs(current_mA) * dp.cur_coef / 4096), dp.dac_full);

% 各相 12bit 占空比（dac*|sin|>>12，sin_amp=4096=2^12）
dac_a = floor(dac * abs(s_a) * dp.sin_amp / 4096);
dac_b = floor(dac * abs(s_b) * dp.sin_amp / 4096);
dac_a = min(dac_a, dp.dac_full);
dac_b = min(dac_b, dp.dac_full);

% 方向脚极性：sin>0 -> +VM；sin<0 -> -VM；sin=0 -> 同高刹车(0V)
v_ab = [sign(s_a)*dac_a; sign(s_b)*dac_b] * dp.vm / dp.dac_full;
end
