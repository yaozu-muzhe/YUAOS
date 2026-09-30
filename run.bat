@echo off
cd /d "%~dp0"
if not exist build\yuaos.img (
    echo [run.bat] build\yuaos.img not found, run build.bat first.
    pause
    exit /b 1
)
if not exist build\pcfs.img (
    echo [run.bat] build\pcfs.img not found, run build.bat first.
    pause
    exit /b 1
)
qemu-system-i386 -fda build\yuaos.img -hda build\pcfs.img -boot a
