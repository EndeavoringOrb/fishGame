:: @echo off
if "%1"=="" (
    echo Please provide a source file as an argument.
    exit /b 1
)

set "SOURCE_FILE=%1"
set "BASE_NAME=%~dpn1"
set "OBJ_FILE=%BASE_NAME%.o"
set "EXE_FILE=%BASE_NAME%"

g++ -c -g -O3 "%SOURCE_FILE%" -IC:/Users/aaron/CODING/Tools/cpp_libs/SFML-2.6.1/include -IC:/Users/aaron/CODING/Tools/cpp_libs/glm-1.0.1-light -DSFML_STATIC -o "%OBJ_FILE%"

g++ "%OBJ_FILE%" -o "%EXE_FILE%" -Wall -Wextra -LC:/Users/aaron/CODING/Tools/cpp_libs/SFML-2.6.1/lib -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -lopengl32 -lwinmm -lgdi32 -lfreetype -static

del "%OBJ_FILE%"