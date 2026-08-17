function cp = control_params()
%% control_params.m — 固件控制参数镜像（motor_usr.h / config_usr.h / main.c）
% 每一组参数标注来源文件:行号（2026-08-17 读取的版本；代码若修改需同步本文件）
% 注意：用户正在修改固件代码，本文件基于当前已知版本；代码稳定后需复核。

%% ==== 系统常量（motor_usr.h:22-27） ====
cp.control_freq   = 20000;        % 控制 tick 20kHz
cp.control_us     = 50;           % 控制周期 50us
cp.hard_steps     = 200;          % 整步/圈
cp.soft_divide    = 256;          % 细分/整步
cp.subdiv_steps   = 51200;        % 细分步/圈

%% ==== 位置环（motor_usr.h:29-80） ====
cp.pos_err_max      = 3200;       % P 环误差限幅（细分步）
cp.vel_goal_acc     = 500;        % vel_goal 每帧变化限幅（步/s/帧）
cp.pos_min_vel      = 30000;      % 死区外最小推进速度（步/s）
cp.pos_min_vel_ds   = 2048;       % 减速窗口（出界深度，细分步）
cp.fake_vel_max     = 25000;      % 假速度上限（步/s）→ 低速直驱分支
cp.pos_db_brake_ms  = 10;         % 死区制动时长（ms）
cp.pos_min_vel_wrap = 60000;      % 绕回窗口猛推速度（步/s）
cp.pos_wrap_win     = 1024;       % 绕回窗口（细分步，目标≈0 点 51200±1024）
cp.pos_db_hyst      = 16;         % 死区入界滞回（步）
cp.pos_deadband_wrap= 256;        % 绕回窗口内死区（细分步）
cp.pos_hold_ma_step = 1024;       % 死区保持力系数（mA/步*1024）
                                   % 保留宏定义；8/15 方案Y 已删除保持力输出（TBC 待代码确认）

%% ==== 速度环（motor_usr.h:73-80） ====
cp.vel_err_max      = 1048576;    % 速度环误差限幅（步/s，1M）
cp.pos_deadband     = 256;        % 位置到位死区（细分步，≈1.8° 判定）
cp.vel_deadband     = 512;        % 速度到位死区（步/s）
cp.cur_deadband     = 10;         % 电流到位死区（mA）

%% ==== 超前角补偿（motor_usr.h:82-90） ====
cp.lead_vel1   = 100000;          % 低于此速无补偿（步/s）
cp.lead_vel2   = 1300000;         % 段 1 上界
cp.lead_vel3   = 2200000;         % 段 2 上界
cp.lead_max    = 430;             % 补偿封顶（细分步，≈3°）
cp.lead_slope1 = 262;             % 段 1 斜率（>>20）
cp.lead_slope2 = 105;             % 段 2 斜率（>>20）
cp.lead_slope3 = 52;              % 段 3 斜率（>>20）

%% ==== DCE 位置环（参考 StepMotorCtrl_42 motor.c CalcDceToOutput + main.c:141-144） ====
% 到位精度优化A：移植参考 DCE 积分保持对照实验（require.md 2026-08-17）
% 原理：位置误差 -> P + 积分（积分保持电流钉住命令角）+ 速度微分 -> 电流输出；
%       FOC 相位 = est_position ± 256（电流符号决定，±90° 电角静态转矩场）
cp.use_dce       = false;         % true=DCE 位置环（参考方案对照实验，run_pos_step_dce.m）；false=方案Y 串级（CL 固件现状）
cp.dce_kp        = 200;           % 位置比例（参考 main.c:141）
cp.dce_kv        = 80;            % 速度前馈项（参考 main.c:142，进积分器）
cp.dce_ki        = 300;           % 位置积分（参考 main.c:143）
cp.dce_kd        = 250;           % 速度微分（参考 main.c:144）
cp.dce_p_clamp   = 3200;          % pError 限幅（细分步，motor.c:156-157）
cp.dce_v_clamp   = 4000;          % vError 限幅（motor.c:158-159，>>7 后）
cp.q_dce_vi      = 7;             % vError >>7、积分余量 >>7（motor.c:153/167）
cp.q_dce_out     = 10;            % 输出 >>10（motor.c:182）
cp.dce_phase_90  = 256;           % SOFT_DIVIDE_NUM=256 细分步 = 90° 电角（motor.h:10）
cp.dce_keep_win  = 256;           % 保持段出界阈值（细分步，motor_usr.h KEEP_WIN）
cp.dce_keep_hys  = 128;           % 保持段入界阈值（细分步，motor_usr.h KEEP_HYS，2:1 滞回）
cp.dce_hold_ma   = 1500;          % 保持段电流限幅（mA，motor_usr.h HOLD_MA：R1 300→R2 500→R3 1500）
cp.dce_cur_max   = 2000;          % 运动段输出/积分限幅（mA，=ratedCurrent，motor_usr.h CUR_MAX）

%% ==== 堵转/过载（motor_usr.h:94-95） ====
cp.stall_time_us  = 1000000;      % 判定时长（us）
cp.stall_vel_max  = cp.subdiv_steps / 5;   % 堵转速度上限（<1/5 圈/s）

%% ==== 默认电机配置（main.c s_motor_config / config_usr.h） ====
cp.cfg = struct( ...
    'encoderHomeOffset', 0, ...           % 编码器零位偏移（细分步）
    'ratedCurrent',    2000, ...          % 电流限幅 (mA)，42 步进额定 2A
    'ratedVelocity',   2*cp.subdiv_steps, ... % 速度限幅 2 圈/s（细分步/s）
    'ratedVelocityAcc',100*cp.subdiv_steps, ... % 加速度 100 圈/s^2
    'ratedCurrentAcc', 2000, ...          % 电流加速度 (mA/s)
    'posKp',           32768, ...         % 位置环增益（Q10：32*err）
    'pidKp',           10, ...            % 速度环增益（Q 格式 TBC）
    'pidKd',           400, ...           % 速度环阻尼（Q 格式 TBC）
    'stallProtectSwitch', true);          % 堵转保护

%% ==== 定点格式（TBC：待代码确认后精化） ====
cp.q_poskp    = 10;    % posKp Q 格式（32768>>10 = 32 = 注释"vel_goal=32*err"）
cp.q_pidkp    = 0;     % pidKp Q 格式（TBC）
cp.q_pidkd    = 0;     % pidKd Q 格式（TBC）
cp.q_lead     = 20;    % 超前角斜率 Q 格式（>>20）
end
