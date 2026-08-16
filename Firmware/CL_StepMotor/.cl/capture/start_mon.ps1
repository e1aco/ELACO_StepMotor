$ErrorActionPreference = "Stop"
Start-Process -FilePath "C:/Users/electronic/AppData/Local/Programs/Python/Python312/python.exe" -ArgumentList @(
    "E:/Desktop/XM/cl_skill/tools/serial_monitor.py",
    "--port", "COM19",
    "--baud", "115200",
    "--duration", "0",
    "--timestamps",
    "--output", ".cl/capture/capture.log",
    "--watch", "MODE_POSITION_90||MODE_CURRENT_STALL_SIM||MODE_VELOCITY_HALF||T:.*,.*,.*,3,4||T:.*,.*,.*,4,4",
    "--status", ".cl/capture/status",
    "--marker", ".cl/capture/flash_marker"
) -WindowStyle Hidden -WorkingDirectory "E:\Desktop\XM\ELACO_StepMotor\Firmware\CL_StepMotor" -RedirectStandardOutput ".cl/capture/mon_out.log" -RedirectStandardError ".cl/capture/mon_err.log"