# -*- coding: utf-8 -*-
# Test framework: Python unittest (compatible with pytest)
# Purpose: Validate README.md style and critical content introduced/modified in the PR diff.

import re
import unittest
from pathlib import Path


def _project_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _find_readme() -> Path:
    root = _project_root()
    candidates = [
        root / "README.md",
        root / "README.MD",
        root / "README.markdown",
        root / "README",
    ]
    for p in candidates:
        if p.is_file():
            return p
    # Fallback: any README* at repo root
    for p in sorted(root.glob("README*")):
        if p.is_file():
            return p
    msg = "README not found at project root"
    raise FileNotFoundError(msg)


def _find_license_file() -> Path | None:
    root = _project_root()
    for name in ("LICENSE", "LICENSE.md", "LICENSE.txt"):
        p = root / name
        if p.is_file():
            return p
    return None


class TestReadmeStyle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.readme_path = _find_readme()
        cls.text = cls.readme_path.read_text(encoding="utf-8", errors="ignore")
        cls.lines = cls.text.splitlines()

    def test_title_present_and_first_line(self):
        self.assertGreater(len(self.lines), 0, "README is empty")
        first = self.lines[0].strip()
        self.assertRegex(
            first,
            r'^#\s*Distributed File System\s*\(DFS\)\s*with\s*AFS\s*Semantics$',
            msg=f"Unexpected README title line: {first!r}",
        )

    def test_badges_from_shields_and_expected_topics(self):
        badge_re = re.compile(r'!\[[^\]]*\]\((?:https?:)?//img\.shields\.io/[^)]+\)', re.I)
        badges = badge_re.findall(self.text)
        self.assertGreaterEqual(len(badges), 6, f"Expected >=6 shields.io badges, found {len(badges)}")
        for kw in ("C++17", "gRPC", "FUSE", "CMake", "Ubuntu", "MIT", "Status"):
            self.assertRegex(self.text, re.compile(re.escape(kw), re.I), msg=f"Badge/topic {kw!r} not found")

    def test_expected_sections_exist_as_markdown_h2(self):
        # Focus on sections present in the diff; match headings regardless of emojis.
        required = [
            "Key Features",
            "Folder Structure",
            "Dependencies",
            "Build Instructions",
            "Run the DFS",
            "Testing the File System",
            "Versioning Logic",
            "Unmounting the File System",
            "Project Explanation",
            "Future Improvements",
            "Author",
            "License",
        ]
        for section in required:
            pat = re.compile(rf'(?mi)^\s*##\s+.*{re.escape(section)}.*$')
            self.assertRegex(self.text, pat, msg=f"Section heading for {section!r} missing")

    def test_code_fences_are_balanced(self):
        fences = self.text.count("```")
        self.assertGreaterEqual(fences, 2, "Expected at least one fenced code block")
        self.assertEqual(fences % 2, 0, "Unbalanced triple backtick fences detected")

    def test_folder_structure_tree_mentions_expected_entries(self):
        for kw in ("dfs_project/", "proto/", "client/", "server/", "build/", "CMakeLists.txt"):
            self.assertIn(kw, self.text, msg=f"Folder structure missing {kw!r}")

    def test_dependencies_section_lists_required_packages(self):
        pkgs = [
            "build-essential",
            "cmake",
            "pkg-config",
            "git",
            "libfuse3-dev",
            "protobuf-compiler",
            "grpc-tools",
            "libgrpc++-dev",
        ]
        for pkg in pkgs:
            self.assertIn(pkg, self.text, msg=f"Dependency {pkg!r} not mentioned in README")

    def test_build_instructions_include_protoc_and_cmake(self):
        self.assertRegex(self.text, r'protoc\s+-I=\.', re.I | re.S)
        self.assertRegex(self.text, r'grpc_cpp_plugin', re.I)
        self.assertIn("cmake -S . -B build", self.text)
        self.assertIn("cmake --build build", self.text)

    def test_run_server_command_present(self):
        self.assertRegex(self.text, r'(?m)^\s*\./build/server\s*$', msg="Server run command missing")

    def test_mount_commands_present(self):
        self.assertIn("mkdir -p /tmp/dfs_mount", self.text)
        self.assertRegex(self.text, r'(?m)^\s*\./build/fuse_client\s+/tmp/dfs_mount\s+-f\s*$', msg="FUSE mount command missing")

    def test_filesystem_operations_examples_present(self):
        self.assertIn('echo "hello world" > /tmp/dfs_mount/test.txt', self.text)
        self.assertIn('cat /tmp/dfs_mount/test.txt', self.text)
        self.assertIn('rm /tmp/dfs_mount/test.txt', self.text)
        self.assertIn('ls /tmp/dfs_mount', self.text)

    def test_versioning_logic_lww_explained(self):
        self.assertIn("Last Writer Wins", self.text)
        self.assertIn("Write from outdated client rejected", self.text)

    def test_unmount_commands_present(self):
        self.assertIn("fusermount3 -u /tmp/dfs_mount", self.text)
        self.assertIn("sudo umount /tmp/dfs_mount", self.text)

    def test_author_and_links_present(self):
        # Accept any LinkedIn/GitHub links in markdown format
        linkedin_re = re.compile(r'\[LinkedIn\]\(https?://(?:www\.)?linkedin\.com/in/[^)]+\)', re.I)
        github_re = re.compile(r'\[GitHub\]\(https?://github\.com/[^)]+\)', re.I)
        self.assertRegex(self.text, linkedin_re, "LinkedIn profile link missing or malformed")
        self.assertRegex(self.text, github_re, "GitHub profile link missing or malformed")
        self.assertRegex(self.text, re.compile(r'Sai\s+Keerthan\s+Palavarapu', re.I), "Author name missing")

    def test_license_link_and_file_exists(self):
        # Ensure README links to ./LICENSE and the file exists and is non-empty
        link_re = re.compile(r'\[[^\]]*License[^\]]*\]\(\./LICENSE\)', re.I)
        self.assertRegex(self.text, link_re, "Expected markdown link to ./LICENSE missing")
        lic = _find_license_file()
        self.assertIsNotNone(lic, "LICENSE file not found in repository root")
        content = lic.read_text(encoding="utf-8", errors="ignore")
        self.assertGreater(len(content.strip()), 0, "LICENSE file is empty")

    def test_relative_links_point_to_existing_files(self):
        root = _project_root()
        rel_links = re.findall(r'\[[^\]]+\]\((\./[^)#]+)\)', self.text)
        for rel in rel_links:
            target = (root / rel[2:]).resolve()
            self.assertTrue(target.exists(), f"Relative link target missing: {rel} -> {target}")

    def test_bash_language_hints_present_for_shell_blocks(self):
        # Ensure at least one fenced bash code block exists (install/run instructions)
        self.assertRegex(self.text, r'```bash', "Expected fenced bash code block(s)")

    def test_code_block_parsing_detects_folder_tree(self):
        # Parse fenced code blocks and ensure one contains the folder tree marker
        code_block_re = re.compile(r'```([A-Za-z0-9_\-]+)?\s*\n(.*?)\n```', re.DOTALL)
        blocks = code_block_re.findall(self.text)
        self.assertGreaterEqual(len(blocks), 3, "Expected multiple fenced code blocks")
        has_tree = any("dfs_project/" in body for (_lang, body) in blocks)
        self.assertTrue(has_tree, "Folder structure code block not found")


if __name__ == "__main__":
    unittest.main()