"""
Keil 工程配置工具 — 将新 .c 文件同步到 .uvprojx 工程

自动扫描 ELA_LIB/ 和 ModTest/ 下的 .c 文件，同步到
对应 Group（找不到 Group 则新建）。同时检查 IncludePath。

用法:
  python keil_config.py --project <工程.uvprojx>
  python keil_config.py --scan <项目目录>     # 自动扫描 .uvprojx
  python keil_config.py --project <工程.uvprojx> --dry-run  # 预览
"""

import sys
import os
from pathlib import Path
import xml.etree.ElementTree as ET

PROJECT_EXTENSIONS = {".uvprojx", ".uvproj"}

# (group_name, source_dir_from_uvprojx, include_path)
SYNC_GROUPS = [
    ("ELA_LIB", "../ELA_LIB", "../ELA_LIB"),
    ("ModTest", "../ModTest", "../ModTest"),
]


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


def get_c_files(source_dir: Path) -> list[Path]:
    """获取指定目录下所有 .c 文件"""
    if not source_dir.exists():
        return []
    return sorted(source_dir.rglob("*.c"))


def sync_group_files(group_elem, proj_dir, source_dir) -> tuple:
    """
    同步单个 Group 的文件列表。
    返回 (added_list, skipped_list)。
    """
    existing = set()
    for files_elem in group_elem.findall("Files"):
        for file_elem in files_elem.findall("File"):
            fp = file_elem.findtext("FilePath", "")
            if fp:
                existing.add(fp.replace("/", "\\"))

    c_files = get_c_files(source_dir)
    added = []
    skipped = []

    for c_file in c_files:
        rel = os.path.relpath(c_file, proj_dir).replace("/", "\\")
        if rel in existing:
            skipped.append(rel)
            continue
        files_elem = group_elem.find("Files")
        if files_elem is None:
            files_elem = ET.SubElement(group_elem, "Files")
        file_elem = ET.SubElement(files_elem, "File")
        fp_elem = ET.SubElement(file_elem, "FilePath")
        fp_elem.text = rel
        ft_elem = ET.SubElement(file_elem, "FileType")
        ft_elem.text = "1"
        added.append(rel)

    return added, skipped


def find_or_create_group(groups_elem, group_name: str):
    """在 groups_elem 下查找 Group，找不到则新建"""
    for group in groups_elem.findall("Group"):
        gname = group.findtext("GroupName", "")
        if gname.strip() == group_name:
            return group, False
    new_group = ET.SubElement(groups_elem, "Group")
    name_elem = ET.SubElement(new_group, "GroupName")
    name_elem.text = group_name
    return new_group, True


def check_include_path(target_elem, include_path: str) -> bool:
    """检查并追加 IncludePath，返回是否修改"""
    tdef_elem = target_elem.find("TargetOption/TargetArmAds/Cads")
    if tdef_elem is None:
        return False
    inc_elem = tdef_elem.find("VariousControls/IncludePath")
    if inc_elem is None or inc_elem.text is None:
        return False
    paths = inc_elem.text.replace("\r\n", ";").replace("\n", ";").split(";")
    paths = [p.strip() for p in paths]
    if include_path not in paths and include_path.replace("/", "\\") not in paths:
        paths.append(include_path)
        inc_elem.text = ";".join(paths)
        return True
    return False


def sync_project(project_path: Path, dry_run: bool = False) -> bool:
    """同步单个 .uvprojx，返回是否有变"""
    if not project_path.exists():
        print(f"  ❌ 工程文件不存在: {project_path}")
        return False

    proj_dir = project_path.parent

    try:
        tree = ET.parse(project_path)
    except ET.ParseError as exc:
        print(f"  ❌ 解析失败: {exc}")
        return False

    root = tree.getroot()
    modified = False

    for target in root.iter("Target"):
        target_name = target.findtext("TargetName", "?")
        groups_elem = target.find("Groups")
        if groups_elem is None:
            continue

        for group_name, rel_dir, inc_path in SYNC_GROUPS:
            source_dir = (proj_dir / rel_dir).resolve()
            if not source_dir.exists():
                continue

            group_elem, is_new = find_or_create_group(groups_elem, group_name)
            if is_new:
                print(f"  📁 Target [{target_name}]: 新建 {group_name} 组")
                modified = True

            added, skipped = sync_group_files(
                group_elem, proj_dir, source_dir)

            if added:
                print(f"  ✅ Target [{target_name}] {group_name}: "
                      f"新增 {len(added)} 个文件")
                for f in added:
                    print(f"     + {f}")
                modified = True
            if skipped:
                print(f"  💡 Target [{target_name}] {group_name}: "
                      f"{len(skipped)} 个已存在，跳过")

    for target in root.iter("Target"):
        target_name = target.findtext("TargetName", "?")
        for group_name, rel_dir, inc_path in SYNC_GROUPS:
            source_dir = (proj_dir / rel_dir).resolve()
            if not source_dir.exists():
                continue
            if check_include_path(target, inc_path):
                print(f"  ✅ Target [{target_name}]: "
                      f"IncludePath 追加 {inc_path}")
                modified = True

    if modified and not dry_run:
        tree.write(project_path, encoding="utf-8", xml_declaration=True)
        print(f"  💾 已保存: {project_path.name}")
    elif modified and dry_run:
        print(f"  🔍 预览模式: 将修改 {project_path.name}")
    else:
        print(f"  ✅ 无需修改: {project_path.name}")

    return modified


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Keil 工程配置工具")
    parser.add_argument("--project",
                        help=".uvprojx 或 .uvproj 工程文件路径")
    parser.add_argument("--scan",
                        help="扫描指定目录中的 Keil 工程文件")
    parser.add_argument("--dry-run", action="store_true",
                        help="预览模式，不写文件")
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
