%% run_vel_ramp.m — 速度模式用例（梯形速度规划 + 速度闭环）
% 用法: run('scripts/run_vel_ramp.m')（Simulation 为工作目录）
% 场景: 0.5 圈/s -> 1.0 圈/s -> 0（复刻板端 VELOCITY 验收）
% 输出: 速度/电流/状态曲线；观察收敛与稳态误差

clear; clc; close all;
addpath(fullfile(fileparts(mfilename('fullpath')), '..', 'params'));
addpath(fullfile(fileparts(mfilename('fullpath')), '..', 'model'));

mp = motor_params();
dp = drive_params();
ep = encoder_params();
cp = control_params();

ctrl = firmware_controller_init(cp);
x_plant = [0; 0; 0; 0];
dt = 1 / cp.control_freq;

% 速度命令序列（细分步/s）：0.5 圈/s -> 1 圈/s -> 0
cmd_vals = [0.5*cp.subdiv_steps, 1.0*cp.subdiv_steps, 0];
cmd_t = [0, 1.0, 3.0, 5.0];
% interp1 需 X/V 等长（'previous' 对 X 末点延伸要求 X 单调；补齐左端）
cmd_t = [0, 1.0, 3.0, 5.0];
cmd_vals = [0, cmd_vals];   % 与 cmd_t 等长：t<1.0 -> 0.5 圈/s（用 'previous' 错位）

T_total = 5.0;
N = round(T_total / dt);
t = (0:N-1)' * dt;

ds = cp.control_freq / 1000;
rec = struct('t',[], 'vel',[], 'cur',[], 'state',[], 'softvel',[], 'goal',[]);

ctrl.request_mode = 2;       % MODE_COMMAND_VELOCITY
ctrl.goal_velocity = 0;

for k = 1:N
    % 命令调度
    ctrl.goal_velocity = interp1(cmd_t, cmd_vals, t(k), 'previous');

    rect = mt6816_encoder(ep, x_plant(3));
    [ctrl, out] = firmware_controller(ctrl, rect);

    if out.sleep || out.brake
        v_ab = [0; 0];
    else
        v_ab = tb67h450_drive(dp, out.foc_position, out.foc_current);
    end
    x_plant = plant_step_motor(mp, x_plant, v_ab, dt);

    if mod(k, ds) == 0
        rec.t(end+1) = t(k);
        rec.vel(end+1) = x_plant(4) / (2*pi) * cp.subdiv_steps;
        rec.cur(end+1) = out.foc_current;
        rec.state(end+1) = out.state;
        rec.softvel(end+1) = out.soft_velocity;
        rec.goal(end+1) = ctrl.goal_velocity;
    end
end

figure('Name','VELOCITY 模式','Color','w');
subplot(3,1,1);
plot(rec.t, rec.vel/256, 'b', rec.t, rec.goal/256, 'r--', rec.t, rec.softvel/256, 'g:');
ylabel('速度 (整步/s)'); grid on; legend('实测','目标','软目标');
subplot(3,1,2);
plot(rec.t, rec.cur, 'b'); ylabel('电流 (mA)'); grid on;
subplot(3,1,3);
plot(rec.t, rec.state, 'b'); ylabel('状态'); grid on; ylim([-0.5 6]);
xlabel('时间 (s)');

% 稳态误差报告（cmd_vals(i+1) = 第 i 段的保持值）
for i = 1:2
    seg = rec.t >= cmd_t(i+1) & rec.t < cmd_t(i+1)+0.5;
    if any(seg)
        fprintf('段%d 目标 %.2f 圈/s: 实测均值 %.3f 圈/s\n', ...
            i, cmd_vals(i+1)/cp.subdiv_steps, mean(rec.vel(seg))/cp.subdiv_steps);
    end
end
