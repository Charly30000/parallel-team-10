#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <map>
#include <ranges>
#include <string>
#include <thread>
#include <vector>

#include "pel_print.hpp"

namespace rn = std::ranges;
namespace fs = std::filesystem;

struct Extension_info
{
    std::uintmax_t num_files,
        total_size;
};

auto main() -> int
{
    pel::print("Please insert a root: ");
    auto root_str = std::string{};
    std::getline(std::cin, root_str);

    auto const root = fs::path{root_str};

    if (!fs::is_directory(root))
    {
        pel::print("you must indicate an actual directory");
        return 0;
    }

    using clock = std::chrono::steady_clock;
    auto const start = clock::now();

    auto paths = std::vector<fs::path>{};
    for (fs::path pth : fs::recursive_directory_iterator{root})
    {
        paths.push_back(pth);
    }

    // ------------------------------------------
    using map_type = std::map<std::string, Extension_info>;
    auto processed_data = map_type{};

    auto directory_counter = std::atomic<std::uintmax_t>{0};

    auto generate_map = [&](auto chunk) -> map_type
    {
        auto res = map_type{};
        for (fs::path const &pth : chunk)
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
    // -------------------------------------------------

    auto const hardw_threads = std::thread::hardware_concurrency();
    auto const num_futures = hardw_threads - 1;
    auto const max_chunk_sz = paths.size() / hardw_threads;

    auto futures = std::vector<std::future<map_type>>{};


    for (std::size_t i = 0; i < num_futures; ++i)
    {
        auto chunk_begin = paths.begin() + i * max_chunk_sz;
        auto chunk_end = (i == num_futures - 1) ? paths.end() : chunk_begin + max_chunk_sz;
        futures.push_back(std::async(std::launch::async, generate_map, std::vector<fs::path>{chunk_begin, chunk_end}));
    }

    // -------------------------------------------------
    auto process_map = [&](map_type const &partial_map) -> void
    {
        for (auto const &[ext, info] : partial_map)
        {
            processed_data[ext].num_files += info.num_files;
            processed_data[ext].total_size += info.total_size;
        }
    };
    
    // -------------------------------------------------

    for (auto &future : futures)
    {
        process_map(future.get());
    }

    auto root_file = std::uintmax_t{0};
    auto root_size = std::uintmax_t{0};

    // -------------------------------------------------

    for (auto const &[ext, info] : processed_data)
    {
        root_file += info.num_files;
        root_size += info.total_size;
        pel::println("{:>15}: {:>8} files {:>16} bytes", ext, info.num_files, info.total_size);
    }

    auto const stop = clock::now();
    auto const duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    pel::println("\n{:>15}: {:>8} files {:>16} bytes | {} folders [{} ms]",
                 "Total", root_file, root_size, directory_counter.load(), duration.count());

    return 0;
}
