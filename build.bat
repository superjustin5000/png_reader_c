@echo off

set MAIN_FILE_NAME=png

set commonCompilerFlags= -MTd -nologo -Gm- -GR- -EHa -Od -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -DINTERNAL -DDEBUG -DMSVC -Zi

cl %commonCompilerFlags% -Fm%MAIN_FILE_NAME%.map %MAIN_FILE_NAME%.cpp /link -incremental:no -opt:ref /PDB:%MAIN_FILE_NAME%_%random%.pdb /out:%MAIN_FILE_NAME%.exe