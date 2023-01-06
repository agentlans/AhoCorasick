#include <iostream>
#include "AhoCorasick.hpp"

void callback(int word_index, unsigned long start, unsigned long end) {
    std::cout << "Found word " << word_index << 
    	" at index [" << start << ", " << end << ")" << std::endl;
}

int main() {
    Trie<char> ac;
    ac.add("He"); // word 0
    ac.add("Hello"); // word 1
    ac.add("HelloWorld"); // word 2
    ac.add("loW"); // word 3
    ac.finish();

    AhoCorasick<char> bar1(ac, callback);
    char s[] = "11234HelloHelloWorld1234";
    // Feed the string one character at a time into the automaton
    // The callback will be called automatically.
    for (const auto& ch : s) {
        bar1.next(ch);
    }
    bar1.reset();
}
