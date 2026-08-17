function [x_new, te, td] = plant_step_motor(mp, x, v_ab, dt)
%% plant_step_motor.m — 两相混合式步进电机连续模型（欧拉积分）
% 输入: mp 电机参数（motor_params.m）
%       x  = [i_a(A); i_b(A); theta_mech(rad); omega(rad/s)] 状态向量
%       v_ab = [v_a; v_b] 相电压 (V)
%       dt 积分步长 (s)，默认 50e-6（20kHz 与固件同步）
% 输出: x_new 新状态；te 电磁转矩 (Nm)；td 齿槽转矩 (Nm)
%
% 模型方程（两相混合式步进电机标准模型，50 对极）：
%   电气: L di/dt = v - R*i - e
%         e_a = Ke*omega*sin(Nr*theta)
%         e_b = -Ke*omega*cos(Nr*theta)
%   机械: J domega/dt = Te - B*omega - Tf*sign(omega) - Td - TL
%         Te = -Kt*i_a*sin(Nr*theta) + Kt*i_b*cos(Nr*theta)
%         Td = T_detent*sin(detent_order*Nr*theta)
% 说明: L/R = 3.6mH/2Ω = 1.8ms >> 50us，欧拉积分精度足够
% 依据: require.md 电机参数（R=2Ω, L=3.6mH, T=0.43Nm, I=2A, 1.8°）

if nargin < 4 || isempty(dt)
    dt = 50e-6;
end

ia = x(1);
ib = x(2);
th = x(3);
om = x(4);

Nr = mp.Nr;
sa = sin(Nr*th);
ca = cos(Nr*th);

% ---- 电气 ----
ea = mp.Ke * om * sa;                 % 反电动势 A 相
eb = -mp.Ke * om * ca;                % 反电动势 B 相
dia = (v_ab(1) - mp.R_phase*ia - ea) / mp.L_phase;
dib = (v_ab(2) - mp.R_phase*ib - eb) / mp.L_phase;

% ---- 转矩 ----
te = -mp.Kt*ia*sa + mp.Kt*ib*ca;     % 电磁转矩
td = mp.T_detent * sin(mp.detent_order*Nr*th);  % 齿槽转矩
tl = mp.T_load;

% ---- 机械 ----
tf = mp.T_coulomb * sign(om);         % 库仑摩擦（0 速处粘滞项兜底）
if om == 0 && abs(te - td - tl) <= mp.T_coulomb
    dom = 0;                          % 静摩擦锁止
else
    dom = (te - mp.B_visc*om - tf - td - tl) / mp.J;
end

% ---- 欧拉积分 ----
x_new = x + dt * [dia; dib; om; dom];
end
