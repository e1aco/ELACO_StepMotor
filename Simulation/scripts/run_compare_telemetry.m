%% run_compare_telemetry.m — 固件遥测 CSV 与仿真叠加对比（模型校准闭环）
% 用法: run('scripts/run_compare_telemetry.m')（Simulation 为工作目录）
% 前提: 板端导出遥测 CSV（100Hz 遥测），列:
%       时间(s), 位置(细分步), 速度(细分步/s), 电流(mA), 模式, 状态
%       （固件 USR_Motor_GetTelemetry 打包 pos/vel/cur/mode/state）
% 流程: 1) 读取 CSV -> 2) 用相同命令序列跑仿真 -> 3) 叠加绘图 + 误差指标
% 当前: 无 CSV 时演示用法并提示（数据就绪即可用，无需改模型）

clear; clc; close all;
addpath(fullfile(fileparts(mfilename('fullpath')), '..', 'params'));
addpath(fullfile(fileparts(mfilename('fullpath')), '..', 'model'));

csv_file = 'telemetry.csv';        % ← 放到 Simulation/ 目录或填绝对路径

if ~isfile(csv_file)
    fprintf(['未找到 %s —— 暂无遥测数据。\n' ...
             '待板端导出后重跑本脚本：仿真将与实测曲线叠加对比，\n' ...
             '并输出误差指标用于校准 motor_params（J/B/detent）。\n'], csv_file);
    return;
end

D = readtable(csv_file);
t_m  = D.t;
pos_m = D.pos;      % 细分步
vel_m = D.vel;      % 细分步/s
cur_m = D.cur;      % mA
mode_m = D.mode;
state_m = D.state;

%% 命令序列从遥测模式/位置推断（简化：位置模式，用实测位置近似命令）
mp = motor_params();
dp = drive_params();
ep = encoder_params();
cp = control_params();

ctrl = firmware_controller_init(cp);
x_plant = [pos_m(1) / cp.subdiv_steps * 2*pi; 0; pos_m(1)/cp.subdiv_steps*2*pi; 0];
dt = 1 / cp.control_freq;
ctrl.request_mode = 1;
ctrl.goal_position = goals(1);

T_total = max(t_m);
N = round(T_total / dt);
t = (0:N-1)' * dt;

rec = struct('t',[], 'pos',[], 'vel',[], 'cur',[]);
for k = 1:N
    % 用实测位置曲线当目标（近似命令序列，误差小；精确命令可硬编码覆盖）
    ctrl.goal_position = interp1(t_m, pos_m, t(k), 'previous', 'extrap');

    rect = mt6816_encoder(ep, x_plant(3));
    [ctrl, out] = firmware_controller(ctrl, rect);
    if out.sleep || out.brake
        v_ab = [0; 0];
    else
        v_ab = tb67h450_drive(dp, out.foc_position, out.foc_current);
    end
    x_plant = plant_step_motor(mp, x_plant, v_ab, dt);

    if mod(k, round(cp.control_freq/1000)) == 0
        rec.t(end+1) = t(k);
        rec.pos(end+1) = x_plant(3) / (2*pi) * cp.subdiv_steps;
        rec.vel(end+1) = x_plant(4) / (2*pi) * cp.subdiv_steps;
        rec.cur(end+1) = out.foc_current;
    end
end

%% 叠加绘图 + 误差指标
figure('Name','实测 vs 仿真','Color','w');
subplot(3,1,1);
plot(t_m, pos_m, 'b', rec.t, rec.pos, 'r--');
ylabel('位置 (细分步)'); grid on; legend('实测','仿真');
subplot(3,1,2);
plot(t_m, vel_m, 'b', rec.t, rec.vel, 'r--');
ylabel('速度 (细分步/s)'); grid on;
subplot(3,1,3);
plot(t_m, cur_m, 'b', rec.t, rec.cur, 'r--');
ylabel('电流 (mA)'); grid on; xlabel('时间 (s)');

% 对齐采样（线性插值仿真到遥测时刻）
pos_sim_i = interp1(rec.t, rec.pos, t_m, 'linear', 'extrap');
err_pos = pos_m - pos_sim_i;
fprintf('\n==== 对比指标 ====\n');
fprintf('位置均方根误差: %.1f 细分步（%.2f°）\n', sqrt(mean(err_pos.^2)), ...
        sqrt(mean(err_pos.^2))/cp.subdiv_steps*360);
fprintf('若误差大：优先校准 motor_params.m 的 J / B_visc / T_detent（run_identify）\n');
