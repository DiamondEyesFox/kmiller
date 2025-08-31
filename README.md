# KMiller

**Finder-style file manager for KDE Plasma with Miller Columns and Quick Look preview.**

---

## 🚧 Status: Alpha
This is an early **proof-of-concept** release.  
KMiller currently demonstrates the **Miller Columns view** and **Quick Look preview**, but many features are incomplete or buggy.  

- ✅ Builds and runs on Arch Linux with KDE Plasma  
- ✅ Basic file browsing in Miller Columns  
- ✅ Quick Look preview for files  
- ⚠️ Many basic features are unimplemented or unstable  
- ⚠️ Not intended for daily use yet  

---

## 🔖 Versioning
- `v0.4.2-alpha`: First working prototype (current release)  
- Earlier versions (`v0.4.0`, etc.) may be archived later  

---

## 🔧 Building
```bash
git clone https://github.com/DiamondEyesFox/kmiller.git
cd kmiller
mkdir build && cd build
cmake ..
make -j$(nproc)
./kmiller
