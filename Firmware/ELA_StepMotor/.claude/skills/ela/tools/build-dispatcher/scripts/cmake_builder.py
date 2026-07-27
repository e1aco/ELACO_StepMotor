#!/usr/bin/env python3
"""
CMake / Makefile 编译脚本 — 用于 arm-none-eabi-gcc 交叉编译项目。

用法:
  python cmake_builder.py --project <项目目录>           # 自动执行 cmake + make
  python cmake_builder.py --project <项目目录> --target clean  # 清理
  python cmake_builder.py --project <项目目录> --make-only      # 只 make，不 cmake
"""

import sys
import subprocess
import os
import json
from pathlib import Path


def find_build_dir(project_dir: str) -> str:
    """查找或确定构建目录。"""
    candidates = [
        os.path.join(project_dir, 'build'),
        os.path.join(project_dir, 'Build'),
        os.path.join(project_dir, 'cmake-build-debug'),
        os.path.join(project_dir, 'cmake-build-release'),
    ]
    for d in candidates:
        if os.path.isdir(d) and os.path.exists(os.path.join(d, 'CMakeCache.txt')):
            return d
    return candidates[0]


def run_cmake(project_dir: str, build_dir: str) -> subprocess.CompletedProcess:
    """运行 CMake 配置。"""
    cmd = ['cmake', '-B', build_dir, '-S', project_dir,
           '-DCMAKE_TOOLCHAIN_FILE=' + find_toolchain_file(project_dir)]
    print(f"🔧 cmake: {' '.join(cmd)}")
    return subprocess.run(cmd, capture_output=True, text=True, timeout=60)


def run_make(build_dir: str, target: str = None) -> subprocess.CompletedProcess:
    """运行 make / ninja。"""
    # 检测用 cmake 还是 ninja
    if os.path.exists(os.path.join(build_dir, 'build.ninja')):
        builder = 'ninja'
    else:
        builder = 'make'

    cmd = [builder]
    if target:
        cmd.append(target)

    print(f"🔧 {builder}: {' '.join(cmd)}")
    return subprocess.run(cmd, capture_output=True, text=True, timeout=120, cwd=build_dir)


def find_toolchain_file(project_dir: str) -> str:
    """查找项目中的 cmake 工具链文件。"""
    expected = os.path.join(project_dir, 'cmake', 'arm-none-eabi-gcc.cmake')
    if os.path.exists(expected):
        return expected
    return 'arm-none-eabi-gcc.cmake'  # 让 cmake 自己找


def find_artifact(build_dir: str, project_dir: str) -> str | None:
    """查找编译产物（.elf / .axf / .bin / .hex）。"""
    for ext in ['.elf', '.axf', '.bin', '.hex']:
        for f in Path(build_dir).rglob(f'*{ext}'):
            return str(f)
    # 在项目根找
    for ext in ['.elf', '.axf', '.bin', '.hex']:
        for f in Path(project_dir).rglob(f'*{ext}'):
            return str(f)
    return None


def build(project_dir: str, target: str = None, make_only: bool = False) -> dict:
    """执行 CMake 构建。"""
    build_dir = find_build_dir(project_dir)

    # 步骤 1: cmake 配置（除非 --make-only）
    if not make_only:
        # .c 文件比 CMakeCache.txt 新，才需要重新 cmake
        need_cmake = True
        cache_file = os.path.join(build_dir, 'CMakeCache.txt')
        if os.path.exists(cache_file):
            cache_mtime = os.path.getmtime(cache_file)
            src_files = list(Path(project_dir).rglob('*.c')) + \
                        list(Path(project_dir).rglob('*.cpp'))
            if src_files:
                latest_src = max(os.path.getmtime(f) for f in src_files)
                need_cmake = latest_src > cache_mtime

        if need_cmake:
            result = run_cmake(project_dir, build_dir)
            if result.returncode != 0:
                return {
                    'status': 'fail',
                    'stage': 'cmake',
                    'output': result.stdout + result.stderr,
                    'error': f'CMake 配置失败 (returncode={result.returncode})',
                }
            print(f"✅ cmake 配置完成")
        else:
            print(f"⏭️ CMakeCache.txt 已是最新，跳过 cmake 配置")

    # 步骤 2: make/ninja 编译
    result = run_make(build_dir, target)
    output = result.stdout + result.stderr

    if result.returncode == 0:
        artifact = find_artifact(build_dir, project_dir)
        return {
            'status': 'ok',
            'returncode': 0,
            'output': output.strip(),
            'artifact': artifact or '未找到产物',
            'errors': 0,
            'warnings': output.lower().count('warning:'),
        }
    else:
        return {
            'status': 'fail',
            'stage': 'build',
            'returncode': result.returncode,
            'output': output.strip(),
            'error': f'编译失败',
            'errors': output.lower().count('error:'),
            'warnings': output.lower().count('warning:'),
        }


# ── CLI 入口 ────────────────────────────────────────────

def main():
    import argparse
    parser = argparse.ArgumentParser(description='EM-SKILL CMake/GCC 编译脚本')
    parser.add_argument('--project', required=True, help='项目目录')
    parser.add_argument('--target', help='编译目标')
    parser.add_argument('--make-only', action='store_true', help='只 make，不 cmake')
    parser.add_argument('--json', action='store_true', help='JSON 格式输出')

    args = parser.parse_args()

    if not os.path.isdir(args.project):
        print(f'❌ 项目目录不存在: {args.project}')
        sys.exit(1)

    result = build(args.project, args.target, args.make_only)

    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    else:
        emoji = '✅' if result['status'] == 'ok' else '❌'
        print(f'{emoji} CMake 编译')
        print(f'   状态: {result["status"]}')
        if result.get('stage'):
            print(f'   阶段: {result["stage"]}')
        if 'errors' in result:
            print(f'   错误: {result["errors"]}  警告: {result["warnings"]}')
        if 'artifact' in result and result['artifact']:
            print(f'   产物: {result["artifact"]}')
        if 'error' in result:
            print(f'   信息: {result["error"]}')

    sys.exit(0 if result['status'] == 'ok' else 1)


if __name__ == '__main__':
    main()
