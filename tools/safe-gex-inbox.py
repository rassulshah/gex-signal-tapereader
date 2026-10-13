"""Validate and safely extract a GEX inbox .tar.gz archive.

The sync task treats the Google Drive inbox as untrusted input. This utility
accepts regular files and directories only, rejects traversal/Windows-special
paths and links, and refuses to replace any existing repository file.

Usage:  python safe-gex-inbox.py ARCHIVE.tar.gz DESTINATION
"""
from __future__ import annotations

import os
import re
import shutil
import sys
import tarfile
import tempfile

MAX_MEMBERS = 20_000
MAX_BYTES = 2 * 1024 ** 3


def safe_relative_path(value: str) -> str | None:
    """Return a portable extraction-relative path, rejecting traversal forms."""
    if not isinstance(value, str) or not value or "\x00" in value:
        return None
    if "\\" in value or value.startswith("/") or re.match(r"^[A-Za-z]:", value):
        return None
    parts = value.rstrip("/").split("/")
    if not parts or any(not p or p in (".", "..") or ":" in p for p in parts):
        return None
    return "/".join(parts)


def safe_join(root: str, rel: str) -> str:
    rel = safe_relative_path(rel)
    if rel is None:
        raise ValueError("unsafe archive path")
    base = os.path.realpath(os.path.abspath(root))
    out = os.path.realpath(os.path.join(base, *rel.split("/")))
    try:
        if os.path.commonpath((base, out)) != base:
            raise ValueError("archive path escapes destination")
    except ValueError as e:
        raise ValueError("archive path escapes destination") from e
    return out


def checked_members(archive: str) -> list[tarfile.TarInfo]:
    """Read and validate all members before any destination directory is made."""
    with tarfile.open(archive, "r:gz") as tar:
        members = tar.getmembers()
    if len(members) > MAX_MEMBERS:
        raise ValueError(f"archive has too many members ({len(members)} > {MAX_MEMBERS})")
    total, seen = 0, set()
    for member in members:
        rel = safe_relative_path(member.name)
        if rel is None:
            raise ValueError("archive contains an unsafe member path")
        if not (member.isfile() or member.isdir()):
            raise ValueError("archive contains a link or special file")
        key = rel.casefold()
        if key in seen:
            raise ValueError("archive contains duplicate member paths")
        seen.add(key)
        if member.isfile():
            if member.size < 0:
                raise ValueError("archive contains a member with an invalid size")
            total += member.size
            if total > MAX_BYTES:
                raise ValueError(f"archive exceeds {MAX_BYTES} uncompressed bytes")
    return members


def extract(archive: str, destination: str) -> int:
    """Safely materialize an inbox archive without overwriting destination files."""
    members = checked_members(archive)
    root = os.path.realpath(os.path.abspath(destination))
    os.makedirs(root, exist_ok=True)
    for member in members:
        out = safe_join(root, member.name)
        if member.isdir():
            if os.path.lexists(out) and (os.path.islink(out) or not os.path.isdir(out)):
                raise ValueError("archive directory conflicts with an existing path")
        elif os.path.lexists(out):
            raise ValueError("archive would overwrite an existing repository file")
    stage = tempfile.mkdtemp(prefix=".gex-inbox-stage-", dir=root)
    try:
        with tarfile.open(archive, "r:gz") as tar:
            for member in members:
                out = safe_join(stage, member.name)
                if member.isdir():
                    os.makedirs(out, exist_ok=True)
                    continue
                os.makedirs(os.path.dirname(out), exist_ok=True)
                source = tar.extractfile(member)
                if source is None:
                    raise ValueError("archive member could not be read")
                with source, open(out, "xb") as f:
                    shutil.copyfileobj(source, f, 1024 * 1024)
        for member in members:
            if member.isdir():
                os.makedirs(safe_join(root, member.name), exist_ok=True)
        for member in members:
            if member.isdir():
                continue
            src, out = safe_join(stage, member.name), safe_join(root, member.name)
            os.makedirs(os.path.dirname(out), exist_ok=True)
            os.link(src, out)  # atomic no-replace operation; never clobber a concurrent user write
    finally:
        shutil.rmtree(stage, ignore_errors=True)
    print(f"OK: extracted {sum(1 for m in members if m.isfile())} file(s) safely")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(__doc__.strip())
        return 2
    try:
        return extract(argv[0], argv[1])
    except (OSError, tarfile.TarError, ValueError) as e:
        print(f"BLOCKED: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
