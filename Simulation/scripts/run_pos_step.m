%% run_pos_step.m — 位置模式阶跃用例（复刻板上 SW2 验收：90°/180°/270°/360°）
% 用法: 直接运行（MATLAB 控制台 run('scripts/run_pos_step.m')，或 cd Simulation 后运行）
% 输出: 位置/速度/电流/状态曲线图 + 各目标到位误差表
%
% 仿真架构:
%   firmware_controller（复刻固件 20kHz 控制环）
%     -> tb67h450_drive（复刻 FOC 查表 -> A/B 相电压）
%     -> plant_step_motor（电机电气-机械模型）
%     -> mt6816_encoder（编码器量化/噪声）
%     -> 回 firmware_controller（编码器角度）——闭环

clear; clc; close all;
addpath(fullfile(fileparts(mfilename('fullpath')), '..', 'params'));
addpath(fullfile(fileparts(mfilename('fullpath')), '..', 'model'));

%% 1. 参数装配
mp = motor_params();          % 电机参数（require.md + 估算）
dp = drive_params();          % 驱动参数（VM=24V）
ep = encoder_params();        % 编码器参数
cp = control_params();        % 固件控制参数镜像

%% 2. 初始化
ctrl = firmware_controller_init(cp);
x_plant = [0; 0; 0; 0];       % [i_a; i_b; theta_mech; omega]
dt = 1 / cp.control_freq;

% 命令序列（细分步）：复刻 SW2 单击 90°->180°->270°->360°
goals_deg  = [90, 180, 270, 360];
goals_step = goals_deg * cp.subdiv_steps / 360;
% 到位等待：每段先等 FINISH，再额外停留 0.3s 观察静止稳定性
settle_s   = 0.3;

T_total = 4.0;                         % 总仿真时长 (s)
N = round(T_total / dt);
t  = (0:N-1)' * dt;

% 记录缓冲（1kHz 降采样显示）
ds = cp.control_freq / 1000;           % 每 ds 帧采 1 点
rec = struct('t',[], 'pos',[], 'vel',[], 'cur',[], 'state',[], 'softpos',[], ...
             'goal',[], 'err',[]);

% 命令调度状态
seg = 1; seg_done = false; seg_settle = 0;
ctrl.request_mode = 1;                 % MODE_COMMAND_POSITION
ctrl.goal_position = 0;
ctrl.goal_velocity = 0;
ctrl.goal_current  = 0;

%% 3. 闭环仿真循环（20kHz）
for k = 1:N
    % ---- 命令调度：FINISH 后停留 settle_s 再发下一段 ----
    if seg <= numel(goals_step)
        if ~seg_done
            ctrl.goal_position = goals_step(seg);
            seg_done = true;
        end
        if ctrl.state == 1 && seg_done   % STATE_FINISH
            seg_settle = seg_settle + 1;
            if seg_settle >= round(settle_s / dt)
                fprintf('[%s] seg%d FINISH goal=%d step, err=%d\n', ...
                        datestr(now,'HH:MM:SS'), seg, goals_step(seg), ...
                        round(goals_step(seg)) - ctrl.real_position);
                seg = seg + 1;
                seg_done = false;
                seg_settle = 0;
            end
        end
    end

    % ---- 编码器 -> 控制环 -> 驱动 -> 电机（每帧一次） ----
    rect = mt6816_encoder(ep, x_plant(3));
    [ctrl, out] = firmware_controller(ctrl, rect);

    if out.sleep
        v_ab = [0; 0];
    elseif out.brake
        v_ab = [0; 0];
    else
        v_ab = tb67h450_drive(dp, out.foc_position, out.foc_current);
    end

    x_plant = plant_step_motor(mp, x_plant, v_ab, dt);

    % ---- 记录 ----
    if mod(k, ds) == 0
        rec.t(end+1) = t(k);
        rec.pos(end+1) = x_plant(3) / (2*pi) * cp.subdiv_steps;  % 细分步
        rec.vel(end+1) = x_plant(4) / (2*pi) * cp.subdiv_steps;  % 细分步/s
        rec.cur(end+1) = out.foc_current;
        rec.state(end+1) = out.state;
        rec.softpos(end+1) = out.soft_position;
        rec.goal(end+1) = ctrl.goal_position;
        rec.err(end+1) = wrap_err(ctrl.goal_position - ctrl.real_position, cp);
    end
end

%% 4. 绘图
figure('Name','POSITION 阶跃（90/180/270/360）','Color','w');

subplot(4,1,1);
plot(rec.t, rec.pos/256, 'b', rec.t, rec.goal/256, 'r--', rec.t, rec.softpos/256, 'g:');
ylabel('位置 (整步)'); grid on; legend('实测','目标','软目标');

subplot(4,1,2);
plot(rec.t, rec.vel/256, 'b'); ylabel('速度 (整步/s)'); grid on;

subplot(4,1,3);
plot(rec.t, rec.cur, 'b'); ylabel('FOC电流 (mA)'); grid on;

subplot(4,1,4);
plot(rec.t, rec.state, 'b'); ylabel('状态'); grid on;
xlabel('时间 (s)'); ylim([-0.5 6]);

%% 5. 到位误差报告
fprintf('\n==== 到位误差报告（细分步，51200/圈） ====\n');
for g = 1:numel(goals_step)
    idx = find(rec.goal == goals_step(g) & rec.state == 1, 1, 'last');
    if ~isempty(idx)
        fprintf('目标 %3d°（%5d 步）: 实测 %5d 步, 误差 %+d 步 (%.2f°)\n', ...
                goals_deg(g), goals_step(g), round(rec.pos(idx)), ...
                rec.err(idx), rec.err(idx)/cp.subdiv_steps*360);
    end
end

function e = wrap_err(e, cp)
    e = mod(e + cp.subdiv_steps/2, cp.subdiv_steps) - cp.subdiv_steps/2;
end
