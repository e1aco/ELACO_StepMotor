function mp = motor_params()
%% motor_params.m — 电机参数（来源：require.md 硬件池实测值 + 42 系列典型值估算）
% 使用: mp = motor_params();
% 说明: "已确认" 来自 require.md / 固件注释；"估算待辨识" 由 run_identify 校准。

%% ==== 已确认参数（require.md 硬件池） ====
mp.steps_per_rev = 200;     % 整步/圈（步进角 1.8°）
mp.hold_torque   = 0.43;    % 保持力矩 (Nm)，两相励磁额定电流定义
mp.R_phase       = 2.0;     % 相电阻 (Ohm)
mp.L_phase       = 3.6e-3;  % 相电感 (H)
mp.I_rated       = 2.0;     % 额定相电流 (A)
mp.shaft_d       = 5e-3;    % 轴径 (m)，仅参考
mp.weight        = 285e-3;  % 重量 (kg)，仅参考

%% ==== 派生常数（几何/电磁） ====
mp.Nr            = mp.steps_per_rev / 4;      % 极对数 = 50（1 整步 = 1 电气周期）
mp.elec_cycle_steps = mp.steps_per_rev / 4 * 1024 / 4; % 保留占位（见 control_params）

%% ==== 估算待辨识（run_identify.m 校准；42 系列典型值初始） ====
mp.Kt            = mp.hold_torque / (sqrt(2) * mp.I_rated);  % ≈0.152 Nm/A
                                                   % 假设 0.43Nm 为两相励磁保持转矩
mp.Ke            = mp.Kt;        % 反电动势常数 (V*s/rad)，SI 下 = Kt
mp.J             = 5.4e-6;       % 转子惯量 (kg*m^2) ≈ 42 系列典型 54 g*cm^2
mp.B_visc        = 2e-3;         % 粘滞摩擦 (Nm*s/rad)，初估（见下注）
mp.T_coulomb     = 0.02;         % 库仑摩擦 (Nm)，初估（见下注）
% 注: 2026-08 仿真调试发现 Tc=6e-3/Bv=3e-4 时闭环欠阻尼振荡失步（电机转
%     而 controller 位置估算反向跑），提升至 0.02/2e-3 后 4s 位置阶跃
%     正常到位（12544/12800, 死区 256 内 FINISH）。真实值须 run_identify
%     用实测遥测标定，当前值仅保证控制环行为定性正确。
mp.T_detent      = 0.03;         % 齿槽转矩幅值 (Nm)，42 典型 ~7% 保持力矩
mp.detent_order  = 4;            % 齿槽转矩谐波阶数（4*Nr，每齿 4 个稳定点）
mp.T_load        = 0.0;          % 外部负载 (Nm)，默认 0
end
