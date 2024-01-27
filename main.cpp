#include <chrono>
#include <filesystem>
#include <map>
#include <ranges>
#include <string>
#include <vector>

#include "pel_print.hpp"

namespace rn = std::ranges;
namespace fs = std::filesystem;

struct Extension_info
{
    std::uintmax_t num_files,
        total_size;
};

// -------------------------

int main()
{
    pel::print("Please insert a root: ");
    auto root_str = std::string{};
    std::getline(std::cin, root_str);

    auto const root = fs::path{root_str};
    if (!fs::is_directory(root))
    {
        pel::print("You must indicate an actual directory");
        return 0;
    }
    // -------------
    using clock = std::chrono::steady_clock;
    auto const start = clock::now();

    auto paths = std::vector<fs::path>{};
    for (fs::path pth : fs::recursive_directory_iterator{root})
    {
        paths.push_back(pth);
    }

    //-----------------------------------------------------------------
    std::uintmax_t directory_counter = 0;
    auto generate_map = [](const std::vector<fs::path> &paths, std::uintmax_t &directory_counter) -> std::map<std::string, Extension_info>
    {
        auto res = std::map<std::string, Extension_info>{};

        for (const fs::path &pth : paths)
        {
            if (fs::is_regular_file(pth))
            {
                auto ext = pth.extension().string();
                auto size = fs::file_size(pth);

                res[ext].num_files++;
                res[ext].total_size += size;
            }
            else if (fs::is_directory(pth))
            {
                directory_counter++;
            }
        }

        return res;
    };
    // -----------------------------
    auto processed_data = generate_map(paths, directory_counter);

    auto const stop = clock::now();

    auto root_file = std::uintmax_t{0};
    auto root_size = std::uintmax_t{0};

    for (auto const &[ext, info] : processed_data)
    {
        root_file += info.num_files;
        root_size += info.total_size;
        pel::println("{:>15}: {:>8} files {:>16} bytes", ext, info.num_files, info.total_size);
    }

    auto const duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    pel::println("\n{:>15}: {:>8} files {:>16} bytes | {} folders [{} ms]",
                 "Total", root_file, root_size, directory_counter, duration.count());

    return 0;
}
