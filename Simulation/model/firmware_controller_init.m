function ctrl = firmware_controller_init(cp)
%% firmware_controller_init.m — 固件控制器状态初始化（firmware_controller.m 配套）
% 用法: ctrl = firmware_controller_init(control_params());
% 说明: 镜像 motor_usr.c 全局静态变量 + motion_planner_usr.c 4 tracker 状态；
%       control_params.m 中 cp.cfg 即 Motor_Config_T（main.c s_motor_config）
    ctrl.cp = cp;
    ctrl.cfg = cp.cfg;
    ctrl.calibrated        = true;      % 编码器校准标志（由外部状态注入）

    ctrl.first_called      = true;
    ctrl.request_mode      = 0;         % MODE_STOP
    ctrl.mode_running      = 0;
    ctrl.state             = 0;

    ctrl.real_lap_position = 0;
    ctrl.real_lap_position_last = 0;
    ctrl.real_position     = 0;
    ctrl.real_position_last = 0;
    ctrl.delta_sum         = 0;

    ctrl.est_velocity      = 0;
    ctrl.est_velocity_integral = 0;
    ctrl.est_lead_position = 0;
    ctrl.est_position      = 0;

    ctrl.goal_position     = 0;
    ctrl.goal_velocity     = 0;
    ctrl.goal_current      = 0;
    ctrl.goal_disable      = false;
    ctrl.goal_brake        = false;

    ctrl.soft_position     = 0;
    ctrl.soft_velocity     = 0;
    ctrl.soft_current      = 0;
    ctrl.soft_disable      = false;
    ctrl.soft_brake        = false;
    ctrl.soft_new_curve    = false;

    ctrl.is_stalled        = false;
    ctrl.stalled_time      = 0;
    ctrl.overload_time     = 0;
    ctrl.overload_flag     = false;

    ctrl.foc_position      = 0;
    ctrl.foc_current       = 0;
    ctrl.foc_integral      = 0;
    ctrl.vel_last          = 0;
    ctrl.vel_goal_last     = 0;

    ctrl.db_active         = 0;
    ctrl.db_brake_cnt      = 0;

    % DCE 位置环状态（参考 StepMotorCtrl_42 motor.c DCE_t）
    ctrl.dce = struct('kp', cp.dce_kp, 'kv', cp.dce_kv, 'ki', cp.dce_ki, 'kd', cp.dce_kd, ...
                      'pError', 0, 'vError', 0, ...
                      'outputKp', 0, 'outputKi', 0, 'outputKd', 0, ...
                      'integralRound', 0, 'integralRemainder', 0, 'output', 0, ...
                      'hold', false);   % 保持段状态（R2 滞回）

    ctrl.out_sleep         = false;
    ctrl.out_brake         = false;

    % planner 状态（motion_planner_usr.c）
    ctrl.pl = struct('cur_acc',     cp.cfg.ratedCurrentAcc, ...
                     'cur_integral',  0, 'track_current', 0, 'go_current', 0, ...
                     'vel_acc',       cp.cfg.ratedVelocityAcc, ...
                     'vel_integral',  0, 'track_velocity', 0, 'go_velocity', 0, ...
                     'pos_up_acc',    cp.cfg.ratedVelocityAcc, ...
                     'pos_down_acc',  cp.cfg.ratedVelocityAcc, ...
                     'pos_integral',  0, 'track_position', 0, ...
                     'track_velocity_pos', 0, ...
                     'go_position', 0, 'go_position_velocity', 0, ...
                     'traj_timeout', 4000);   % 200ms @ 20kHz（轨迹更新超时）
end
