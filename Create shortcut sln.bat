cd build
if not exist "..\UwU_Engine.sln.lnk" (
    powershell -Command "$ws = New-Object -ComObject WScript.Shell; $s = $ws.CreateShortcut('..\UwU_Engine.sln.lnk'); $s.TargetPath = '%CD%\UwU_Engine.sln'; $s.Save()"
    echo Created shortcut: UwU_Engine.sln.lnk
)