%% run_tune_gain_sweep.m — posKp / pidKp / pidKd 网格扫描整定
% 用法: run('scripts/run_tune_gain_sweep.m')（Simulation 为工作目录）
% 输出: 指标表（收敛时间/超调/到位误差）+ 推荐参数
% 注意: 每次仿真 2s = 4 万帧；网格 3*4*3=36 组，耗时约 1~3 分钟（MATLAB 循环开销）

clear; clc; close all;
addpath(fullfile(fileparts(mfilename('fullpath')), '..', 'params'));
addpath(fullfile(fileparts(mfilename('fullpath')), '..', 'model'));

mp = motor_params();
dp = drive_params();
ep = encoder_params();
cp = control_params();

%% 扫描网格
posKp_grid = [16384, 32768, 65536];    % Q10: 16 / 32 / 64
pidKp_grid = [3, 10, 15];              % 板端实测 5 推不动 / 10 可行
pidKd_grid = [200, 400, 800];          % 板端实测 400 收敛 / 800 饱和振荡

goal_step = 12800;                     % 90° 阶跃（细分步）
T_total = 2.0;
dt = 1 / cp.control_freq;
N = round(T_total / dt);

results = table();
r = 0;
for pk = posKp_grid
for pv = pidKp_grid
for pd = pidKd_grid
    cp2 = cp;
    cp2.cfg.posKp = pk;
    cp2.cfg.pidKp = pv;
    cp2.cfg.pidKd = pd;

    ctrl = firmware_controller_init(cp2);
    x_plant = [0; 0; 0; 0];
    ctrl.request_mode = 1;
    ctrl.goal_position = goal_step;

    finish_k = NaN; overshoot = NaN; err_final = NaN; max_i = 0;
    prev_pos = 0;
    for k = 1:N
        rect = mt6816_encoder(ep, x_plant(3));
        [ctrl, out] = firmware_controller(ctrl, rect);
        if out.sleep || out.brake
            v_ab = [0; 0];
        else
            v_ab = tb67h450_drive(dp, out.foc_position, out.foc_current);
        end
        x_plant = plant_step_motor(mp, x_plant, v_ab, dt);

        pos = x_plant(3) / (2*pi) * cp2.subdiv_steps;
        max_i = max(max_i, abs(out.foc_current));
        if ctrl.state == 1 && isnan(finish_k)
            finish_k = t_k(k, dt);
        end
        if pos > goal_step && overshoot == 0  % 首次越过目标
            overshoot = pos - goal_step;
        end
        prev_pos = pos;
    end
    % 终态误差（最后 0.5s 均值）
    err_final = pos - goal_step;
    converge = ~isnan(finish_k) && abs(err_final) <= cp2.pos_deadband;

    r = r + 1;
    results(r,:) = table(pk, pv, pd, finish_k, overshoot, err_final, ...
                         max_i, converge);
    fprintf('posKp=%5d pidKp=%2d pidKd=%3d -> 收敛=%d t_fin=%.2fs 过冲=%4.0f err=%+4.0f Imax=%5.0fmA\n', ...
            pk, pv, pd, converge, finish_k, overshoot, err_final, max_i);
end
end
end

results.Properties.VariableNames = ...
    {'posKp','pidKp','pidKd','t_finish_s','overshoot_step','err_final_step', ...
     'Imax_mA','converged'};
disp(results);

%% 推荐参数（所有指标中取收敛且 t_finish 最小）
ok = results(results.converged, :);
if ~isempty(ok)
    [~, bi] = min(ok.t_finish_s);
    fprintf('\n推荐参数: posKp=%d, pidKp=%d, pidKd=%d（t_finish=%.2fs）\n', ...
        ok.posKp(bi), ok.pidKp(bi), ok.pidKd(bi), ok.t_finish_s(bi));
    fprintf('可回写 config_usr.h: posKp/pidKp/pidKd（代码稳定后由你确认再写）\n');
else
    fprintf('\n无收敛组合 —— 需检查模型参数或放宽网格\n');
end

function tk = t_k(k, dt)
    tk = (k-1) * dt;
end
