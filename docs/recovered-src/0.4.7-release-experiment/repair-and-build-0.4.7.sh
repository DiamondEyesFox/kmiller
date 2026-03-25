# ==== From project root ====
cd ~/Downloads/kmiller || exit 1

# 0) Back up current files (just in case)
for f in src/Pane.h src/Pane.cpp src/MainWindow.cpp CMakeLists.txt; do
  [ -f "$f" ] && cp -a "$f" "$f.bak.$(date +%s)"
done

# 1) If you have an earlier backup of Pane.h / Pane.cpp from today, restore the latest clean ones
#    (harmless if none exist)
for f in Pane.h Pane.cpp; do
  LATEST="$(ls -t "src/$f.bak."* 2>/dev/null | head -n1 || true)"
  if [ -n "$LATEST" ]; then
    echo "Restoring $f from $LATEST"
    cp -f "$LATEST" "src/$f"
  fi
done

# 2) Fix the showContextMenu declaration IN-PLACE in Pane.h (delete any corrupted lines, insert one good one)
#    - Remove any line that declares showContextMenu(…)
#    - Insert a correct declaration just before 'private:' (or at end if not found)
awk '
  BEGIN{inserted=0}
  /showContextMenu\s*\(/ {next}                       # drop any existing bad declarations
  /private:/{                                          
    if(!inserted){
      print "    void showContextMenu(const QPoint &globalPos, const QList<QUrl> &urls = QList<QUrl>());"
      inserted=1
    }
    print $0
    next
  }
  {print $0}
  END{
    if(!inserted){
      print ""
      print "    void showContextMenu(const QPoint &globalPos, const QList<QUrl> &urls = QList<QUrl>());"
    }
  }
' src/Pane.h > src/.Pane.h.fixed && mv src/.Pane.h.fixed src/Pane.h

# 3) Ensure Pane.cpp has QDir include
grep -q '<QDir>' src/Pane.cpp || sed -i '1i #include <QDir>' src/Pane.cpp

# 4) Replace the showContextMenu *implementation* in Pane.cpp with the correct QList<QUrl> version
perl -0777 -pe '
  s|void\s+Pane::showContextMenu[^{]*\{.*?\n\}|
void Pane::showContextMenu(const QPoint &globalPos, const QList<QUrl> &urls) {
    QList<QUrl> list = urls;

    // If nothing explicit passed (e.g. Miller right-click on background), use current selection
    if (list.isEmpty()) {
        QAbstractItemView *v = qobject_cast<QAbstractItemView*>(stack->currentWidget());
        if (v && v->selectionModel()) {
            const auto rows = v->selectionModel()->selectedRows();
            for (const QModelIndex &idx : rows) {
                QModelIndex src = proxy->mapToSource(idx);
                KFileItem item = dirModel->itemForIndex(src);
                if (!item.isNull()) list << item.url();
            }
        }
    }
    if (list.isEmpty()) return;

    QMenu menu;
    QAction *actOpen = menu.addAction("Open");
    QAction *actQL   = menu.addAction("Quick Look");

    QAction *chosen = menu.exec(globalPos);
    if (!chosen) return;

    if (chosen == actOpen) {
        for (const QUrl &u : list) {
            auto *job = new KIO::OpenUrlJob(u);
            job->start();
        }
        return;
    }
    if (chosen == actQL) {
        if (!ql) ql = new QuickLookDialog(this);
        for (const QUrl &u : list) {
            if (u.isLocalFile()) ql->showFile(u.toLocalFile());
        }
        return;
    }
}
|s' -i src/Pane.cpp

# 5) Make sure the MillerView -> Pane connection passes a single URL wrapped as a list
#    (this replaces whatever that line is with the clean lambda)
sed -i 's|connect(.*MillerView::contextMenuRequested.*|connect(miller, &MillerView::contextMenuRequested, this, [this](const QUrl &u, const QPoint &g){ showContextMenu(g, {u}); });|' src/Pane.cpp

# 6) Add a compile-time definition for the version string (safe, minimal CMake touch)
grep -q 'KMILLER_VERSION_STR' CMakeLists.txt || cat >> CMakeLists.txt <<'EOF'

# Provide version string to the code as KMILLER_VERSION_STR
if(NOT DEFINED PROJECT_VERSION)
  set(PROJECT_VERSION "${KMILLER_VERSION}")
endif()
target_compile_definitions(kmiller PRIVATE KMILLER_VERSION_STR="${PROJECT_VERSION}")
EOF

# 7) Show the version in the window title (MainWindow constructor)
#    Insert once right after the opening brace of MainWindow::MainWindow(...)
perl -0777 -pe '
  s|(MainWindow::MainWindow\s*\([^)]*\)\s*\{\s*)|\1\n    setWindowTitle(QStringLiteral("KMiller ") + QStringLiteral(KMILLER_VERSION_STR));\n|s
' -i src/MainWindow.cpp

# 8) Rebuild clean and install as 0.4.7
rm -rf build
mkdir build && cd build
cmake -DKMILLER_VERSION=0.4.7 ..
make -j"$(nproc)" || exit 1
sudo make install

# 9) Point launcher/menu at this build and refresh entries
sudo ln -sfn /opt/kmiller/versions/0.4.7/bin/kmiller /usr/local/bin/kmiller
sudo sed -i 's|^Exec=.*|Exec=/opt/kmiller/versions/0.4.7/bin/kmiller %U|' /usr/local/share/applications/kmiller.desktop
kbuildsycoca6 --noincremental 2>/dev/null || true
update-desktop-database /usr/local/share/applications 2>/dev/null || true

# 10) Sanity checks
echo "CLI -> $(readlink -f /usr/local/bin/kmiller)"
echo "Desktop Exec -> $(grep -m1 '^Exec=' /usr/local/share/applications/kmiller.desktop)"
ls -lh /opt/kmiller/versions/0.4.7/bin/kmiller
