# About

This library, called eez-framework, is used in combination with the [EEZ Studio](https://github.com/eez-open/studio). When EEZ Studio generates C/C++ for your eez-project, and your eez-project is using EEZ Flow or EEZ-GUI, then you will need this library. It is written in C++.

# For LVGL users

Only if you want to use EEZ Flow with your LVGL based project you will need this libary.

---

If you are using CMake there is `CMakeLists.txt` file so you can link eez-framework with your project as static library. Here is what you need to add to your `CMakeLists.txt` file:

```
add_definitions(-DEEZ_FOR_LVGL)

add_subdirectory(eez-framework)

target_link_libraries(your_project_name
    lvgl
    eez-framework
)
```

This assumes that eez-framework is located in the same directory as your `CMakeLists.txt` file (for example you can add this repository as git submodule).

---

For all other build systems:

-   define `EEZ_FOR_LVGL` globally
-   add `<path-to-eez-framework>/src` to include directories
-   compile all _.cpp and _.c files from this repository together with your source files
