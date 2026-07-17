@echo off
setlocal EnableDelayedExpansion

set LOW=1
set HIGH=1

echo Finding upper limit...

:increase
echo.
echo Testing !HIGH!

python scripts\generate_benchmark.py !HIGH!

if not !ERRORLEVEL! EQU 0 (
    echo Generator failed
    goto done
)

echo Building...

.\bin\leyo build tests\benchmark.leyo
set RESULT=!ERRORLEVEL!

echo Result: !RESULT!

if !RESULT! EQU 0 (
    set LOW=!HIGH!
    set /a HIGH=HIGH*2
    goto increase
)

echo.
echo Failed at !HIGH!
echo Searching between !LOW! and !HIGH!

:binary
if !LOW! GEQ !HIGH! goto done

set /a MID=(LOW+HIGH)/2

echo.
echo Testing !MID!

python scripts\generate_benchmark.py !MID!

if not !ERRORLEVEL! EQU 0 (
    echo Generator failed
    goto done
)

.\bin\leyo build tests\benchmark.leyo
set RESULT=!ERRORLEVEL!

echo Result: !RESULT!

if !RESULT! EQU 0 (
    set LOW=!MID!
) else (
    set /a HIGH=MID-1
)

goto binary


:done

echo.
echo ==========================
echo Maximum working COUNT: !LOW!
echo ==========================

endlocal