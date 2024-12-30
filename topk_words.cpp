#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <future>

const size_t TOPK = 10;
using Counter = std::map<std::string, std::size_t>;
using FileCounters = std::vector<Counter>;

std::string tolower(const std::string& str);
Counter count_words(std::istream& stream);
void print_topk(std::ostream& stream, const Counter& counter, const size_t k);
Counter merge_counters(const FileCounters& counters);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: topk_words [FILES...]\n";
        return EXIT_FAILURE;
    }

    auto start = std::chrono::high_resolution_clock::now();
    FileCounters file_counters(argc - 1);

    std::vector<std::future<Counter>> futures;
    for (int i = 1; i < argc; ++i) {
        futures.push_back(std::async(std::launch::async, [file = argv[i]]() {
            std::ifstream input{file};
            if (!input.is_open()) {
                std::cerr << "Failed to open file " << file << '\n';
                return Counter{};
            }
            return count_words(input);
        }));
    }

    for (int i = 0; i < futures.size(); ++i) {
        file_counters[i] = futures[i].get();
    }

    Counter global_counter = merge_counters(file_counters);
    print_topk(std::cout, global_counter, TOPK);

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Elapsed time is " << elapsed_ms.count() << " us\n";

    return EXIT_SUCCESS;
}

std::string tolower(const std::string& str) {
    std::string lower_str;
    std::transform(std::cbegin(str), std::cend(str),
                   std::back_inserter(lower_str),
                   [](unsigned char ch) { return std::tolower(ch); });
    return lower_str;
}

Counter count_words(std::istream& stream) {
    Counter counter;
    std::for_each(std::istream_iterator<std::string>(stream),
                  std::istream_iterator<std::string>(),
                  [&counter](const std::string& s) { ++counter[tolower(s)]; });
    return counter;
}

Counter merge_counters(const FileCounters& counters) {
    Counter merged_counter;
    for (const auto& counter : counters) {
        for (const auto& [word, count] : counter) {
            merged_counter[word] += count;
        }
    }
    return merged_counter;
}

void print_topk(std::ostream& stream, const Counter& counter, const size_t k) {
    std::vector<Counter::const_iterator> words;
    words.reserve(counter.size());
    for (auto it = std::cbegin(counter); it != std::cend(counter); ++it) {
        words.push_back(it);
    }

    size_t actual_k = std::min(k, words.size());
    std::partial_sort(
        std::begin(words), std::begin(words) + actual_k, std::end(words),
        [](auto lhs, auto& rhs) { return lhs->second > rhs->second; });

    for (size_t i = 0; i < actual_k; ++i) {
        stream << std::setw(4) << words[i]->second << " " << words[i]->first << '\n';
    }
}
