%% run_cur_step.m — 电流模式用例（梯形电流规划 + 恒流输出）
% 用法: run('scripts/run_cur_step.m')（Simulation 为工作目录）
% 场景: 200mA -> 800mA -> 0（复刻板端 CURRENT 验收低电流）
% 输出: 电流/速度曲线；验证电流规划器与堵转保护

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

cmd_vals = [200, 800, 0];
cmd_t = [0, 1.0, 3.0, 5.0];
cmd_vals = [0, cmd_vals];   % 与 cmd_t 等长：t<1.0 -> 200mA，t>=3.0 -> 0

T_total = 5.0;
N = round(T_total / dt);
t = (0:N-1)' * dt;

ds = cp.control_freq / 1000;
rec = struct('t',[], 'vel',[], 'cur',[], 'state',[], 'softcur',[], 'goal',[]);

ctrl.request_mode = 3;       % MODE_COMMAND_CURRENT
ctrl.goal_current = 0;

for k = 1:N
    ctrl.goal_current = interp1(cmd_t, cmd_vals, t(k), 'previous');

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
        rec.softcur(end+1) = out.soft_current;
        rec.goal(end+1) = ctrl.goal_current;
    end
end

figure('Name','CURRENT 模式','Color','w');
subplot(3,1,1);
plot(rec.t, rec.cur, 'b', rec.t, rec.goal, 'r--', rec.t, rec.softcur, 'g:');
ylabel('电流 (mA)'); grid on; legend('输出','目标','软目标');
subplot(3,1,2);
plot(rec.t, rec.vel/256, 'b'); ylabel('速度 (整步/s)'); grid on;
subplot(3,1,3);
plot(rec.t, rec.state, 'b'); ylabel('状态'); grid on; ylim([-0.5 6]);
xlabel('时间 (s)');
