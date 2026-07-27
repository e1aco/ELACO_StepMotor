#!/usr/bin/env python3
"""
统一编译入口 — 自动检测项目使用的工具链并调度到对应 builder。

用法:
  python builder.py --project <项目目录>              # 自动检测工具链
  python builder.py --project <项目目录> --keil        # 强制 Keil
  python builder.py --project <项目目录> --cmake       # 强制 CMake/GCC
  python builder.py detect --project <项目目录>        # 只检测不编译

输出:
  JSON 格式 { status, errors, warnings, output, artifact }
"""

import sys
import json
import subprocess
import os
from pathlib import Path


# ── 工具链检测 ──────────────────────────────────────────

DETECT_SIGNALS = {
    'keil':   [('*.uvprojx', False), ('*.uvproj', False)],
    'iar':    [('*.ewp', False)],
    'cmake':  [('CMakeLists.txt', False)],
    'esp_idf': [('sdkconfig', False)],
    'platformio': [('platformio.ini', False)],
}

# 对于 Makefile，需要检查是否含交叉编译链
MAKEFILE_CROSS_PREFIXES = ['arm-none-eabi', 'riscv-none-elf', 'aarch64-none-elf']


def detect_toolchain(project_dir: str) -> str | None:
    """检测项目目录下的工具链特征文件。返回工具链名称或 None。"""
    root = Path(project_dir)

    # 先检查明确的特征文件
    for toolchain, patterns in DETECT_SIGNALS.items():
        for pattern, _ in patterns:
            # 拆分 pattern 为目录和扩展名
            matches = list(root.rglob(pattern))
            if matches:
                return toolchain

    # Makefile 需要检查内容
    for mk in ['Makefile', 'makefile', 'GNUmakefile']:
        mk_path = root / mk
        if mk_path.exists():
            content = mk_path.read_text(errors='replace')
            for prefix in MAKEFILE_CROSS_PREFIXES:
                if prefix in content:
                    return 'cmake'  # 交叉编译 Makefile 归 cmake 处理

    # 检查是否有 .c 文件 + arm gcc 安装（弱信号）
    try:
        result = subprocess.run(
            ['arm-none-eabi-gcc', '--version'],
            capture_output=True, text=True, timeout=5
        )
        if result.returncode == 0:
            # 有 arm gcc 但没特征文件 → 可能手动项目
            c_files = list(root.rglob('*.c'))
            if c_files:
                return 'cmake'
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass

    return None


# ── 调度器 ──────────────────────────────────────────────

BUILDERS = {
    'keil': 'build-keil',
    'cmake': 'build-cmake',
    # 预留
    'iar': None,
    'esp_idf': None,
    'platformio': None,
}


def get_builder_script(toolchain: str, skill_dir: str) -> str | None:
    """返回对应工具链的 builder 脚本路径。"""
    builder = BUILDERS.get(toolchain)
    if not builder:
        return None

    script_dir = Path(skill_dir) / 'tools' / builder / 'scripts'
    scripts = {
        'build-keil': script_dir / 'keil_builder.py',
        'build-cmake': script_dir / 'cmake_builder.py',
    }

    script = scripts.get(builder)
    if script and script.exists():
        return str(script)
    return None


def build(project_dir: str, toolchain: str = None, skill_dir: str = None,
          target: str = None) -> dict:
    """
    编译入口。

    参数:
      project_dir: 项目根目录
      toolchain:   指定工具链（None=自动检测）
      skill_dir:   EM-SKILL 根目录（用于定位 builder 脚本）
      target:      编译目标（工具链相关）

    返回:
      { status: 'ok'|'fail'|'not_found',
        toolchain: str,
        errors: int,
        warnings: int,
        output: str,       # 编译输出摘要
        artifact: str }    # 产物路径（如有）
    """
    if not project_dir:
        return {'status': 'fail', 'error': '未指定项目目录'}

    detected_toolchain = toolchain or detect_toolchain(project_dir)
    if not detected_toolchain:
        return {
            'status': 'not_found',
            'error': '无法自动检测工具链。请指定工具链: --keil / --cmake',
        }

    script = get_builder_script(detected_toolchain, skill_dir or '.')
    if not script:
        return {
            'status': 'not_found',
            'error': f'未找到 {detected_toolchain} 的 builder 脚本',
        }

    # 调用具体 builder
    cmd = [sys.executable, script, '--project', project_dir]
    if target:
        cmd.extend(['--target', target])

    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, timeout=120
        )
        output = result.stdout + result.stderr

        return {
            'status': 'ok' if result.returncode == 0 else 'fail',
            'toolchain': detected_toolchain,
            'returncode': result.returncode,
            'output': output.strip(),
            'errors': _count_pattern(output, 'error'),
            'warnings': _count_pattern(output, 'warning'),
        }
    except subprocess.TimeoutExpired:
        return {'status': 'fail', 'error': '编译超时（>120s）'}
    except Exception as e:
        return {'status': 'fail', 'error': str(e)}


def _count_pattern(text: str, keyword: str) -> int:
    """粗略统计输出中 keyword 出现次数（不精准，仅供摘要参考）。"""
    return sum(1 for line in text.lower().split('\n') if keyword in line)


# ── CLI 入口 ────────────────────────────────────────────

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='EM-SKILL 统一编译入口')
    parser.add_argument('--project', required=True, help='项目目录')
    parser.add_argument('--keil', action='store_true', help='强制 Keil 编译')
    parser.add_argument('--cmake', action='store_true', help='强制 CMake 编译')
    parser.add_argument('--target', help='编译目标（可选）')
    parser.add_argument('--json', action='store_true', help='JSON 格式输出')
    parser.add_argument('--skill-dir', default='.', help='EM-SKILL 根目录')

    args = parser.parse_args()

    # 工具链选择
    toolchain = None
    if args.keil:
        toolchain = 'keil'
    elif args.cmake:
        toolchain = 'cmake'

    if args.project == 'detect' or args.project.endswith('detect'):
        # 只检测
        tc = detect_toolchain(args.project)
        print(json.dumps({'detected_toolchain': tc}))
        sys.exit(0)

    result = build(args.project, toolchain, args.skill_dir, args.target)

    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    else:
        status_emoji = '✅' if result.get('status') == 'ok' else '❌'
        tc = result.get('toolchain', 'unknown')
        print(f'{status_emoji} 编译结果 [{tc}]')
        print(f'   状态: {result.get("status")}')
        if 'errors' in result:
            print(f'   错误: {result["errors"]}  警告: {result["warnings"]}')
        if 'output' in result:
            # 只输出最后 10 行摘要
            lines = result['output'].split('\n')
            print('   输出摘要:')
            for line in lines[-10:]:
                print(f'     {line}')
        if 'error' in result:
            print(f'   错误信息: {result["error"]}')
