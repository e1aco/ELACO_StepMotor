function [ctrl, out] = firmware_controller(ctrl, rectified)
%% firmware_controller.m — 固件电机控制环 1:1 复刻（motor_usr.c + motion_planner_usr.c）
% 用法:
%   cp   = control_params();
%   ctrl = firmware_controller_init(cp);          % 初始化（见同目录 init 文件）
%   [ctrl, out] = firmware_controller(ctrl, rect);% 每 50us 调用一次（20kHz）
% 返回 out 结构（诊断）: est_velocity/est_position/foc_current/foc_position/
%                       state/mode/soft_position/soft_velocity/vel_goal/db_active
%
% ⚠ 复刻范围说明:
%   - 逐帧逻辑、定点移位（>>N）、限幅、状态机 与 motor_usr.c:599 Tick20kHz 对齐
%   - 内部函数按 motor_usr.h 注释行为重构，标注 [TBC] 处待与当前源码核对
%     （用户正在修改固件代码；代码稳定后逐项复核）:
%     S_CalcPositionCascade / S_CalcVelocityP(Kp/Kd Q格式) /
%     S_CalcCurrentToOutput / foc_position 更新方式 / planner 轨迹细节
%   - 定点语义: C int32 算术右移 >> 用 sra()（先 int32 环绕再移位，负数向 -inf 取整）
%     乘加溢出用 wrap32() 模拟 int32 环绕

    cp = ctrl.cp;
    S  = cp.subdiv_steps;

    % ---- 2. 首帧零位初始化（motor_usr.c:622-657） ----
    if ctrl.first_called
        off = ctrl.cfg.encoderHomeOffset;
        if off < S/2
            if rectified > off + S/2
                angle = rectified - S;
            else
                angle = rectified;
            end
        else
            if rectified < off - S/2
                angle = rectified + S;
            else
                angle = rectified;
            end
        end
        ctrl.real_lap_position      = angle;
        ctrl.real_lap_position_last = angle;
        ctrl.real_position          = angle;
        ctrl.real_position_last     = angle;
        ctrl.first_called           = false;
        out = fw_out(ctrl);
        return;
    end

    % ---- 3. 位置增量（回绕安全 cycle_sub，motor_usr.c:660-666） ----
    ctrl.real_lap_position_last = ctrl.real_lap_position;
    ctrl.real_lap_position      = rectified;
    delta = cycle_sub(ctrl.real_lap_position, ctrl.real_lap_position_last, S);
    ctrl.delta_sum = ctrl.delta_sum + delta;
    ctrl.real_position_last = ctrl.real_position;
    ctrl.real_position      = ctrl.real_position + delta;

    % ---- 4. 速度估计 IIR（1/32 低通，motor_usr.c:671-675） ----
    % integral += Δpos×20kHz + (v<<5 - v);  v = integral>>5
    % int32 溢出用 wrap32 模拟（C 语义）
    ctrl.est_velocity_integral = wrap32(ctrl.est_velocity_integral ...
        + delta * cp.control_freq ...
        + (sra(ctrl.est_velocity, -5) - ctrl.est_velocity));
    ctrl.est_velocity = sra(ctrl.est_velocity_integral, 5);
    ctrl.est_velocity_integral = wrap32(ctrl.est_velocity_integral ...
        - sra(ctrl.est_velocity, -5));

    % ---- 5. 超前角补偿（motor_usr.c:680-681） ----
    ctrl.est_lead_position = s_compensate_advanced_angle(ctrl, ctrl.est_velocity);
    ctrl.est_position      = ctrl.real_position + ctrl.est_lead_position;

    % ---- 6. 模式切换（motor_usr.c:684-689） ----
    if ctrl.mode_running ~= ctrl.request_mode
        ctrl.mode_running = ctrl.request_mode;
        ctrl = s_zero_output(ctrl);
        ctrl.soft_new_curve = true;
    end

    % ---- 7. 目标限幅（motor_usr.c:692-707） ----
    ctrl.goal_velocity = clamp(ctrl.goal_velocity, -ctrl.cfg.ratedVelocity, ctrl.cfg.ratedVelocity);
    ctrl.goal_current  = clamp(ctrl.goal_current, -ctrl.cfg.ratedCurrent, ctrl.cfg.ratedCurrent);

    % ---- 8. 新曲线触发（motor_usr.c:752-785）：先初始化 planner 再生成软目标 ----
    if ctrl.soft_new_curve ...
            || (ctrl.soft_disable && ~ctrl.goal_disable) ...
            || (ctrl.soft_brake && ~ctrl.goal_brake)
        ctrl.soft_new_curve = false;
        ctrl = s_clear_integral(ctrl);
        ctrl.is_stalled    = false;
        ctrl.stalled_time  = 0;
        ctrl.overload_time = 0;
        ctrl.overload_flag = false;
        switch ctrl.mode_running
            case {1, 5}                  % POSITION / PWM_POSITION
                ctrl.pl.track_position = ctrl.est_position;
                ctrl.pl.track_velocity_pos = ctrl.est_velocity;
                ctrl.pl.go_position = ctrl.est_position;
                ctrl.pl.go_position_velocity = 0;
            case 4                       % TRAJECTORY
                ctrl.pl.track_position = ctrl.est_position;
                ctrl.pl.track_velocity_pos = ctrl.est_velocity;
            case {2, 6}                  % VELOCITY / PWM_VELOCITY
                ctrl.pl.track_velocity = ctrl.est_velocity;
                ctrl.pl.go_velocity = ctrl.est_velocity;
            case {3, 7}                  % CURRENT / PWM_CURRENT
                ctrl.pl.track_current = ctrl.foc_current;
                ctrl.pl.go_current = ctrl.foc_current;
        end
    end

    % ---- 9. planner 软目标生成（motor_usr.c:788-817）：先规划，分派用本帧软目标 ----
    switch ctrl.mode_running
        case {1, 5}                      % POSITION（S 曲线位置规划）
            [ctrl.pl, sp, sv] = pl_position_tracker(ctrl.pl, ctrl.goal_position, cp);
            ctrl.soft_position = sp;
            ctrl.soft_velocity = sv;
        case 4                           % TRAJECTORY
            [ctrl.pl, sp, sv] = pl_trajectory_tracker(ctrl.pl, ctrl.goal_position, ...
                ctrl.goal_velocity, cp);
            ctrl.soft_position = sp;
            ctrl.soft_velocity = sv;
        case {2, 6}                      % VELOCITY（梯形速度规划）
            [ctrl.pl, sv] = pl_velocity_tracker(ctrl.pl, ctrl.goal_velocity, cp);
            ctrl.soft_velocity = sv;
        case {3, 7}                      % CURRENT（梯形电流规划）
            [ctrl.pl, sc] = pl_current_tracker(ctrl.pl, ctrl.goal_current, cp);
            ctrl.soft_current = sc;
    end
    ctrl.soft_disable = ctrl.goal_disable;
    ctrl.soft_brake   = ctrl.goal_brake;

    % ---- 10. 控制分派（motor_usr.c:711-749） ----
    if ctrl.is_stalled || ctrl.soft_disable || ~ctrl.calibrated
        ctrl = s_zero_output(ctrl);
        ctrl = s_clear_integral(ctrl);
        ctrl.out_sleep = true;          % TB67H450_Sleep（驱动层，仿真不建模）
        ctrl.out_brake = false;
    elseif ctrl.soft_brake
        ctrl = s_zero_output(ctrl);
        ctrl = s_clear_integral(ctrl);
        ctrl.out_sleep = false;
        ctrl.out_brake = true;          % TB67H450_Brake
    else
        ctrl.out_sleep = false;
        ctrl.out_brake = false;
        switch ctrl.mode_running
            case 0                       % MODE_STOP
                ctrl = s_zero_output(ctrl);
                ctrl.out_sleep = true;
            case {1, 4, 5}               % POSITION / TRAJECTORY / PWM_POSITION
                if cp.use_dce
                    % 到位精度优化A：DCE 积分保持（参考 StepMotorCtrl_42 motor.c:149-192）
                    ctrl = s_calc_dce_to_output(ctrl, ctrl.soft_position, ctrl.soft_velocity);
                else
                    ctrl = s_calc_position_cascade(ctrl, ctrl.soft_position);
                end
            case {2, 6}                  % VELOCITY / PWM_VELOCITY
                ctrl = s_calc_velocity_p(ctrl, ctrl.soft_velocity);
            case {3, 7}                  % CURRENT / PWM_CURRENT
                ctrl = s_calc_current_to_output(ctrl, ctrl.soft_current);
            otherwise
                ctrl = s_zero_output(ctrl);
                ctrl.out_sleep = true;
        end
    end

    % ---- 11. 堵转检测（motor_usr.c:824-847） ----
    if ctrl.cfg.stallProtectSwitch
        in_cur_mode = (ctrl.mode_running == 3) || (ctrl.mode_running == 7);
        if (in_cur_mode && ctrl.foc_current ~= 0) ...
                || abs(ctrl.foc_current) == ctrl.cfg.ratedCurrent
            if abs(ctrl.est_velocity) < cp.stall_vel_max
                if ctrl.stalled_time >= cp.stall_time_us
                    ctrl.is_stalled = true;
                else
                    ctrl.stalled_time = ctrl.stalled_time + cp.control_us;
                end
            end
        else
            ctrl.stalled_time = 0;
        end
    end

    % ---- 12. 过载检测（motor_usr.c:850-867） ----
    if (ctrl.mode_running ~= 3) && (ctrl.mode_running ~= 7) ...
            && abs(ctrl.foc_current) == ctrl.cfg.ratedCurrent
        if ctrl.overload_time >= cp.stall_time_us
            ctrl.overload_flag = true;
        else
            ctrl.overload_time = ctrl.overload_time + cp.control_us;
        end
    else
        ctrl.overload_time = 0;
        ctrl.overload_flag = false;
    end

    % ---- 13. 状态机（motor_usr.c:870-940） ----
    ctrl.state = fw_state_machine(ctrl, cp);

    out = fw_out(ctrl);
end

% ============ 内部函数（与固件静态函数一一对应） ============

% ---- 超前角补偿（motor_usr.c S_CompensateAdvancedAngle，参数 motor_usr.h:84-90） ----
% C int32 乘法溢出环绕语义（v 可能达 ±67M，v*slope 超 2^31）
function lead = s_compensate_advanced_angle(ctrl, vel)
    cp = ctrl.cp;
    v = abs(vel);
    if v < cp.lead_vel1
        lead = 0;
    elseif v < cp.lead_vel2
        lead = sra(wrap32((v - cp.lead_vel1) * cp.lead_slope1), cp.q_lead);
    elseif v < cp.lead_vel3
        lead = sra(wrap32((v - cp.lead_vel1) * cp.lead_slope1), cp.q_lead) ...
             + sra(wrap32((v - cp.lead_vel2) * cp.lead_slope2), cp.q_lead);
    else
        lead = sra(wrap32((v - cp.lead_vel1) * cp.lead_slope1), cp.q_lead) ...
             + sra(wrap32((v - cp.lead_vel2) * cp.lead_slope2), cp.q_lead) ...
             + sra(wrap32((v - cp.lead_vel3) * cp.lead_slope3), cp.q_lead);
    end
    lead = sign(vel) * clamp(lead, -cp.lead_max, cp.lead_max);
end

% ---- 位置串级环：位置 P -> 速度 PD -> 电流（[TBC] 待与源码核对） ----
% 行为依据 motor_usr.h 注释：
%   死区滞回 |err|<HYST 才归零；绕回窗口死区 256；MIN_VEL 强制推进；
%   减速窗口内 vel_goal=32*err（posKp=32768>>10）；假速度区间直驱电流；
%   死区制动 10ms 后 0 电流（方案Y：保持力已删除）
% FOC 电角度 = 命令微步推进（foc_position += vel_goal 积分，foc_integral 保留余量），
%   保证电角度前进产生旋转磁场拖动转子（开环 FOC 原理；[TBC] 待确认固件方式）
function ctrl = s_calc_position_cascade(ctrl, goal_position)
    cp = ctrl.cp;
    S  = cp.subdiv_steps;

    err = cycle_sub(goal_position, ctrl.est_position, S);

    % 绕回窗口死区（motor_usr.h:56-66）
    db = cp.pos_deadband;
    wrap_mode = (goal_position > S - cp.pos_wrap_win) ...
             && (goal_position < S + cp.pos_wrap_win);
    if wrap_mode
        db = cp.pos_deadband_wrap;
    end

    if abs(err) <= db
        % ===== 死区内 =====
        if ctrl.db_active
            ctrl.db_active = 0;
            ctrl.db_brake_cnt = round(cp.pos_db_brake_ms * cp.control_freq / 1000);
        end
        if ctrl.db_brake_cnt > 0
            % 一次性制动：速度环刹停残余速度（有限帧，防假速度自激）
            ctrl.db_brake_cnt = ctrl.db_brake_cnt - 1;
            ctrl = s_calc_velocity_p(ctrl, 0);
        else
            % 0 电流（方案Y 保持力删除，motor_usr.h:67-71 宏保留未用）
            ctrl.foc_current = 0;
            ctrl.vel_last = ctrl.est_velocity;
        end
    else
        % ===== 死区外：出界触发驱动 =====
        ctrl.db_active = 1;
        ctrl.db_brake_cnt = 0;

        % 位置误差 -> 速度目标（posKp Q10：vel_goal = err*posKp>>10 = 32*err）
        vel_goal = sra(err * ctrl.cfg.posKp, cp.q_poskp);
        vel_goal = clamp(vel_goal, -cp.pos_err_max * 32, cp.pos_err_max * 32);

        if wrap_mode
            % 绕回窗口（0 点毛刺区）：60000 猛推冲破（motor_usr.h:52-55）
            if abs(vel_goal) < cp.pos_min_vel_wrap
                vel_goal = sign(vel_goal) * cp.pos_min_vel_wrap;
            end
        elseif abs(err) > cp.pos_min_vel_ds
            % 常规：出界深 -> MIN_VEL 强制推进（推得动 > 编码器假速度）
            if abs(vel_goal) < cp.pos_min_vel
                vel_goal = sign(vel_goal) * cp.pos_min_vel;
            end
        end
        % 减速窗口内：保持 32*err 线性下坡，低速进死区

        % vel_goal 每帧限斜率（motor_usr.h:31-33）
        dv = clamp(vel_goal - ctrl.vel_goal_last, -cp.vel_goal_acc, cp.vel_goal_acc);
        vel_goal = ctrl.vel_goal_last + dv;
        ctrl.vel_goal_last = vel_goal;

        if abs(vel_goal) <= cp.fake_vel_max
            % 假速度区间：位置误差 1:1 直驱电流（不经速度环，防假速度主导）
            % [TBC] 注释为"用 s_real_position"（motor_usr.h:44-47），这里用 est_position
            ctrl.foc_current = clamp(err, -ctrl.cfg.ratedCurrent, ctrl.cfg.ratedCurrent);
            ctrl.vel_last = ctrl.est_velocity;
            % FOC 电角度推进 = 命令微步积分（速度环路径由 s_calc_velocity_p 内部推进，
            % 这里仅直驱分支需自行推进，避免双重推进）
            ctrl.foc_integral = ctrl.foc_integral + vel_goal;
            ctrl.foc_position = mod(ctrl.foc_position + fix(ctrl.foc_integral / cp.control_freq), S);
            ctrl.foc_integral = rem(ctrl.foc_integral, cp.control_freq);
        else
            % 速度环（内部已推进 FOC 电角度）
            ctrl = s_calc_velocity_p(ctrl, vel_goal);
        end
    end
end

% ---- 速度 PD 环（motor_usr.c S_CalcVelocityP；[TBC] Kp/Kd Q 格式待核对） ----
% pidKp=10 速度误差->电流；pidKd=400 阻尼（速度变化->反向电流）
function ctrl = s_calc_velocity_p(ctrl, goal_vel)
    cp = ctrl.cp;

    verr = goal_vel - ctrl.est_velocity;
    verr = clamp(verr, -cp.vel_err_max, cp.vel_err_max);

    dvel = ctrl.est_velocity - ctrl.vel_last;      % 帧间速度变化（阻尼）

    % C int32 乘法溢出环绕语义（verr*Kp / dvel*Kd 可能超 2^31）
    p = sra(wrap32(verr * ctrl.cfg.pidKp), cp.q_pidkp);   % 比例
    d = sra(wrap32(dvel * ctrl.cfg.pidKd), cp.q_pidkd);   % 微分/阻尼
    ctrl.foc_current = clamp(p - d, -ctrl.cfg.ratedCurrent, ctrl.cfg.ratedCurrent);

    ctrl.vel_last = ctrl.est_velocity;

    % FOC 电角度推进 = 命令微步积分（速度模式下跟随速度命令）
    ctrl.foc_integral = ctrl.foc_integral + goal_vel;
    ctrl.foc_position = mod(ctrl.foc_position + fix(ctrl.foc_integral / cp.control_freq), ...
                            cp.subdiv_steps);
    ctrl.foc_integral = rem(ctrl.foc_integral, cp.control_freq);
end

% ---- 电流直接输出（motor_usr.c S_CalcCurrentToOutput） ----
function ctrl = s_calc_current_to_output(ctrl, goal_current)
    ctrl.foc_current = clamp(goal_current, -ctrl.cfg.ratedCurrent, ctrl.cfg.ratedCurrent);
    ctrl.vel_last = ctrl.est_velocity;
    % [TBC] 电流模式下 FOC 电角度更新方式待确认（校准场景固定角度或跟随）
end

% ---- DCE 位置环（motor_usr.c S_CalcDceToOutput，R2/R3 现状逐帧镜像） ----
% 到位精度优化A（require.md 2026-08-17）：积分保持电流钉住命令角
% 定点语义与固件一致：
%   pError = location - est_position（限幅 ±3200）
%   vError = (speed - est_velocity)>>7（限幅 ±4000）
%   outputKp = kp*pError
%   积分: 运动段 integralRound += ki*pError + kv*vError（>>7 提取余量）
%        保持段仅 ki*pError（R2：速度项入段瞬间冲击积分）
%   outputKi 限幅: 运动段 ±ratedCurrent<<10 / 保持段 ±HOLD_MA<<10（R2 防隐藏弹簧跑飞）
%   抗饱和: 已达限幅且同向累积丢弃（R2）
%   outputKd = kd*vError（保持段剔除，R2）
%   output = (Kp+Ki+Kd)>>10，限幅 运动段 ±ratedCurrent / 保持段 ±HOLD_MA
% 保持段滞回（R2）：入界 |err|≤KEEP_HYS(128) 进保持、出界 |err|>KEEP_WIN(256) 回运动
% FOC 相位: 保持段 focpos=goal 钉命令角（磁弹簧 Kt·|I|·Nr 抵抗 detent）；
%          运动段 est±256 旋转拖（电流符号决定，±90° 电角静态转矩场）
function ctrl = s_calc_dce_to_output(ctrl, location, speed)
    cp = ctrl.cp;
    d  = ctrl.dce;

    d.pError = location - ctrl.est_position;
    d.pError = clamp(d.pError, -cp.dce_p_clamp, cp.dce_p_clamp);

    d.vError = sra(speed - ctrl.est_velocity, cp.q_dce_vi);
    d.vError = clamp(d.vError, -cp.dce_v_clamp, cp.dce_v_clamp);

    % 保持段状态（滞回，须先于积分/输出计算）
    e_hold = cycle_sub(location, ctrl.est_position, cp.subdiv_steps);
    if d.hold
        if abs(e_hold) > cp.dce_keep_win
            d.hold = false;
        end
    elseif abs(e_hold) <= cp.dce_keep_hys
        d.hold = true;
    end

    % 分段限幅（R2：保持段收紧，防运动段饱和积分隐藏弹簧反向猛推跑飞）
    if d.hold
        ki_limit  = sra(cp.dce_hold_ma, -cp.q_dce_out);   % HOLD_MA<<10
        out_limit = cp.dce_hold_ma;
    else
        ki_limit  = sra(ctrl.cfg.ratedCurrent, -cp.q_dce_out); % rated<<10
        out_limit = ctrl.cfg.ratedCurrent;
    end

    % 比例项（C int32 乘法环绕语义）
    d.outputKp = wrap32(d.kp * d.pError);

    % 积分项（>>7 余量提取，motor.c:165-169；保持段仅位置项）
    if d.hold
        d.integralRound = wrap32(d.integralRound + wrap32(d.ki * d.pError));
    else
        d.integralRound = wrap32(d.integralRound ...
            + wrap32(d.ki * d.pError) + wrap32(d.kv * d.vError));
    end
    d.integralRemainder = sra(d.integralRound, cp.q_dce_vi);
    d.integralRound     = wrap32(d.integralRound - sra(d.integralRemainder, -cp.q_dce_vi));

    % 抗饱和（R2：已达限幅且本帧同向累积则丢弃，防顶格滞留/超调回摆）
    if d.integralRemainder > 0 && d.outputKi >= ki_limit
        d.integralRemainder = 0;
    elseif d.integralRemainder < 0 && d.outputKi <= -ki_limit
        d.integralRemainder = 0;
    end
    d.outputKi = d.outputKi + d.integralRemainder;
    d.outputKi = clamp(d.outputKi, -ki_limit, ki_limit);

    % 微分项（R2：保持段剔除，入段瞬间速度项冲击输出）
    if d.hold
        d.outputKd = 0;
    else
        d.outputKd = wrap32(d.kd * d.vError);
    end

    % 合成输出（motor.c:181-189）
    d.output = sra(d.outputKp + d.outputKi + d.outputKd, cp.q_dce_out);
    d.output = clamp(d.output, -out_limit, out_limit);

    % FOC 相位（变体 B：保持钉命令角 + 运动旋转拖）
    % 驱动 dac=abs(电流)，方向由 focpos 决定：
    %   - 运动段（|err|>keep_win）：est±256 旋转磁场拖动
    %   - 保持段（|err|≤keep_hys 滞回）：focpos=goal 钉命令角 → 恢复刚度
    if d.hold
        ctrl.foc_position = mod(location, cp.subdiv_steps);
    elseif d.output > 0
        ctrl.foc_position = mod(ctrl.est_position + cp.dce_phase_90, cp.subdiv_steps);
    elseif d.output < 0
        ctrl.foc_position = mod(ctrl.est_position - cp.dce_phase_90, cp.subdiv_steps);
    else
        ctrl.foc_position = mod(ctrl.est_position, cp.subdiv_steps);
    end

    ctrl.dce = d;
    ctrl.foc_current = d.output;
    ctrl.vel_last = ctrl.est_velocity;
end

% ---- 输出清零 + 积分清零（motor_usr.c S_ZeroOutput/S_ClearIntegral） ----
function ctrl = s_zero_output(ctrl)
    ctrl.foc_position = 0;
    ctrl.foc_current  = 0;
end
function ctrl = s_clear_integral(ctrl)
    ctrl.vel_goal_last = 0;
    ctrl.pl.cur_integral = 0;
    ctrl.pl.vel_integral = 0;
    ctrl.pl.pos_integral = 0;
    % DCE 积分清零（motor.c ClearIntegral:194-203；R2 保持段状态复位）
    ctrl.dce.integralRound = 0;
    ctrl.dce.integralRemainder = 0;
    ctrl.dce.outputKi = 0;
    ctrl.dce.hold = false;
end

% ============ planner（motion_planner_usr.c，[TBC] 轨迹细节待核对） ============

% ---- CurrentTracker（复刻 motion_planner_usr.c:161-213 梯形，C 向零截断语义） ----
function [pl, go_cur] = pl_current_tracker(pl, goal_current, cp)
    acc = pl.cur_acc;
    delta = goal_current - pl.track_current;
    if delta == 0
        pl.track_current = goal_current;
    else
        step = fix(pl.cur_integral / cp.control_freq);   % C int 除法（向零）
        pl.track_current = pl.track_current + step;
        pl.cur_integral  = rem(pl.cur_integral, cp.control_freq);  % C %
        if delta > 0
            if pl.track_current >= 0
                pl.cur_integral = pl.cur_integral + acc;
                pl.track_current = pl.track_current ...
                    + fix(pl.cur_integral / cp.control_freq);
                pl.cur_integral = rem(pl.cur_integral, cp.control_freq);
                if pl.track_current >= goal_current
                    pl.cur_integral = 0;
                    pl.track_current = goal_current;
                end
            else
                pl.cur_integral = pl.cur_integral + acc;
                pl.track_current = pl.track_current ...
                    + fix(pl.cur_integral / cp.control_freq);
                pl.cur_integral = rem(pl.cur_integral, cp.control_freq);
                if pl.track_current >= 0
                    pl.cur_integral = 0;
                    pl.track_current = 0;
                end
            end
        else
            if pl.track_current <= 0
                pl.cur_integral = pl.cur_integral - acc;
                pl.track_current = pl.track_current ...
                    + fix(pl.cur_integral / cp.control_freq);
                pl.cur_integral = rem(pl.cur_integral, cp.control_freq);
                if pl.track_current <= goal_current
                    pl.cur_integral = 0;
                    pl.track_current = goal_current;
                end
            else
                pl.cur_integral = pl.cur_integral - acc;
                pl.track_current = pl.track_current ...
                    + fix(pl.cur_integral / cp.control_freq);
                pl.cur_integral = rem(pl.cur_integral, cp.control_freq);
                if pl.track_current <= 0
                    pl.cur_integral = 0;
                    pl.track_current = 0;
                end
            end
        end
    end
    go_cur = pl.track_current;
end

% ---- VelocityTracker（梯形，同 CurrentTracker 结构，motion_planner_usr.c:267+） ----
function [pl, go_vel] = pl_velocity_tracker(pl, goal_velocity, cp)
    acc = pl.vel_acc;
    delta = goal_velocity - pl.track_velocity;
    if delta == 0
        pl.track_velocity = goal_velocity;
    else
        step = fix(pl.vel_integral / cp.control_freq);
        pl.track_velocity = pl.track_velocity + step;
        pl.vel_integral   = rem(pl.vel_integral, cp.control_freq);
        if delta > 0
            if pl.track_velocity >= 0
                pl.vel_integral = pl.vel_integral + acc;
                pl.track_velocity = pl.track_velocity ...
                    + fix(pl.vel_integral / cp.control_freq);
                pl.vel_integral = rem(pl.vel_integral, cp.control_freq);
                if pl.track_velocity >= goal_velocity
                    pl.vel_integral = 0;
                    pl.track_velocity = goal_velocity;
                end
            else
                pl.vel_integral = pl.vel_integral + acc;
                pl.track_velocity = pl.track_velocity ...
                    + fix(pl.vel_integral / cp.control_freq);
                pl.vel_integral = rem(pl.vel_integral, cp.control_freq);
                if pl.track_velocity >= 0
                    pl.vel_integral = 0;
                    pl.track_velocity = 0;
                end
            end
        else
            if pl.track_velocity <= 0
                pl.vel_integral = pl.vel_integral - acc;
                pl.track_velocity = pl.track_velocity ...
                    + fix(pl.vel_integral / cp.control_freq);
                pl.vel_integral = rem(pl.vel_integral, cp.control_freq);
                if pl.track_velocity <= goal_velocity
                    pl.vel_integral = 0;
                    pl.track_velocity = goal_velocity;
                end
            else
                pl.vel_integral = pl.vel_integral - acc;
                pl.track_velocity = pl.track_velocity ...
                    + fix(pl.vel_integral / cp.control_freq);
                pl.vel_integral = rem(pl.vel_integral, cp.control_freq);
                if pl.track_velocity <= 0
                    pl.vel_integral = 0;
                    pl.track_velocity = 0;
                end
            end
        end
    end
    go_vel = pl.track_velocity;
end

% ---- PositionTracker（S 曲线位置规划，[TBC] 梯形近似，细节待核对） ----
% 目标：以限加速/限速生成位置软目标，接近目标时减速窗口，到位停住
% 整数积分器语义：同 motion_planner_usr.c S_CalcPositionVelocityIntegral
% （pos_integral 按步/s 累加，每帧 +integral/20000 整数步，余量保留）
function [pl, go_pos, go_vel] = pl_position_tracker(pl, goal_position, cp)
    acc = pl.pos_up_acc;
    vmax = cp.cfg.ratedVelocity;
    freq = cp.control_freq;

    err = goal_position - pl.track_position;
    dir_goal = sign(err);
    if abs(err) < 1e-6
        pl.track_position = goal_position;
        pl.track_velocity_pos = 0;
        pl.pos_integral = 0;
    else
        v = pl.track_velocity_pos;
        need = (v*v) / (2*acc);            % 当前速度下减速所需距离
        if abs(err) <= need
            v = v - dir_goal * acc / freq; % 减速段
        else
            v = v + dir_goal * acc / freq; % 加速/巡航段
            v = clamp(v, -vmax, vmax);
        end
        if dir_goal ~= 0 && sign(v) ~= dir_goal
            v = 0;                         % 防减速过头反向
        end
        % 整数步积分（步/s 累加，每帧取整进给；C int 截断语义）
        pl.track_velocity_pos = fix(v);
        pl.pos_integral = pl.pos_integral + pl.track_velocity_pos;
        pl.track_position = pl.track_position ...
            + fix(pl.pos_integral / freq);
        pl.pos_integral = rem(pl.pos_integral, freq);
    end
    go_pos = pl.track_position;
    go_vel = pl.track_velocity_pos;
end

% ---- TrajectoryTracker（位置+速度轨迹，200ms 超时停车，[TBC] 简化为位置规划） ----
function [pl, go_pos, go_vel] = pl_trajectory_tracker(pl, goal_position, goal_velocity, cp)
    if pl.traj_timeout > 0
        pl.traj_timeout = pl.traj_timeout - 1;
        [pl, go_pos, go_vel] = pl_position_tracker(pl, goal_position, cp);
        % 速度目标钳制（轨迹速度主导，[TBC]）
        go_vel = sign(go_vel) * min(abs(go_vel), abs(goal_velocity));
    else
        % 超时：停车
        pl.track_velocity_pos = 0;
        go_pos = pl.track_position;
        go_vel = 0;
    end
end

% ============ 状态机（motor_usr.c:870-940） ============
function state = fw_state_machine(ctrl, cp)
    if ~ctrl.calibrated
        state = 5;                       % STATE_NO_CALIB
    elseif ctrl.mode_running == 0
        state = 0;                       % STATE_STOP
    elseif ctrl.is_stalled
        state = 4;                       % STATE_STALL
    elseif ctrl.overload_flag
        state = 3;                       % STATE_OVERLOAD
    else
        S = cp.subdiv_steps;
        switch ctrl.mode_running
            case {1, 4, 5}               % 位置类
                if cp.use_dce
                    % DCE 方案（参考 motor.c:434-437）：planner 软目标到位即 FINISH，
                    % 无死区判定；控制环持续积分保持，精度由积分/编码器分辨率决定
                    if ctrl.soft_position == ctrl.goal_position && ctrl.soft_velocity == 0
                        state = 1;       % STATE_FINISH
                    else
                        state = 2;       % STATE_RUNNING
                    end
                else
                    db = cp.pos_deadband;
                    if ctrl.goal_position > S - cp.pos_wrap_win ...
                            && ctrl.goal_position < S + cp.pos_wrap_win
                        db = cp.pos_deadband_wrap;
                    end
                    e = cycle_sub(ctrl.goal_position, ctrl.est_position, S);
                    if abs(e) <= db && abs(ctrl.est_velocity) <= cp.vel_deadband
                        state = 1;       % STATE_FINISH
                    else
                        state = 2;       % STATE_RUNNING
                    end
                end
            case {2, 6}                  % 速度类
                if abs(ctrl.goal_velocity - ctrl.est_velocity) <= cp.vel_deadband
                    state = 1;
                else
                    state = 2;
                end
            case {3, 7}                  % 电流类
                if abs(ctrl.goal_current - ctrl.foc_current) <= cp.cur_deadband
                    state = 1;
                else
                    state = 2;
                end
            otherwise
                state = 0;
        end
    end
end

% ============ 输出打包 ============
function out = fw_out(ctrl)
    out.est_velocity   = ctrl.est_velocity;
    out.est_position   = ctrl.est_position;
    out.real_position  = ctrl.real_position;
    out.est_lead       = ctrl.est_lead_position;
    out.foc_current    = ctrl.foc_current;
    out.foc_position   = ctrl.foc_position;
    out.state          = ctrl.state;
    out.mode           = ctrl.mode_running;
    out.soft_position  = ctrl.soft_position;
    out.soft_velocity  = ctrl.soft_velocity;
    out.soft_current   = ctrl.soft_current;
    out.vel_goal_last  = ctrl.vel_goal_last;
    out.db_active      = ctrl.db_active;
    out.is_stalled     = ctrl.is_stalled;
    out.overload_flag  = ctrl.overload_flag;
    out.sleep          = ctrl.out_sleep;
    out.brake          = ctrl.out_brake;
end

% ============ 通用工具 ============
function v = clamp(v, lo, hi)
    v = min(max(v, lo), hi);
end

% 模周期最短差（同 cycle_usr 最短循环差语义）
function d = cycle_sub(a, b, period)
    d = a - b;
    if d >  period/2, d = d - period;
    elseif d < -period/2, d = d + period; end
end

% int32 溢出环绕（模拟 C int32 语义）
function x = wrap32(x)
    x = mod(x, 2^32);
    if x >= 2^31, x = x - 2^32; end
end

% C int32 算术右移：先环绕 int32，再 >>k（负数向 -inf 取整，与 C 一致）
% k<0 表示左移（v<<5 = v*32），同样环绕
function y = sra(x, k)
    x = wrap32(x);
    if k >= 0
        y = floor(x / 2^k);
    else
        y = x * 2^(-k);
        y = wrap32(y);
    end
end