"use strict";

// Wrap a virtual-address range from a Wii DOL in a one-section Microsoft COFF
// object. The Xbox 360 SDK dumpbin understands IMAGE_FILE_MACHINE_POWERPCBE,
// making this a small, local disassembly bridge for Wii PowerPC code.

const fs = require("fs");

if (process.argv.length !== 6) {
  console.error(
    "usage: node wrap_ppcbe_coff.js <main.dol> <start-va> <end-va> <out.obj>",
  );
  process.exit(2);
}

const [, , dolPath, startText, endText, outputPath] = process.argv;
const start = Number(BigInt(startText));
const end = Number(BigInt(endText));
if (!Number.isSafeInteger(start) || !Number.isSafeInteger(end) || end <= start) {
  throw new Error("invalid virtual-address range");
}

const dol = fs.readFileSync(dolPath);
const sections = [];
for (let i = 0; i < 7; ++i) {
  sections.push({
    offset: dol.readUInt32BE(i * 4),
    address: dol.readUInt32BE(0x48 + i * 4),
    size: dol.readUInt32BE(0x90 + i * 4),
  });
}
for (let i = 0; i < 11; ++i) {
  sections.push({
    offset: dol.readUInt32BE(0x1c + i * 4),
    address: dol.readUInt32BE(0x64 + i * 4),
    size: dol.readUInt32BE(0xac + i * 4),
  });
}

const section = sections.find(
  (candidate) =>
    candidate.size > 0 &&
    start >= candidate.address &&
    end <= candidate.address + candidate.size,
);
if (!section) throw new Error("range does not fit within one DOL section");

const sourceOffset = section.offset + start - section.address;
const code = dol.subarray(sourceOffset, sourceOffset + end - start);
if ((code.length & 3) !== 0) throw new Error("PowerPC range must be 4-byte aligned");

const fileHeaderSize = 20;
const sectionHeaderSize = 40;
const rawOffset = fileHeaderSize + sectionHeaderSize;
const coff = Buffer.alloc(rawOffset + code.length);

// IMAGE_FILE_HEADER (COFF metadata is little-endian; section contents remain
// PowerPC big-endian machine code).
coff.writeUInt16LE(0x01f2, 0); // IMAGE_FILE_MACHINE_POWERPCBE
coff.writeUInt16LE(1, 2);
coff.writeUInt32LE(0, 4);
coff.writeUInt32LE(0, 8);
coff.writeUInt32LE(0, 12);
coff.writeUInt16LE(0, 16);
coff.writeUInt16LE(0, 18);

coff.write(".text", fileHeaderSize, "ascii");
coff.writeUInt32LE(0, fileHeaderSize + 8);
coff.writeUInt32LE(start >>> 0, fileHeaderSize + 12);
coff.writeUInt32LE(code.length, fileHeaderSize + 16);
coff.writeUInt32LE(rawOffset, fileHeaderSize + 20);
coff.writeUInt32LE(0, fileHeaderSize + 24);
coff.writeUInt32LE(0, fileHeaderSize + 28);
coff.writeUInt16LE(0, fileHeaderSize + 32);
coff.writeUInt16LE(0, fileHeaderSize + 34);
coff.writeUInt32LE(0x60000020, fileHeaderSize + 36);
code.copy(coff, rawOffset);

fs.writeFileSync(outputPath, coff);
console.log(
  `wrapped ${code.length} bytes VA=0x${start.toString(16)}..0x${end.toString(16)} ` +
    `DOL=0x${sourceOffset.toString(16)} -> ${outputPath}`,
);
