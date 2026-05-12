# Core libs
git submodule add https://github.com/raysan5/raylib.git          third_party/raylib
git submodule add https://github.com/skypjack/entt.git           third_party/entt
git submodule add https://github.com/g-truc/glm.git              third_party/glm
git submodule add https://github.com/nlohmann/json.git           third_party/json
git submodule add https://github.com/erincatto/box2d.git         third_party/box2d
git submodule add https://github.com/wren-lang/wren.git          third_party/wren
git submodule add https://github.com/icculus/physfs.git          third_party/physfs

# Editor-only
git submodule add https://github.com/ocornut/imgui.git           third_party/imgui
git submodule add https://github.com/raylib-extras/rlImGui.git   third_party/rlimgui
git submodule add https://github.com/native-toolkit/tinyfiledialogs.git third_party/tinyfiledialogs

# Pin each to the same version you were using
cd third_party/raylib      && git checkout 5.5              && cd ../..
cd third_party/entt        && git checkout v3.12.2          && cd ../..
cd third_party/glm         && git checkout 1.0.1            && cd ../..
cd third_party/json        && git checkout v3.11.3          && cd ../..
cd third_party/box2d       && git checkout v2.4.1           && cd ../..
cd third_party/wren        && git checkout 0.4.0            && cd ../..
cd third_party/physfs      && git checkout release-3.2.0    && cd ../..
cd third_party/imgui       && git checkout v1.91.8-docking  && cd ../..
cd third_party/rlimgui     && git checkout 0105acf1bfba22e883688e5b9ffb1af0536739e2 && cd ../..

git add third_party/box2d
git add third_party/raylib
git add third_party/entt
git add third_party/glm
git add third_party/json
git add third_party/wren
git add third_party/physfs
git add third_party/imgui
git add third_party/rlimgui
git add third_party/tinyfiledialogs