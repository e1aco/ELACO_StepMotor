%% run_identify.m — 电机机械参数辨识（J / B_visc / T_coulomb，加速段线性 LS）
% 用法: run('scripts/run_identify.m')（Simulation 为工作目录）
% 原理:
%   激励: 速度模式闭环从 0 加速到 1 圈/s（0.5s），全程记录每帧
%         ω[k]（编码器速度）与 Te[k]（电磁力矩）
%   机械方程（每帧）: J*dω/dt = Te - B*ω - Tf
%   线性最小二乘（3 参数）: dω/dt = [Te, -ω, -1] * [1/J; B/J; Tf/J]
%       -> A*c = b,  c = A\b  =>  J = 1/c1, B = c2*J, Tf = c3*J
%   合成时 Te 由 plant 逐帧采样；实测时 Te = Kt*I_eff
%       （Kt 由板端堵转校准测保持力矩得出，I_eff 由固件指令电流得出）
% 当前: 用"合成真值"验证辨识流程（无遥测数据时）；预留 CSV 导入段，
%       固件遥测就绪后填 CSV 路径即可对实测数据拟合。
% 输出: 辨识值 vs 真值 对比表

clear; clc; close all;
addpath(fullfile(fileparts(mfilename('fullpath')), '..', 'params'));
addpath(fullfile(fileparts(mfilename('fullpath')), '..', 'model'));

mp = motor_params();
dp = drive_params();
ep = encoder_params();
cp = control_params();

dt = 1 / cp.control_freq;
freq = cp.control_freq;

%% ========== 1. 合成测量（或导入固件遥测 CSV） ==========
use_csv = false;                 % ← 有遥测 CSV 时改 true 并填 csv_path
csv_path = 'telemetry.csv';      % 期望列: t, vel(细分步/s), cur(mA)  或 t,pos,vel,cur

if use_csv
    D = readtable(csv_path);
    t_m = D.t; vel_m = D.vel; cur_m = D.cur;   % 细分步/s, mA
    om  = vel_m / cp.subdiv_steps * 2*pi;      % rad/s
    Te  = mp.Kt * 0.001 * cur_m;               % Nm（Kt 板端校准）
    d_om = [diff(om); 0] / dt;
    keep = (1:numel(om))' > 2;                 % 去首帧毛刺
else
    % ---- 合成激励: 速度闭环 1 圈/s 与 2 圈/s 各跑 1.2s，记录 ω 与 Te ----
    % 注: 合成验证时关齿槽（T_detent=0），齿槽正弦项会污染拟合；
    %     实测时齿槽相对小，可对速度/力矩先做多周期平均消除。
    mp_syn = mp; mp_syn.T_detent = 0;
    N = round(1.2 / dt);
    om = zeros(N,1); Te = zeros(N,1);
    for seg = 1:2
        ctrl = firmware_controller_init(cp);
        x_plant = [0; 0; 0; 0];
        ctrl.request_mode = 2;
        ctrl.goal_velocity = seg * cp.subdiv_steps;   % 1 或 2 圈/s
        for k = 1:N
            rect = mt6816_encoder(ep, x_plant(3));
            [ctrl, out] = firmware_controller(ctrl, rect);
            if out.sleep || out.brake
                v_ab = [0; 0];
            else
                v_ab = tb67h450_drive(dp, out.foc_position, out.foc_current);
            end
            [x_plant, te] = plant_step_motor(mp_syn, x_plant, v_ab, dt);
            om(k) = x_plant(4);
            Te(k) = te;
        end
        if seg == 1
            om1v = om; Te1v = Te;
        else
            om2v = om; Te2v = Te;
        end
    end
    fprintf('合成测量完成（真值: J=%.2e, B=%.2e, Tf=%.4f）\n', mp.J, mp.B_visc, mp.T_coulomb);
end

%% ========== 2. B/Tf: 双稳态差分（两次独立速度点） ==========
nh = 3000;                       % 各取末段 3000 帧稳态
om1 = mean(om1v(end-nh+1:end)); Te1 = mean(Te1v(end-nh+1:end));
om2 = mean(om2v(end-nh+1:end)); Te2 = mean(Te2v(end-nh+1:end));
fprintf('稳态1: ω1=%.2f rad/s (%.2f 圈/s), Te1=%.4f Nm\n', om1, om1/2/pi, Te1);
fprintf('稳态2: ω2=%.2f rad/s (%.2f 圈/s), Te2=%.4f Nm\n', om2, om2/2/pi, Te2);
B_id  = (Te2 - Te1) / (om2 - om1);
Tf_id = Te1 - B_id * om1;

%% ========== 3. J: 加速段 LS（固定 B/Tf，每帧 J = (Te-B*ω-Tf)/(dω/dt)） ==========
om = om1v; Te = Te1v;            % 用第一段（0->1 圈/s 加速）
d_om = [diff(om); 0] / dt;
keep = (1:N)' > 2;
num = Te(keep) - B_id*om(keep) - Tf_id;
den = d_om(keep);
use = abs(den) > 1e-3;           % 丢弃 dω≈0 帧（稳态段无信息量）
J_id = mean(num(use) ./ den(use));

%% ========== 3. 报告 ==========
fprintf('\n==== 辨识结果（合成验证） ====\n');
fprintf(' 参数      真值              辨识值            误差\n');
fprintf('  J       %-10.3e  %-10.3e  %+5.1f%%\n', mp.J, J_id, (J_id-mp.J)/mp.J*100);
fprintf('  B       %-10.3e  %-10.3e  %+5.1f%%\n', mp.B_visc, B_id, (B_id-mp.B_visc)/mp.B_visc*100);
fprintf('  Tf      %-10.3e  %-10.3e  %+5.1f%%\n', mp.T_coulomb, Tf_id, (Tf_id-mp.T_coulomb)/mp.T_coulomb*100);
fprintf('\n提示: 有固件遥测 CSV 后设 use_csv=true，可对实测速度/电流拟合。\n');
fprintf('      Kt 独立测量：堵转恒流测保持力矩（板端校准流程已含）。\n');

figure('Name','辨识激励响应','Color','w');
plot((1:N)'*dt, om/2/pi, 'b'); ylabel('速度 (圈/s)'); grid on;
xlabel('时间 (s)'); title('速度模式加速响应');