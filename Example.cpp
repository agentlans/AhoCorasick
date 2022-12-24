#include <iostream>
#include "AhoCorasick.hpp"

void callback(int word_index, unsigned long start, unsigned long end) {
    std::cout << "Found word " << word_index << " at " << start << " to " << end << std::endl;
}

int main() {
    Trie<char> ac;
    ac.add("He");
    ac.add("Hello");
    ac.add("HelloWorld");
    ac.add("loW");
    ac.finish();

    AhoCorasick<char> bar1(ac, callback);
    char s[] = "11234HelloHelloWorld1234";
    for (const auto& ch : s) {
        bar1.next(ch);
    }
    bar1.reset();
}