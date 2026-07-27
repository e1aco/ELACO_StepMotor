#!/usr/bin/env python3
"""
EM-SKILL PDF Reader — 高精度 PDF 读取工具
==========================================

主引擎: marker (37.7k stars, 基准测试 76%+ 准确率)
  - 支持表格、公式、多栏布局、图片提取
  - 可选 LLM 增强 (--use-llm) 进一步提升精度
增强引擎: Docling (结构化技术文档提取)
  - 表格边界识别更准确
  - 代码块保留格式更好
  - 对中文技术文档支持更好
轻量备份: PyMuPDF (用于 info/search 等快速操作)

用法:
    python pdf_reader.py info <pdf>                          # 元信息 + TOC
    python pdf_reader.py convert <pdf> --pages 1-5           # 高精度转 Markdown (marker)
    python pdf_reader.py convert <pdf> --pages 1-5 --engine docling  # Docling 引擎
    python pdf_reader.py convert <pdf> --pages 1-5 --engine auto     # 自动选择
    python pdf_reader.py convert <pdf> --pages 1-5 --use-llm # LLM 增强模式
    python pdf_reader.py convert <pdf> --format json         # 输出 JSON 格式
    python pdf_reader.py tables <pdf> --pages 1              # 提取表格
    python pdf_reader.py extract <pdf> --pages 1-5           # 快速文本提取 (PyMuPDF)
    python pdf_reader.py images <pdf> --pages 1 --out dir    # 提取图片
    python pdf_reader.py index <pdf>                         # 生成索引摘要
    python pdf_reader.py search <pdf> "关键词"                # 全文搜索
    python pdf_reader.py install                             # 安装依赖

依赖:
    pip install marker-pdf pymupdf
    # 可选 LLM 增强:
    # pip install marker-pdf[all]
    # 可选 Docling 引擎 (更好的表格/代码提取):
    # pip install docling
"""

from __future__ import annotations

import argparse
import io
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Optional

# Force UTF-8 on Windows
if sys.platform == "win32":
    try:
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
        sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")
    except Exception:
        pass


def parse_pages(pages_str: str) -> Optional[str]:
    """Parse page range string, return marker-compatible format."""
    if not pages_str:
        return None
    # marker accepts: "0,5-10,20" (0-based)
    parts = []
    for part in pages_str.split(","):
        if "-" in part:
            start, end = part.split("-", 1)
            # Convert 1-based input to 0-based
            parts.append(f"{int(start)-1}-{int(end)-1}")
        else:
            parts.append(str(int(part) - 1))
    return ",".join(parts)


def check_marker() -> bool:
    """Check if marker-pdf is installed."""
    try:
        from marker.converters.pdf import PdfConverter
        return True
    except ImportError:
        return False


def check_pymupdf() -> bool:
    """Check if pymupdf is installed."""
    try:
        import pymupdf
        return True
    except ImportError:
        return False


def check_docling() -> bool:
    """Check if docling is installed."""
    try:
        from docling.document_converter import DocumentConverter
        return True
    except ImportError:
        return False


def convert_with_docling(pdf_path: str, pages: Optional[str] = None) -> str:
    """使用 Docling 进行高精度提取（更好的表格/代码/公式支持）"""
    import subprocess
    import tempfile

    # 创建临时 Python 脚本来执行 Docling 提取
    script_content = '''
import sys
import json

def extract(pdf_path, pages=None):
    try:
        from docling.document_converter import DocumentConverter
        from docling.datamodel.pipeline_options import PdfPipelineOptions
        from docling.datamodel.base_models import InputFormat
        from docling.document_converter import PdfFormatOption

        pipeline_options = PdfPipelineOptions()
        pipeline_options.do_ocr = False
        pipeline_options.do_table_structure = True

        converter = DocumentConverter(
            format_options={
                InputFormat.PDF: PdfFormatOption(pipeline_options=pipeline_options)
            }
        )

        if pages:
            page_list = []
            for part in pages.split(","):
                if "-" in part:
                    s, e = part.split("-", 1)
                    page_list.extend(range(int(s), int(e) + 1))
                else:
                    page_list.append(int(part))
            result = converter.convert(pdf_path, page_range=page_list)
        else:
            result = converter.convert(pdf_path)

        return result.document.export_to_markdown()
    except Exception as e:
        return f"ERROR:{type(e).__name__}:{str(e)}"

if __name__ == "__main__":
    pdf_path = sys.argv[1]
    pages = sys.argv[2] if len(sys.argv) > 2 else None
    result = extract(pdf_path, pages)
    print(result)
'''

    # 写入临时文件
    with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False, encoding='utf-8') as f:
        f.write(script_content)
        script_path = f.name

    try:
        # 使用子进程执行，捕获输出
        result = subprocess.run(
            [sys.executable, script_path, pdf_path, pages or ""],
            capture_output=True,
            timeout=120
        )

        if result.returncode != 0:
            stderr_text = result.stderr.decode('utf-8', errors='replace')[:500]
            raise RuntimeError(f"Docling 子进程失败: {stderr_text}")

        output = result.stdout.decode('utf-8', errors='replace').strip()
        if output.startswith("ERROR:"):
            # 解析错误信息
            parts = output.split(":", 2)
            if len(parts) >= 3:
                error_type = parts[1]
                error_msg = parts[2]
                raise RuntimeError(f"Docling 提取失败: {error_type} - {error_msg}")
            else:
                raise RuntimeError(f"Docling 提取失败: {output}")

        return output
    finally:
        # 清理临时文件
        try:
            os.unlink(script_path)
        except:
            pass


def cmd_install(args):
    """Install dependencies."""
    print("安装 EM-SKILL PDF Reader 依赖...")
    print()

    # Core: marker-pdf + pymupdf
    print("[1/3] 安装 marker-pdf (高精度 PDF 引擎)...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "marker-pdf", "pymupdf"])
    print()

    # Optional: Docling support
    if args.docling:
        print("[2/3] 安装 Docling (结构化技术文档引擎)...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", "docling"])
    else:
        print("[2/3] 跳过 Docling (需要时用 --docling 安装)")

    # Optional: LLM support
    if args.use_llm:
        print("[3/3] 安装 LLM 增强支持...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", "marker-pdf[all]"])
    else:
        print("[3/3] 跳过 LLM 增强 (需要时用 --use-llm 安装)")

    print()
    print("安装完成!")
    print("  marker-pdf: 高精度 PDF→Markdown (表格/公式/多栏)")
    print("  pymupdf:    快速文本提取/搜索/元信息")
    if args.docling:
        print("  docling:    结构化技术文档提取 (更好的表格/代码/公式)")


def cmd_info(args):
    """Print PDF metadata and TOC (uses PyMuPDF for speed)."""
    if not check_pymupdf():
        print("ERROR: pymupdf 未安装。运行: python pdf_reader.py install", file=sys.stderr)
        sys.exit(2)

    import pymupdf
    doc = pymupdf.open(args.pdf)
    meta = doc.metadata
    toc = doc.get_toc()

    print(f"文件: {args.pdf}")
    print(f"页数: {doc.page_count}")
    print(f"标题: {meta.get('title', '-')}")
    print(f"作者: {meta.get('author', '-')}")
    print(f"主题: {meta.get('subject', '-')}")
    print()

    if toc:
        print("目录:")
        for level, title, page in toc:
            indent = "  " * (level - 1)
            print(f"  {indent}{title} → 页 {page}")
    else:
        print("目录: (无嵌入目录)")

    doc.close()


def cmd_convert(args):
    """High-accuracy PDF to Markdown/JSON conversion using marker or Docling."""
    pdf_path = args.pdf
    output_format = args.format
    page_range = parse_pages(args.pages)
    engine = args.engine
    original_engine = engine  # 保存原始值用于 fallback 判断

    print(f"正在转换: {pdf_path}", file=sys.stderr)
    if page_range:
        print(f"  页码范围: {page_range}", file=sys.stderr)

    # 自动选择引擎
    if engine == "auto":
        if check_docling():
            engine = "docling"
            print(f"  引擎: Docling (自动选择，更好的表格/代码提取)", file=sys.stderr)
        elif check_marker():
            engine = "marker"
            print(f"  引擎: marker (Docling 未安装，fallback)", file=sys.stderr)
        else:
            print("ERROR: Docling 和 marker-pdf 均未安装。运行: python pdf_reader.py install", file=sys.stderr)
            sys.exit(2)

    # Docling 引擎
    if engine == "docling":
        if not check_docling():
            print("ERROR: docling 未安装。安装: pip install docling", file=sys.stderr)
            sys.exit(2)

        if args.use_llm:
            print(f"  模式: Docling + LLM 增强", file=sys.stderr)
        else:
            print(f"  模式: Docling (结构化提取)", file=sys.stderr)

        try:
            text = convert_with_docling(pdf_path, args.pages)
            print(text)
        except Exception as e:
            if original_engine == "auto":
                # auto 模式: 尝试 fallback 到 marker
                print(f"  ⚠️ Docling 失败: {type(e).__name__}", file=sys.stderr)
                if check_marker():
                    print(f"  引擎: marker (fallback)", file=sys.stderr)
                    engine = "marker"
                elif check_pymupdf():
                    print(f"  引擎: PyMuPDF (fallback)", file=sys.stderr)
                    # 使用 PyMuPDF 快速提取
                    import pymupdf
                    doc = pymupdf.open(pdf_path)
                    for pg in range(doc.page_count):
                        print(f"--- 页 {pg + 1} ---")
                        print(doc[pg].get_text("text"))
                    doc.close()
                    print("转换完成!", file=sys.stderr)
                    return
                else:
                    print("ERROR: 所有引擎均失败", file=sys.stderr)
                    sys.exit(2)
            else:
                raise

    # Marker 引擎 (默认)
    if engine == "marker":
        if not check_marker():
            print("ERROR: marker-pdf 未安装。运行: python pdf_reader.py install", file=sys.stderr)
            sys.exit(2)

        from marker.converters.pdf import PdfConverter
        from marker.models import create_model_dict
        from marker.output import text_from_rendered

        if args.use_llm:
            print(f"  模式: LLM 增强 (更高精度)", file=sys.stderr)
        else:
            print(f"  模式: 标准 (本地模型)", file=sys.stderr)

        # Build converter config
        config = {"output_format": output_format}
        if page_range:
            config["page_range"] = page_range
        if args.use_llm:
            config["use_llm"] = True

        converter = PdfConverter(
            artifact_dict=create_model_dict(),
        )
        rendered = converter(pdf_path)

        if output_format == "markdown":
            text, _, images = text_from_rendered(rendered)
            print(text)
            if images:
                print(f"\n<!-- 提取了 {len(images)} 张图片 -->", file=sys.stderr)
        elif output_format == "json":
            print(json.dumps(rendered.model_dump(), ensure_ascii=False, indent=2, default=str))
        else:
            text, _, _ = text_from_rendered(rendered)
            print(text)

    print("转换完成!", file=sys.stderr)


def cmd_tables(args):
    """Extract tables using marker's TableConverter."""
    if not check_marker():
        print("ERROR: marker-pdf 未安装。运行: python pdf_reader.py install", file=sys.stderr)
        sys.exit(2)

    from marker.converters.table import TableConverter
    from marker.models import create_model_dict
    from marker.output import text_from_rendered

    pdf_path = args.pdf
    page_range = parse_pages(args.pages)

    print(f"正在提取表格: {pdf_path}", file=sys.stderr)

    config = {"output_format": "json"}
    if page_range:
        config["page_range"] = page_range

    converter = TableConverter(
        artifact_dict=create_model_dict(),
    )
    rendered = converter(pdf_path)
    text, _, _ = text_from_rendered(rendered)
    print(text)


def cmd_extract(args):
    """Quick text extraction using PyMuPDF (fallback, less accurate)."""
    if not check_pymupdf():
        print("ERROR: pymupdf 未安装。运行: python pdf_reader.py install", file=sys.stderr)
        sys.exit(2)

    import pymupdf
    doc = pymupdf.open(args.pdf)
    pages_str = args.pages
    if pages_str:
        # Parse 1-based page ranges
        page_list = []
        for part in pages_str.split(","):
            if "-" in part:
                s, e = part.split("-", 1)
                page_list.extend(range(int(s) - 1, int(e)))
            else:
                page_list.append(int(part) - 1)
    else:
        page_list = list(range(doc.page_count))

    for pg in page_list:
        if pg < 0 or pg >= doc.page_count:
            continue
        page = doc[pg]
        text = page.get_text("text")
        print(f"--- 页 {pg + 1} ---")
        print(text)

    doc.close()


def cmd_images(args):
    """Extract images from PDF pages."""
    if not check_pymupdf():
        print("ERROR: pymupdf 未安装。运行: python pdf_reader.py install", file=sys.stderr)
        sys.exit(2)

    import pymupdf
    doc = pymupdf.open(args.pdf)
    out_dir = Path(args.out) if args.out else Path("images")
    out_dir.mkdir(parents=True, exist_ok=True)

    pages_str = args.pages
    if pages_str:
        page_list = []
        for part in pages_str.split(","):
            if "-" in part:
                s, e = part.split("-", 1)
                page_list.extend(range(int(s) - 1, int(e)))
            else:
                page_list.append(int(part) - 1)
    else:
        page_list = list(range(doc.page_count))

    count = 0
    for pg in page_list:
        if pg < 0 or pg >= doc.page_count:
            continue
        page = doc[pg]
        for img_index, img in enumerate(page.get_images()):
            xref = img[0]
            pix = pymupdf.Pixmap(doc, xref)
            if pix.n > 4:
                pix = pymupdf.Pixmap(pymupdf.csRGB, pix)
            fname = out_dir / f"page{pg + 1}_img{img_index + 1}.png"
            pix.save(str(fname))
            count += 1
            print(f"  保存: {fname}")

    doc.close()
    print(f"共提取 {count} 张图片到 {out_dir}/")


def cmd_index(args):
    """Generate index summary for require-index.md."""
    if not check_pymupdf():
        print("ERROR: pymupdf 未安装。运行: python pdf_reader.py install", file=sys.stderr)
        sys.exit(2)

    import pymupdf
    doc = pymupdf.open(args.pdf)
    toc = doc.get_toc()
    fname = Path(args.pdf).name

    print(f"### {fname}")
    print(f"| 页范围 | 内容 |")
    print(f"|--------|------|")

    if toc:
        for i, (level, title, page) in enumerate(toc):
            next_page = toc[i + 1][2] if i + 1 < len(toc) else doc.page_count + 1
            print(f"| 页 {page}-{next_page - 1} | {title} |")
    else:
        # No TOC, extract first line of each chunk as hint
        chunk_size = 10
        for start in range(0, doc.page_count, chunk_size):
            end = min(start + chunk_size, doc.page_count)
            first_text = doc[start].get_text("text").strip().split("\n")[0][:60]
            hint = f" — {first_text}..." if first_text else ""
            print(f"| 页 {start + 1}-{end}{hint} |")

    doc.close()


def cmd_search(args):
    """Search for text in PDF."""
    if not check_pymupdf():
        print("ERROR: pymupdf 未安装。运行: python pdf_reader.py install", file=sys.stderr)
        sys.exit(2)

    import pymupdf
    doc = pymupdf.open(args.pdf)
    keyword = args.keyword
    results = []

    for pg_num in range(doc.page_count):
        page = doc[pg_num]
        hits = page.search_for(keyword)
        if hits:
            text = page.get_text("text")
            for rect in hits:
                lines = text.split("\n")
                for line in lines:
                    if keyword.lower() in line.lower():
                        results.append({
                            "page": pg_num + 1,
                            "text": line.strip()[:200]
                        })
                        break

    doc.close()

    if results:
        print(f"找到 {len(results)} 处匹配:")
        for r in results:
            print(f"  页 {r['page']}: {r['text']}")
    else:
        print(f"未找到: {keyword}")


def main():
    parser = argparse.ArgumentParser(
        prog="em-pdf-reader",
        description="EM-SKILL 高精度 PDF 读取工具 (marker + PyMuPDF)",
    )
    parser.add_argument("--use-llm", action="store_true",
                        help="LLM 增强模式 (需额外安装 marker-pdf[all])")
    sub = parser.add_subparsers(dest="command", help="子命令")

    # install
    p_install = sub.add_parser("install", help="安装依赖")
    p_install.add_argument("--docling", action="store_true",
                           help="同时安装 Docling 引擎 (更好的表格/代码提取)")
    p_install.add_argument("--use-llm", action="store_true",
                           help="同时安装 LLM 增强支持")

    # info
    p_info = sub.add_parser("info", help="PDF 元信息 + TOC (PyMuPDF)")
    p_info.add_argument("pdf", help="PDF 文件路径")

    # convert (main high-accuracy command)
    p_convert = sub.add_parser("convert", help="高精度转 Markdown/JSON (marker/Docling)")
    p_convert.add_argument("pdf", help="PDF 文件路径")
    p_convert.add_argument("--pages", help="页码范围，如 1-5 或 1,3,5 (0-based 自动转换)")
    p_convert.add_argument("--format", choices=["markdown", "json", "html"],
                           default="markdown", help="输出格式 (默认: markdown)")
    p_convert.add_argument("--engine", choices=["marker", "docling", "auto"],
                           default="marker", help="提取引擎: marker(默认), docling(更好的表格/代码), auto(自动选择)")
    p_convert.add_argument("--use-llm", action="store_true",
                           help="LLM 增强模式 (表格合并/公式/表单)")

    # tables
    p_tables = sub.add_parser("tables", help="提取表格 (marker TableConverter)")
    p_tables.add_argument("pdf", help="PDF 文件路径")
    p_tables.add_argument("--pages", help="页码范围")

    # extract (quick fallback)
    p_extract = sub.add_parser("extract", help="快速文本提取 (PyMuPDF, 备用)")
    p_extract.add_argument("pdf", help="PDF 文件路径")
    p_extract.add_argument("--pages", help="页码范围")

    # images
    p_images = sub.add_parser("images", help="提取图片")
    p_images.add_argument("pdf", help="PDF 文件路径")
    p_images.add_argument("--pages", help="页码范围")
    p_images.add_argument("--out", help="输出目录 (默认: images)")

    # index
    p_index = sub.add_parser("index", help="生成索引摘要")
    p_index.add_argument("pdf", help="PDF 文件路径")

    # search
    p_search = sub.add_parser("search", help="全文搜索 (PyMuPDF)")
    p_search.add_argument("pdf", help="PDF 文件路径")
    p_search.add_argument("keyword", help="搜索关键词")

    args = parser.parse_args()
    if not args.command:
        parser.print_help()
        return 1

    commands = {
        "install": cmd_install,
        "info": cmd_info,
        "convert": cmd_convert,
        "tables": cmd_tables,
        "extract": cmd_extract,
        "images": cmd_images,
        "index": cmd_index,
        "search": cmd_search,
    }
    commands[args.command](args)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as e:
        print(f"ERROR: {type(e).__name__}: {e}", file=sys.stderr)
        sys.exit(1)
