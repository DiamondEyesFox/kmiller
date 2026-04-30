#!/usr/bin/env python3
"""Floating manual QA coach for KMiller release checks.

This helper intentionally sits outside KMiller. It watches a disposable fixture
folder for outcomes that are safe to infer, while leaving visual/subjective
checks to explicit Pass/Fail buttons.
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Callable

from PyQt6.QtCore import QTimer, Qt
from PyQt6.QtGui import QFont
from PyQt6.QtWidgets import (
    QApplication,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QProgressBar,
    QPlainTextEdit,
    QVBoxLayout,
    QWidget,
)


DEFAULT_FIXTURE = Path.home() / "Downloads" / "Kmiller Testing Folder"


@dataclass
class Step:
    title: str
    instruction: str
    expected: str
    detector: Callable[[Path], bool] | None = None
    status: str = "pending"
    note: str = ""
    completed_at: str = ""


def kmiller_is_active(_: Path) -> bool:
    return "kmiller" in active_window().lower()


def p(root: Path, rel: str) -> Path:
    return root / rel


def missing(root: Path, rel: str) -> bool:
    return not p(root, rel).exists()


def exists(root: Path, rel: str) -> bool:
    return p(root, rel).exists()


def duplicate_file_seen(root: Path) -> bool:
    folder = p(root, "01 File Operations")
    if not folder.exists():
        return False
    matches = [
        item
        for item in folder.iterdir()
        if item.is_file()
        and item.name.startswith("duplicate me")
        and item.name != "duplicate me.txt"
    ]
    return bool(matches)


def duplicate_folder_seen(root: Path) -> bool:
    folder = p(root, "01 File Operations")
    if not folder.exists():
        return False
    matches = [
        item
        for item in folder.iterdir()
        if item.is_dir()
        and item.name.startswith("Duplicate Target")
        and item.name != "Duplicate Target"
    ]
    return bool(matches)


def new_folder_seen(root: Path) -> bool:
    folder = p(root, "01 File Operations")
    if not folder.exists():
        return False
    return any(item.is_dir() and "new" in item.name.lower() for item in folder.iterdir())


def archive_output_seen(root: Path) -> bool:
    likely_archives = list(root.glob("*.zip")) + list(root.glob("*.tar*")) + list(root.glob("*.7z"))
    conflict_child = p(root, "05 Conflict Extraction Target/Nested Folder/nested archive child.txt")
    return bool(likely_archives) or conflict_child.exists()


def active_window() -> str:
    env = os.environ.copy()
    uid = os.getuid()
    env.setdefault("XDG_RUNTIME_DIR", f"/run/user/{uid}")
    hypr_root = Path(env["XDG_RUNTIME_DIR"]) / "hypr"
    if "HYPRLAND_INSTANCE_SIGNATURE" not in env and hypr_root.exists():
        try:
            first = next(path.name for path in hypr_root.iterdir() if path.is_dir())
            env["HYPRLAND_INSTANCE_SIGNATURE"] = first
        except StopIteration:
            pass
    try:
        result = subprocess.run(
            ["hyprctl", "-j", "activewindow"],
            check=False,
            capture_output=True,
            text=True,
            timeout=0.8,
            env=env,
        )
        if result.returncode != 0 or not result.stdout.strip():
            return "active window unavailable"
        data = json.loads(result.stdout)
        title = data.get("title") or "(untitled)"
        klass = data.get("class") or data.get("initialClass") or "(unknown app)"
        return f"{klass}: {title}"
    except Exception as exc:
        return f"active window unavailable: {exc.__class__.__name__}"


def build_steps() -> list[Step]:
    return [
        Step(
            "Focus KMiller",
            "Open KMiller on the testing folder. Keep this harness visible beside it.",
            "Active window should show KMiller while you perform checks.",
            kmiller_is_active,
        ),
        Step(
            "Move File",
            "Cut `move me to Move Target.txt` and paste it into `Move Target`.",
            "Source disappears and destination file appears.",
            lambda root: missing(root, "01 File Operations/move me to Move Target.txt")
            and exists(root, "01 File Operations/Move Target/move me to Move Target.txt"),
        ),
        Step(
            "Rename File",
            "Rename `rename me.txt` to `renamed from kmiller.txt`.",
            "Old filename disappears and new filename appears.",
            lambda root: missing(root, "01 File Operations/rename me.txt")
            and exists(root, "01 File Operations/renamed from kmiller.txt"),
        ),
        Step(
            "Duplicate File",
            "Use KMiller Duplicate on `duplicate me.txt`.",
            "A second file whose name starts with `duplicate me` appears.",
            duplicate_file_seen,
        ),
        Step(
            "Duplicate Folder",
            "Use KMiller Duplicate on the `Duplicate Target` folder.",
            "A second folder whose name starts with `Duplicate Target` appears.",
            duplicate_folder_seen,
        ),
        Step(
            "Create New Folder",
            "Use empty-space context menu to create a folder with `new` in its name.",
            "A new folder appears in `01 File Operations`.",
            new_folder_seen,
        ),
        Step(
            "Trash File",
            "Move `99 Trash And Delete Me/trash me.txt` to Trash.",
            "The file disappears from the fixture folder.",
            lambda root: missing(root, "99 Trash And Delete Me/trash me.txt"),
        ),
        Step(
            "Permanent Delete",
            "Permanently delete `99 Trash And Delete Me/permanently delete me.txt` and confirm the warning.",
            "The warning appears, then the file disappears after confirmation.",
            lambda root: missing(root, "99 Trash And Delete Me/permanently delete me.txt"),
        ),
        Step(
            "Open With Text",
            "Open With on `.txt`, `.md`, `.csv`, `.json`, and `.html` samples.",
            "Handlers appear, double-click launches one, custom command remains available.",
            None,
        ),
        Step(
            "Open With Media",
            "Open With on `.svg`, `.pdf`, and `.kmtest` samples.",
            "Image/PDF handlers appear; unknown extension still offers custom command.",
            None,
        ),
        Step(
            "Search",
            "Search for `needle-kmiller-alpha`, `needle-kmiller-beta`, and `needle-kmiller-nested`.",
            "Results appear, then clearing search returns to normal folder browsing.",
            None,
        ),
        Step(
            "Symlink Setting",
            "Toggle Follow symbolic links and open `06 Symlink Target/link to real folder`.",
            "Off/on behavior matches the setting and does not get stuck.",
            None,
        ),
        Step(
            "Archive Flow",
            "Compress `04 Archive Source`, then extract into `05 Conflict Extraction Target`.",
            "Conflict handling is clear and nested extracted contents are visible.",
            archive_output_seen,
        ),
        Step(
            "View Consistency",
            "Repeat open, rename, Quick Look, right-click, and duplicate in Miller, Icons, Details, and Compact views.",
            "The same action feels consistent across all four views.",
            None,
        ),
    ]


class Harness(QMainWindow):
    def __init__(self, root: Path):
        super().__init__()
        self.root = root
        self.steps = build_steps()
        self.current_index = 0
        self.setWindowTitle("KMiller Manual QA Coach")
        self.setMinimumSize(620, 560)
        self.setWindowFlag(Qt.WindowType.WindowStaysOnTopHint, True)
        self._build_ui()
        self._refresh_all()
        self.timer = QTimer(self)
        self.timer.timeout.connect(self._tick)
        self.timer.start(800)

    def _build_ui(self) -> None:
        central = QWidget()
        layout = QVBoxLayout(central)

        title = QLabel("KMiller Manual QA Coach")
        title_font = QFont()
        title_font.setPointSize(18)
        title_font.setBold(True)
        title.setFont(title_font)
        layout.addWidget(title)

        self.fixture_label = QLabel(f"Fixture: {self.root}")
        self.fixture_label.setWordWrap(True)
        layout.addWidget(self.fixture_label)

        self.active_label = QLabel("Active window: checking...")
        self.active_label.setWordWrap(True)
        layout.addWidget(self.active_label)

        self.progress = QProgressBar()
        layout.addWidget(self.progress)

        self.step_title = QLabel()
        step_font = QFont()
        step_font.setPointSize(15)
        step_font.setBold(True)
        self.step_title.setFont(step_font)
        self.step_title.setWordWrap(True)
        layout.addWidget(self.step_title)

        self.instruction = QLabel()
        self.instruction.setWordWrap(True)
        layout.addWidget(self.instruction)

        self.expected = QLabel()
        self.expected.setWordWrap(True)
        layout.addWidget(self.expected)

        self.detector_label = QLabel()
        self.detector_label.setWordWrap(True)
        layout.addWidget(self.detector_label)

        buttons = QHBoxLayout()
        self.prev_button = QPushButton("Previous")
        self.prev_button.clicked.connect(self._previous)
        buttons.addWidget(self.prev_button)

        self.pass_button = QPushButton("Pass")
        self.pass_button.clicked.connect(lambda: self._set_current("passed", "manual pass"))
        buttons.addWidget(self.pass_button)

        self.fail_button = QPushButton("Fail")
        self.fail_button.clicked.connect(lambda: self._set_current("failed", "manual fail"))
        buttons.addWidget(self.fail_button)

        self.skip_button = QPushButton("Skip")
        self.skip_button.clicked.connect(lambda: self._set_current("skipped", "manual skip"))
        buttons.addWidget(self.skip_button)

        self.next_button = QPushButton("Next")
        self.next_button.clicked.connect(self._next)
        buttons.addWidget(self.next_button)
        layout.addLayout(buttons)

        self.list_widget = QListWidget()
        self.list_widget.currentRowChanged.connect(self._select_row)
        layout.addWidget(self.list_widget)

        bottom = QHBoxLayout()
        report_button = QPushButton("Write Report")
        report_button.clicked.connect(self._write_report)
        bottom.addWidget(report_button)

        reset_button = QPushButton("Reset Known Fixtures")
        reset_button.clicked.connect(self._reset_known_fixtures)
        bottom.addWidget(reset_button)
        layout.addLayout(bottom)

        self.log = QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setMaximumBlockCount(200)
        layout.addWidget(self.log)

        self.setCentralWidget(central)

    def _tick(self) -> None:
        active = active_window()
        self.active_label.setText(f"Active window: {active}")
        for idx, step in enumerate(self.steps):
            if step.status != "pending" or step.detector is None:
                continue
            try:
                if step.detector(self.root):
                    self._set_step(idx, "passed", "auto-detected expected fixture state")
                    if idx == self.current_index:
                        self._next_pending_or_stay()
            except Exception as exc:
                self._append_log(f"Detector error for {step.title}: {exc}")
        self._refresh_all()

    def _set_step(self, idx: int, status: str, note: str) -> None:
        step = self.steps[idx]
        if step.status == status and step.note == note:
            return
        step.status = status
        step.note = note
        step.completed_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self._append_log(f"{step.title}: {status} ({note})")

    def _set_current(self, status: str, note: str) -> None:
        self._set_step(self.current_index, status, note)
        self._next_pending_or_stay()
        self._refresh_all()

    def _next_pending_or_stay(self) -> None:
        for idx in range(self.current_index + 1, len(self.steps)):
            if self.steps[idx].status == "pending":
                self.current_index = idx
                return

    def _previous(self) -> None:
        self.current_index = max(0, self.current_index - 1)
        self._refresh_all()

    def _next(self) -> None:
        self.current_index = min(len(self.steps) - 1, self.current_index + 1)
        self._refresh_all()

    def _select_row(self, row: int) -> None:
        if 0 <= row < len(self.steps):
            self.current_index = row
            self._refresh_current()

    def _refresh_all(self) -> None:
        passed = sum(1 for step in self.steps if step.status == "passed")
        done = sum(1 for step in self.steps if step.status != "pending")
        self.progress.setMaximum(len(self.steps))
        self.progress.setValue(done)
        self.progress.setFormat(f"{done}/{len(self.steps)} done, {passed} passed")

        self.list_widget.blockSignals(True)
        self.list_widget.clear()
        for step in self.steps:
            marker = {
                "pending": "[ ]",
                "passed": "[x]",
                "failed": "[!]",
                "skipped": "[-]",
            }.get(step.status, "[?]")
            item = QListWidgetItem(f"{marker} {step.title}")
            self.list_widget.addItem(item)
        self.list_widget.setCurrentRow(self.current_index)
        self.list_widget.blockSignals(False)
        self._refresh_current()

    def _refresh_current(self) -> None:
        step = self.steps[self.current_index]
        self.step_title.setText(f"{self.current_index + 1}. {step.title} - {step.status}")
        self.instruction.setText(f"Do: {step.instruction}")
        self.expected.setText(f"Expected: {step.expected}")
        if step.detector is None:
            self.detector_label.setText("Detection: manual check only")
        else:
            try:
                state = "ready/pass condition met" if step.detector(self.root) else "waiting for expected state"
            except Exception as exc:
                state = f"detector error: {exc.__class__.__name__}"
            self.detector_label.setText(f"Detection: {state}")

    def _append_log(self, message: str) -> None:
        self.log.appendPlainText(f"{datetime.now().strftime('%H:%M:%S')}  {message}")

    def _write_report(self) -> None:
        lines = [
            "# KMiller Manual QA Harness Report",
            "",
            f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}",
            f"Fixture: {self.root}",
            "",
        ]
        for step in self.steps:
            lines.extend(
                [
                    f"## {step.title}",
                    f"Status: {step.status}",
                    f"Completed: {step.completed_at or 'not completed'}",
                    f"Note: {step.note or '-'}",
                    "",
                ]
            )
        report = self.root / f"KMiller QA Report {datetime.now().strftime('%Y%m%d-%H%M%S')}.md"
        report.write_text("\n".join(lines), encoding="utf-8")
        QMessageBox.information(self, "Report written", str(report))

    def _reset_known_fixtures(self) -> None:
        answer = QMessageBox.question(
            self,
            "Reset fixtures?",
            "This recreates only the known disposable fixture files that may have been moved, renamed, or deleted.",
        )
        if answer != QMessageBox.StandardButton.Yes:
            return
        fixtures = {
            "01 File Operations/move me to Move Target.txt": "Move this file into Move Target.\n",
            "01 File Operations/rename me.txt": "Rename this file to renamed from kmiller.txt.\n",
            "01 File Operations/duplicate me.txt": "Duplicate this file.\n",
            "99 Trash And Delete Me/trash me.txt": "Move this file to Trash.\n",
            "99 Trash And Delete Me/permanently delete me.txt": "Permanently delete this file.\n",
        }
        for rel, text in fixtures.items():
            target = p(self.root, rel)
            target.parent.mkdir(parents=True, exist_ok=True)
            if not target.exists():
                target.write_text(text, encoding="utf-8")
        self._append_log("Known disposable fixtures reset")


def main() -> int:
    root = Path(sys.argv[1]).expanduser() if len(sys.argv) > 1 else DEFAULT_FIXTURE
    if len(sys.argv) > 1 and sys.argv[1] == "--self-test":
        test_root = Path(sys.argv[2]).expanduser() if len(sys.argv) > 2 else DEFAULT_FIXTURE
        if not test_root.exists():
            print(f"Fixture folder does not exist: {test_root}", file=sys.stderr)
            return 2
        steps = build_steps()
        detector_count = sum(1 for step in steps if step.detector is not None)
        for step in steps:
            if step.detector is not None:
                bool(step.detector(test_root))
        print(f"ok: {len(steps)} steps, {detector_count} detectors, fixture={test_root}")
        return 0

    if not root.exists():
        print(f"Fixture folder does not exist: {root}", file=sys.stderr)
        return 2
    app = QApplication(sys.argv)
    harness = Harness(root)
    harness.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
