function ep = encoder_params()
%% encoder_params.m — MT6816 编码器模型参数
% 来源: mt6816_usr/motor_usr.c（raw*25/8 映射）、require.md
% 说明: 校准表存在时 rectified 直接落在 51200 细分步空间；
%       未校准路径 raw*25/8 缩放（motor_usr.c:616）。

ep.bits           = 14;           % MT6816 分辨率
ep.raw_per_rev    = 2^14;         % 16384 raw/圈
ep.scale_mul      = 25;           % 未校准 raw→细分步 乘数（motor_usr.c:617）
ep.scale_div      = 8;            % 未校准 raw→细分步 除数（raw*25/8=51200）
ep.subdiv_per_rev = 51200;        % 细分步/圈（200 整步 * 256 细分）

%% 磁干扰/量化噪声模型（细分步域，加在 rectified 上）
% 固件注释（motor_usr.h）：8/15 实测编码器磁干扰 → 假速度 ±25000~40000 步/s
% noise_std = 1~3 细分步 可复现该量级假速度；0 = 理想编码器
ep.noise_std      = 0;            % 噪声标准差（细分步），默认关

%% 校准状态（true=已写校准表，rectified=细分步空间；false=raw*25/8）
ep.calibrated     = true;

%% 校准残差模拟（8/17 实板诊断：DCE 保持 1500mA/3s 落点残差 -10/+24/+6/+12，
% 电流无关、可复现 → 编码器校准表插值残差。锚点 90° 间隔线性插值 + 周期绕回，
% 对齐实板 4 目标实测值；true 时 run_pos_step_dce 复现实板现象）
ep.calib_residual = false;      % 校准残差开关（默认关，保持理想编码器）
ep.calib_anchor_deg  = [0 90 180 270 360];   % 锚点（度）
ep.calib_res_err     = [12 -10 24 6 12];     % 锚点报数误差（细分步）
end
