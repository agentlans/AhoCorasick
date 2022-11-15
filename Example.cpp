#include <iostream>
#include <string>

#include "AhoCorasick.hpp"

// Main program
int main() {
    auto o = [](int start, const std::string& str) {
    	std::cout << str << " found at position " << start << std::endl;
    };
    AhoCorasick<char,decltype(o)> ac(o);
    ac.add("He");
    ac.add("Hello");
    ac.add("HelloWorld");
    ac.add("loW");
    ac.finish();
    auto ac2 = ac;

    char input[] = "11234HelloHelloWorld1234";
    for (const auto& x : input) {
    	ac2.next(x);
    }
    ac2.reset();
}
