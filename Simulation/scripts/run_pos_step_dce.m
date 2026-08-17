%% run_pos_step_dce.m — DCE 积分保持位置阶跃对比（到位精度优化A）
% 对照实验（require.md 2026-08-17）：参考项目 DCE 方案能否把落点精度提到 0.08~0.1°。
% 原理：参考 StepMotorCtrl_42 motor.c CalcDceToOutput（kp200/kv80/ki300/kd250），
%   位置误差 -> P + 积分（积分保持电流钉住命令角）+ 速度微分 -> 电流。
% 变体 B（本脚本，已在 firmware_controller.m 实现，R2/R3 与固件现状逐帧镜像）：
%   - 运动段：focpos = est±256（参考版旋转磁场拖动）
%   - 保持段：滞回 入界 |err|≤128 / 出界 |err|>256，focpos=goal 钉命令角
%     -> 恢复刚度 Kt·|I|·Nr 抵抗 detent；积分仅位置项 + 去 kd + 分段限幅
%     （R2：防运动段饱和积分隐藏弹簧跑飞；R3：保持电流 1500mA）
%   - 保持段电流限幅 dce_hold_ma=1500mA（R3）
% 对比：cp.use_dce=false（run_pos_step.m 方案Y 死区串级，落点 ±0.6~0.9°）。
% 用法: cd Simulation 后直接运行；输出落点误差表 + 曲线图。

clear; clc; close all;
addpath(fullfile(fileparts(mfilename('fullpath')), '..', 'params'));
addpath(fullfile(fileparts(mfilename('fullpath')), '..', 'model'));

%% 1. 参数装配
mp = motor_params();          % 电机参数（require.md + 估算）
dp = drive_params();          % 驱动参数（VM=24V）
ep = encoder_params();        % 编码器参数
cp = control_params();        % 固件控制参数镜像
cp.use_dce = true;            % DCE 积分保持（变体 B）
noise_std  = 3;               % 编码器噪声（磁干扰量级 std=3；0=理想）
ep.noise_std = noise_std;
ep.calib_residual = true;     % 校准残差（复现实板 -10/+24/+6/+12；false=理想编码器）

%% 2. 初始化
ctrl = firmware_controller_init(cp);
x_plant = [0; 0; 0; 0];       % [i_a; i_b; theta_mech; omega]
dt = 1 / cp.control_freq;

% 命令序列（细分步）：同 SW2 验收 90°->180°->270°->360°
goals_deg  = [90, 180, 270, 360];
goals_step = goals_deg * cp.subdiv_steps / 360;
settle_s   = 3.0;             % 到位后停留（对齐固件 TEST_A 3s 采样：保持积分
                              % 1500mA 爬升 ~1.2s + 收敛 ~1s）

T_total = 16.0;                         % 总仿真时长 (s)（4 段 × 3s settle + 运动裕量）
N = round(T_total / dt);
t  = (0:N-1)' * dt;

% 记录缓冲（1kHz 降采样显示）
ds = cp.control_freq / 1000;
rec = struct('t',[], 'pos',[], 'vel',[], 'cur',[], 'state',[], 'softpos',[], ...
             'goal',[], 'err',[], 'ki',[]);

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
        if ctrl.state == 1 && seg_done   % STATE_FINISH（DCE：planner 到位即 FINISH）
            seg_settle = seg_settle + 1;
            if seg_settle >= round(settle_s / dt)
                fprintf('[%s] seg%d FINISH goal=%d step, err=%d (%.3f°)\n', ...
                        datestr(now,'HH:MM:SS'), seg, goals_step(seg), ...
                        round(goals_step(seg)) - ctrl.real_position, ...
                        (round(goals_step(seg)) - ctrl.real_position) ...
                            / cp.subdiv_steps * 360);
                seg = seg + 1;
                seg_done = false;
                seg_settle = 0;
            end
        end
    end

    % ---- 编码器 -> 控制环 -> 驱动 -> 电机（每帧一次） ----
    rect = mt6816_encoder(ep, x_plant(3));
    [ctrl, out] = firmware_controller(ctrl, rect);

    if out.sleep || out.brake
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
        rec.ki(end+1) = ctrl.dce.outputKi;
    end
end

%% 4. 绘图
figure('Name','POSITION 阶跃 DCE（90/180/270/360）','Color','w');

subplot(5,1,1);
plot(rec.t, rec.pos/256, 'b', rec.t, rec.goal/256, 'r--', rec.t, rec.softpos/256, 'g:');
ylabel('位置 (整步)'); grid on; legend('实测','目标','软目标');

subplot(5,1,2);
plot(rec.t, rec.vel/256, 'b'); ylabel('速度 (整步/s)'); grid on;

subplot(5,1,3);
plot(rec.t, rec.cur, 'b'); ylabel('FOC电流 (mA)'); grid on;

subplot(5,1,4);
plot(rec.t, rec.err, 'b'); ylabel('位置误差 (步)'); grid on;
ylim([-20 20]);

subplot(5,1,5);
plot(rec.t, rec.state, 'b'); ylabel('状态'); grid on;
xlabel('时间 (s)'); ylim([-0.5 6]);

%% 5. 落点误差报告（对照目标 0.08~0.1° = 11.4~14.2 步）
fprintf('\n==== DCE 落点误差报告（细分步，51200/圈；目标 0.08~0.1° = ±11~14 步） ====\n');
fprintf('编码器噪声 std=%g（0=理想；3=磁干扰量级）\n', noise_std);
for g = 1:numel(goals_step)
    idx = find(rec.goal == goals_step(g) & rec.state == 1, 1, 'last');
    if ~isempty(idx)
        e = rec.err(idx);
        fprintf('目标 %3d°（%5d 步）: 实测 %5d 步, 误差 %+d 步 (%+.3f°) %s\n', ...
                goals_deg(g), goals_step(g), round(rec.pos(idx)), e, ...
                e/cp.subdiv_steps*360, ...
                iif(abs(e) <= 11, '达标(≤0.08°)', iif(abs(e) <= 14, '达标(≤0.1°)', '未达标')));
    end
end
% 静止段保持电流统计（DCE 积分保持特性）
idx_fin = find(rec.state == 1);
if ~isempty(idx_fin)
    fprintf('到位保持电流: 均值 %+.0f mA, 峰值 %+.0f mA\n', ...
            mean(rec.cur(idx_fin)), max(abs(rec.cur(idx_fin))));
end

function e = wrap_err(e, cp)
    e = mod(e + cp.subdiv_steps/2, cp.subdiv_steps) - cp.subdiv_steps/2;
end
function v = iif(c, a, b)
    if c, v = a; else, v = b; end
end