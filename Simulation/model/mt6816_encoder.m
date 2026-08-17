function rectified = mt6816_encoder(ep, theta_mech_rad)
%% mt6816_encoder.m — 复刻编码器路径（MT6816 14bit 绝对角 + raw*25/8 映射）
% 输入: ep 编码器参数（encoder_params.m）
%       theta_mech_rad 真实机械角 (rad)
% 输出: rectified 角度（0~51199 细分步，与固件 USR_MT6816_GetRectifiedAngle 同域）
% 说明: 1. 14bit 量化 raw = round(theta/2pi*16384)
%       2. 校准表存在：理想比例映射到 51200 细分步
%          （实际校准表为每整步锚点+插值，这里理想化；噪声用细分步域高斯模拟磁干扰）
%       3. 未校准：raw*25/8 线性缩放（motor_usr.c:616）
% 依据: require.md 编码器 MT6816-ACD；motor_usr.c 校准/缩放路径

raw = mod(round(theta_mech_rad / (2*pi) * ep.raw_per_rev), ep.raw_per_rev);

if ep.calibrated
    % 校准表理想映射（0~16383 -> 0~51199）
    rectified = mod(round(raw * ep.subdiv_per_rev / ep.raw_per_rev), ep.subdiv_per_rev);
else
    % 未校准线性缩放（motor_usr.c:616-618）
    rectified = mod(round(raw * ep.scale_mul / ep.scale_div), ep.subdiv_per_rev);
end

% 磁干扰/微振噪声（细分步域，模拟固件注释中编码器假速度现象）
if ep.noise_std > 0
    rectified = mod(rectified + round(ep.noise_std * randn), ep.subdiv_per_rev);
end

% 校准残差（8/17 实板诊断）：锚点线性插值（90° 间隔）+ 周期绕回，
% 模拟校准表插值残差；默认关（理想编码器）
if isfield(ep, 'calib_residual') && ep.calib_residual
    n  = numel(ep.calib_res_err);
    xf = rectified / ep.subdiv_per_rev * (n - 1);
    i  = min(floor(xf), n - 2);
    f  = xf - i;
    res = ep.calib_res_err(i + 1) * (1 - f) + ep.calib_res_err(i + 2) * f;
    rectified = mod(rectified + round(res), ep.subdiv_per_rev);
end
end
