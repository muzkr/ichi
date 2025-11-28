#!/usr/bin/env python

import argparse
from uf2utils import UF2File
from uf2utils.block import DataBlock
from uf2utils.header import Header


def extract(
    input: str, addr: int | None = None, size: int | None = None
) -> UF2File | None:

    uf2 = UF2File(input)

    if addr is None:
        addr1 = 0
    else:
        addr1 = addr

    blocks = []
    for block in uf2:
        if block.header.address < addr1:
            continue
        if size is not None and block.header.address >= addr1 + size:
            continue
        blocks.append(block)

    # blocks = sorted(blocks, key=lambda b: b.header.address)

    if 0 == len(blocks):
        return None

    uf2_out = UF2File()
    if uf2.family:
        uf2_out.family = uf2.family

    for block in blocks:
        uf2_out.append_payload(block.payload, block.header.address)

    uf2_out.sort_blocks()
    uf2_out.renumber_blocks()

    return uf2_out


def extract_uf2(
    input: str, output: str, addr: int | None = None, size: int | None = None
):
    uf2 = extract(input, addr, size)

    if uf2 is None:
        print("Nothing extracted")
        return

    uf2.to_file(output)

    print_uf2(uf2)


def print_uf2(uf2: UF2File):

    print("{} blocks: ".format(uf2.num_blocks))
    for block in uf2:
        h: Header = block.header
        print(
            "{0}: address = 0x{1:04x}, payload_size = {2} (0x{2:04x})".format(
                h.block_number, h.address, h.payload_size
            )
        )


def main():

    # Cmd line format:
    # extract.py -a <addr> -s <size> <input_file> <output_file>

    ap = argparse.ArgumentParser(description="extract data from UF2")
    ap.add_argument("input", help="input file")
    ap.add_argument("output", help="output file")

    int0 = lambda x: int(x, base=0)
    ap.add_argument(
        "--addr", "--address", "-a", type=int0, help="start address of data to extract"
    )
    ap.add_argument("--size", "-s", type=int0, help="size of data to extract")

    args = ap.parse_args()
    extract_uf2(args.input, args.output, args.addr, args.size)


if __name__ == "__main__":

    main()
