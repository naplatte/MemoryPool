# Use an ASCII timezone name while retaining China Standard Time (UTC+8).
# Windows CTest otherwise writes the localized timezone name in the ANSI code page.
set(ENV{TZ} "CST-8")
