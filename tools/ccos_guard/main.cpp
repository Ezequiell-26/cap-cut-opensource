#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Guard {
    fs::path root;
    int errors = 0;

    void fail(const std::string& message) {
        std::cerr << "ERROR: " << message << '\n';
        ++errors;
    }

    bool exists(std::string_view relative) const {
        return fs::exists(root / fs::path(relative));
    }

    static bool sourceTree(const fs::path& path) {
        const auto rel = path.generic_string();
        return rel.rfind("src/", 0) == 0 || rel.rfind("apps/", 0) == 0 ||
               rel.rfind("engine/", 0) == 0 || rel.rfind("application/", 0) == 0 ||
               rel.rfind("infrastructure/", 0) == 0 || rel.rfind("platform/", 0) == 0 ||
               rel.rfind("presentation/", 0) == 0 || rel.rfind("plugins/", 0) == 0 ||
               rel.rfind("tools/", 0) == 0;
    }

    static bool bannedRuntimeExtension(const fs::path& path) {
        static const std::unordered_set<std::string> banned = {
            ".js", ".mjs", ".cjs", ".ts", ".tsx", ".py", ".rs", ".go",
            ".java", ".kt", ".swift"
        };
        return banned.contains(path.extension().string());
    }

    static std::string readText(const fs::path& path) {
        std::ifstream input(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input), {});
    }

    void checkRequiredFiles() {
        for (const auto required : {"CMakeLists.txt", "README.md", "LICENSE", "AGENTS.md", "docs/PROJECT_MEMORY.md"}) {
            if (!exists(required)) fail(std::string("missing required file: ") + required);
        }
    }

    void checkTree() {
        if (!fs::exists(root)) {
            fail("repository root does not exist");
            return;
        }

        for (const auto& entry : fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied)) {
            if (!entry.is_regular_file()) continue;
            const auto relPath = fs::relative(entry.path(), root);
            const auto generic = relPath.generic_string();
            if (generic.rfind(".git/", 0) == 0 || generic.rfind("build/", 0) == 0 ||
                generic.rfind("build-", 0) == 0 || generic.find("/_deps/") != std::string::npos) continue;

            if (sourceTree(relPath)) {
                if (bannedRuntimeExtension(relPath)) {
                    fail("non-C++ runtime source in project tree: " + generic);
                }
                const auto ext = relPath.extension().string();
                if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".hpp" || ext == ".h") {
                    const std::string content = readText(entry.path());
                    if (content.find("<<<<<<< ") != std::string::npos ||
                        content.find(">>>>>>> ") != std::string::npos) {
                        fail("merge conflict marker in " + generic);
                    }
                    const std::string unboundedWait = "waitForFinished(" + std::string("-1)");
                    if (content.find(unboundedWait) != std::string::npos) {
                        fail("unbounded QProcess wait in " + generic);
                    }
                }
            }
        }
    }

    void checkCMakeSources() {
        const auto cmakePath = root / "CMakeLists.txt";
        if (!fs::exists(cmakePath)) return;
        const std::string content = readText(cmakePath);
        const std::regex sourcePattern(R"((?:src|apps|engine|application|infrastructure|platform|presentation|plugins)/[^\s\)]+\.(?:cpp|cc|cxx|hpp|h)))");
        for (std::sregex_iterator it(content.begin(), content.end(), sourcePattern), end; it != end; ++it) {
            const auto path = it->str();
            if (!fs::exists(root / path)) fail("CMake references missing source: " + path);
        }
    }

    int run() {
        checkRequiredFiles();
        checkTree();
        checkCMakeSources();
        std::cout << "ccos_guard: " << errors << " error(s)\n";
        return errors == 0 ? 0 : 1;
    }
};

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: ccos_guard <repository-root>\n";
        return 2;
    }

    std::error_code error;
    const fs::path root = fs::weakly_canonical(fs::path(argv[1]), error);
    if (error) {
        std::cerr << "invalid repository root: " << error.message() << '\n';
        return 2;
    }

    Guard guard{root};
    return guard.run();
}
