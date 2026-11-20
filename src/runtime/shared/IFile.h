/*
    LGCK Builder Runtime
    Copyright (C) 1999, 2016  Francois Blanchette

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// MAINTAINER DIRECTIVE (2026-11-18): this is the engine's file-interface
// contract and it is FROZEN — do not remove, rename, or re-sign members,
// including the "production-dead" virtuals flagged in
// docs/audits/IFile.h.audit.md. Two backends plus the external LGCK
// builder/editor (bin/utils/sync.sh) compile against this exact surface.

#define IFILE_OK 1
#define IFILE_NOT_OK 0

#include <string>
#include <string_view>

// IFile contract (pinned 2026-11-17; both CFileWrap and CFileMem must obey):
// - Wire format for the string operators: little-endian length prefix,
//   1 byte when length < 0xff, else 0xff + uint16, and when that uint16 is
//   0xffff a following uint32 carries the real length. int is a raw 4-byte
//   host-order passthrough. Both backends must encode/decode identically.
// - read()/write() return an IFILE_OK/IFILE_NOT_OK *flag*, never a byte
//   count; callers must compare against IFILE_OK only.
// - read(buf, 0) and write(buf, 0) are success no-ops returning IFILE_OK.
// - seek(): negative offsets fail on both backends; seeking past EOF succeeds
//   (subsequent reads then fail). Verified 2026-11-18 in FileWrap/FileMem.
// - open() releases any previously held handle before reopening (both).
//   Mode vocabulary is fopen-style; a legal mode contains at least one of
//   'r', 'w', 'a' ("rb", "wb", "rw+", "ab" are in use). Divergence callers
//   must know: 'w' truncates a disk file (fopen) but does NOT clear CFileMem's
//   existing buffer — stale tail bytes remain visible to getSize(); generic
//   code must not rely on truncation.
// - Mode gating: CFileMem rejects read() without 'r' and write() without
//   'w'/'a' as IFILE_NOT_OK. CFileWrap does not inspect the mode and defers
//   to stdio (a failed open leaves a null handle; transfers then return
//   IFILE_NOT_OK). The string operators can bypass a backend's own gate —
//   do not mix modes and string operators portably.
// - close() flushes and releases. CFileWrap returns false when nothing was
//   open; CFileMem always returns true and DESTROYS its buffer contents:
//   inspect getSize()/buffer() before close(), never after (its dtor closes).
class IFile
{
public:
    virtual ~IFile() {};

    virtual bool operator>>(std::string &str) = 0;
    virtual bool operator<<(const std::string_view &str) = 0;
    virtual bool operator<<(const char *s) = 0;
    virtual bool operator+=(const std::string_view &str) = 0;

    virtual bool operator>>(int &n) = 0;
    virtual bool operator<<(const int n) = 0;

    virtual bool operator>>(bool &b) = 0;
    virtual bool operator<<(const bool b) = 0;
    virtual bool operator+=(const char *) = 0;

    virtual bool open(const std::string_view &filename, const std::string_view &mode = "rb") = 0;
    virtual int read(void *buf, const int size) = 0;
    virtual int write(const void *buf, const int size) = 0;

    virtual bool close() = 0;
    virtual long getSize() = 0;
    virtual bool seek(const long i) = 0;
    virtual long tell() = 0;
    virtual bool flush() = 0;
    virtual const std::string_view mode() = 0;
};
