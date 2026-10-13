"""Synthetic regression tests for safe-gex-inbox.py; no Drive or repository is used."""
from __future__ import annotations

import importlib.util
import io
import os
import shutil
import tarfile
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
SPEC = importlib.util.spec_from_file_location("safe_gex_inbox", os.path.join(HERE, "safe-gex-inbox.py"))
INBOX = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(INBOX)


def make_archive(path: str, rows: list[tuple[str, bytes | None, str]]) -> None:
    with tarfile.open(path, "w:gz") as tar:
        for name, data, kind in rows:
            info = tarfile.TarInfo(name)
            if kind == "dir":
                info.type = tarfile.DIRTYPE
                tar.addfile(info)
            elif kind == "symlink":
                info.type = tarfile.SYMTYPE
                info.linkname = "outside"
                tar.addfile(info)
            else:
                info.size = len(data or b"")
                tar.addfile(info, io.BytesIO(data or b""))


class SafeGexInboxTests(unittest.TestCase):
    def setUp(self):
        self.root = tempfile.mkdtemp(prefix="safe-gex-inbox-")
        self.dest = os.path.join(self.root, "repo")
        self.archive = os.path.join(self.root, "drop.tar.gz")

    def tearDown(self):
        shutil.rmtree(self.root, ignore_errors=True)

    def test_regular_files_extract_under_destination(self):
        make_archive(self.archive, [("new", None, "dir"), ("new/config.txt", b"safe\n", "file")])
        self.assertEqual(INBOX.extract(self.archive, self.dest), 0)
        with open(os.path.join(self.dest, "new", "config.txt"), "rb") as f:
            self.assertEqual(f.read(), b"safe\n")

    def test_traversal_member_is_blocked_without_outside_write(self):
        outside = os.path.join(self.root, "outside.txt")
        make_archive(self.archive, [("../outside.txt", b"blocked", "file")])
        self.assertEqual(INBOX.main([self.archive, self.dest]), 1)
        self.assertFalse(os.path.exists(outside))

    def test_existing_nested_symlink_cannot_redirect_extraction(self):
        outside = os.path.join(self.root, "outside")
        os.makedirs(self.dest)
        os.makedirs(outside)
        try:
            os.symlink(outside, os.path.join(self.dest, "nested"))
        except (NotImplementedError, OSError):
            self.skipTest("symlink creation is unavailable on this host")
        make_archive(self.archive, [("nested/config.txt", b"blocked", "file")])
        self.assertEqual(INBOX.main([self.archive, self.dest]), 1)
        self.assertFalse(os.path.exists(os.path.join(outside, "config.txt")))

    def test_links_and_existing_files_are_blocked_without_replacement(self):
        make_archive(self.archive, [("link", None, "symlink")])
        self.assertEqual(INBOX.main([self.archive, self.dest]), 1)
        os.makedirs(self.dest)
        existing = os.path.join(self.dest, "kept.txt")
        with open(existing, "wb") as f:
            f.write(b"keep")
        make_archive(self.archive, [("kept.txt", b"replace", "file")])
        self.assertEqual(INBOX.main([self.archive, self.dest]), 1)
        with open(existing, "rb") as f:
            self.assertEqual(f.read(), b"keep")


if __name__ == "__main__":
    unittest.main(verbosity=2)
