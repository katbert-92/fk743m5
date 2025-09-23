import argparse
import json
import logging
import sys
from pathlib import Path

from elf_map_parse import ElfParser, MapParser
from tabulate import tabulate

DEFAULT_BAR_LENGTH = 50


def parse_args():
    parser = argparse.ArgumentParser(description="ELF file analysis")
    parser.add_argument("-e", "--elf_path", type=Path, required=True, help="Path to the ELF file")
    parser.add_argument("-m", "--map_path", type=Path, required=True, help="Path to the MAP file")
    parser.add_argument(
        "-o", "--output_path", type=Path, required=True, help="Path to the output file"
    )

    return parser.parse_args()


def compute_memory_usage(map_regions_info, elf_sections_info):
    usage = {k: 0 for k in map_regions_info}
    flash_to_ram_size = 0

    for key, value in elf_sections_info.items():
        if not value["sh_flags"] & 0x2:  # SHF_ALLOC
            continue

        size = value["sh_size"]
        if size == 0:
            continue

        if key == ".data":
            flash_to_ram_size += size

        sec_start = value["sh_addr"]
        sec_end = sec_start + size

        for region_key, region_val in map_regions_info.items():
            reg_start = region_val["origin"]
            reg_end = reg_start + region_val["length"]

            overlap_start = max(sec_start, reg_start)
            overlap_end = min(sec_end, reg_end)

            if overlap_start < overlap_end:
                usage[region_key] += overlap_end - overlap_start

    usage["FLASH"] += flash_to_ram_size  # Account for .data init

    result = {}
    for region, used in usage.items():
        total = map_regions_info[region]["length"]
        pcnt = (used / total) * 100 if total > 0 else 0
        result[region] = {"used": used, "total": total, "pcnt": pcnt}

    return result


def make_progress_bar(pcnt, length=DEFAULT_BAR_LENGTH):
    filled = int(length * pcnt / 100)
    return "█" * filled + "░" * (length - filled)


def print_map_info(map_regions_info):
    table = [
        [name, f"0x{region['origin']:08X}", region["length"] / 1024]
        for name, region in map_regions_info.items()
    ]
    headers = ["Region", "Origin", "Size (KB)"]
    print(tabulate(table, headers=headers, tablefmt="grid"))


def print_elf_sections_info(elf_sections_info):
    table = [
        [
            sec,
            sec_data["sh_name"],
            sec_data["sh_type"],
            sec_data["sh_flags"],
            f"0x{sec_data['sh_addr']:08X}",
            f"0x{sec_data['sh_offset']:X}",
            round(sec_data["sh_size"] / 1024, 2),
            sec_data["sh_addralign"],
        ]
        for sec, sec_data in elf_sections_info.items()
    ]
    headers = ["Section", "Name", "Type", "Flags", "Addr", "Offset", "Size (KB)", "Align"]
    print(tabulate(table, headers=headers, tablefmt="grid"))


def print_memory_usage(region_usage):
    table = [
        [
            region,
            f"{info['total'] / 1024:.2f}",
            f"{info['used'] / 1024:.2f}",
            f"{info['pcnt']:.2f}%",
            make_progress_bar(info["pcnt"]),
        ]
        for region, info in region_usage.items()
    ]
    headers = ["Region", "Total (KB)", "Used (KB)", "Usage %", "Progress"]
    colalign = ("left", "left", "left", "left", "center")
    print(tabulate(table, headers=headers, tablefmt="grid", colalign=colalign))


def analyze_fw(elf_path: Path, map_path: Path):
    map_parser = MapParser(map_path)
    map_info = map_parser.get_regions_info()

    elf_parser = ElfParser(elf_path)
    elf_info = elf_parser.get_sections_info()

    # Optional debug outputs:
    # print_map_info(map_info)
    # print_elf_sections_info(elf_info)

    usage = compute_memory_usage(map_info, elf_info)
    print_memory_usage(usage)

    return usage


def main():
    logging.basicConfig(level=logging.INFO)
    args = parse_args()

    logging.info(f"Analyzing ELF: {args.elf_path}, MAP: {args.map_path}")

    try:
        res = analyze_fw(args.elf_path, args.map_path)
        with args.output_path.open("w", encoding="utf-8") as f:
            json.dump(res, f, ensure_ascii=False, indent=4)
        logging.info(f"Data written to {args.output_path}")
    except Exception as e:
        logging.error(f"An error occurred: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
