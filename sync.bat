@echo off
cd /d %~dp0

git add .
git commit -m "修改内容"
git push

pause
