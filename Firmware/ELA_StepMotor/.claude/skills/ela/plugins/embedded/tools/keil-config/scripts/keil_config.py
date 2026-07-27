"""
Keil 工程配置工具 — 将 ELA_LIB/ 下的新 .c 文件同步到 .uvprojx 工程

用法:
  python keil_config.py --project <工程.uvprojx>
  python keil_config.py --scan <项目目录>     # 自动扫描 .uvprojx

功能:
  1. 扫描 ELA_LIB/*.c（相对于 .uvprojx 的 ../ELA_LIB/）
  2. 检查 .uvprojx 的 <Group>ELA_LIB</Group> 中是否已有
  3. 无则追加 <File> 节点
  4. 检查 <IncludePath> 是否包含 ../ELA_LIB，无则追加
"""

import sys
import os
from pathlib import Path
import xml.etree.ElementTree as ET

ELA_LIB_GROUP = "ELA_LIB"
INCLUDE_PATH = "../ELA_LIB"
PROJECT_EXTENSIONS = {".uvprojx", ".uvproj"}


def find_projects(scan_dir: str) -> list[Path]:
    """扫描目录下的所有 Keil 工程文件"""
    results = []
    for root, dirs, files in os.walk(scan_dir):
        for fname in files:
            p = Path(root) / fname
            if p.suffix.lower() in PROJECT_EXTENSIONS:
                results.append(p)
    results.sort(key=lambda p: (p.suffix != ".uvprojx", str(p)))
    return results


def get_ela_lib_c_files(project_path: Path) -> list[Path]:
    """获取 ELA_LIB/ 下所有 .c 文件（相对于 .uvprojx 的 ../ELA_LIB/）"""
    ela_lib_dir = project_path.parent.parent / "ELA_LIB"
    if not ela_lib_dir.exists():
        return []
    return sorted(ela_lib_dir.rglob("*.c"))


def sync_project(project_path: Path, dry_run: bool = False) -> bool:
    """同步单个 .uvprojx，返回是否有变"""
    if not project_path.exists():
        print(f"  ❌ 工程文件不存在: {project_path}")
        return False

    proj_dir = project_path.parent
    c_files = get_ela_lib_c_files(project_path)
    if not c_files:
        print(f"  ⏭️  ELA_LIB/ 为空或无 .c 文件，跳过")
        return False

    print(f"  📂 发现 {len(c_files)} 个 .c 文件")

    try:
        tree = ET.parse(project_path)
    except ET.ParseError as exc:
        print(f"  ❌ 解析失败: {exc}")
        return False

    root = tree.getroot()
    modified = False

    # ---- 遍历所有 Target，每个 Target 都可能有自己的 Groups ----
    for target in root.iter("Target"):
        target_name = target.findtext("TargetName", "?")
        groups_elem = target.find("Groups")
        if groups_elem is None:
            continue

        # 找 ELA_LIB Group
        ela_group = None
        for group in groups_elem.findall("Group"):
            gname = group.findtext("GroupName", "")
            if gname.strip() == ELA_LIB_GROUP:
                ela_group = group
                break

        if ela_group is None:
            # 没有 ELA_LIB 组 → 新建
            print(f"  📁 Target [{target_name}]: 新建 {ELA_LIB_GROUP} 组")
            new_group = ET.SubElement(groups_elem, "Group")
            name_elem = ET.SubElement(new_group, "GroupName")
            name_elem.text = ELA_LIB_GROUP
            ela_group = new_group
            modified = True

        # 获取 ELA_LIB 组中已有的文件路径集合
        existing = set()
        for files_elem in ela_group.findall("Files"):
            for file_elem in files_elem.findall("File"):
                fp = file_elem.findtext("FilePath", "")
                if fp:
                    existing.add(fp.replace("/", "\\"))

        # 已有的相对路径列表（用于输出对比）
        added = []
        skipped = []
        for c_file in c_files:
            rel = os.path.relpath(c_file, proj_dir).replace("/", "\\")
            if rel in existing:
                skipped.append(rel)
                continue
            # 新增 File 节点
            files_elem = ela_group.find("Files")
            if files_elem is None:
                files_elem = ET.SubElement(ela_group, "Files")
            file_elem = ET.SubElement(files_elem, "File")
            fp_elem = ET.SubElement(file_elem, "FilePath")
            fp_elem.text = rel
            ft_elem = ET.SubElement(file_elem, "FileType")
            ft_elem.text = "1"  # 1 = C 源文件
            added.append(rel)
            modified = True

        if added:
            print(f"  ✅ Target [{target_name}]: 新增 {len(added)} 个文件")
            for f in added:
                print(f"     + {f}")
        if skipped:
            print(f"  💡 Target [{target_name}]: {len(skipped)} 个已存在，跳过")

    # ---- IncludePath 检查 ----
    for target in root.iter("Target"):
        target_name = target.findtext("TargetName", "?")
        tdef_elem = target.find("TargetOption/TargetArmAds/Cads")
        if tdef_elem is None:
            continue
        inc_elem = tdef_elem.find("VariousControls/IncludePath")
        if inc_elem is None or inc_elem.text is None:
            continue
        paths = inc_elem.text.replace("\r\n", ";").replace("\n", ";").split(";")
        paths = [p.strip() for p in paths]
        if INCLUDE_PATH not in paths and INCLUDE_PATH.replace("/", "\\") not in paths:
            paths.append(INCLUDE_PATH)
            inc_elem.text = ";".join(paths)
            modified = True
            print(f"  ✅ Target [{target_name}]: IncludePath 追加 {INCLUDE_PATH}")

    if modified and not dry_run:
        # 写回（保留 Keil 的 UTF-8 编码）
        tree.write(project_path, encoding="utf-8", xml_declaration=True)
        print(f"  💾 已保存: {project_path.name}")
    elif modified and dry_run:
        print(f"  🔍 预览模式: 将修改 {project_path.name}")
    else:
        print(f"  ✅ 无需修改: {project_path.name}")

    return modified


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Keil 工程配置工具 — ELA_LIB 文件同步")
    parser.add_argument("--project", help=".uvprojx 或 .uvproj 工程文件路径")
    parser.add_argument("--scan", help="扫描指定目录中的 Keil 工程文件")
    parser.add_argument("--dry-run", action="store_true", help="预览模式，不写文件")
    args = parser.parse_args()

    projects = []
    if args.project:
        projects.append(Path(args.project))
    elif args.scan:
        projects = find_projects(args.scan)
        if not projects:
            print(f"❌ 在 {args.scan} 中未找到 Keil 工程文件")
            sys.exit(1)
        print(f"📋 找到 {len(projects)} 个 Keil 工程文件：")
        for p in projects:
            print(f"   {p}")
    else:
        # 默认：扫描当前目录
        projects = find_projects(".")
        if not projects:
            print("❌ 当前目录未找到 Keil 工程文件")
            sys.exit(1)

    any_modified = False
    for proj in projects:
        print(f"\n🔧 {proj.name}")
        if sync_project(proj, dry_run=args.dry_run):
            any_modified = True

    if any_modified:
        print(f"\n✅ Keil 工程配置完成")
    else:
        print(f"\n✅ 已是最新，无需修改")


if __name__ == "__main__":
    main()
