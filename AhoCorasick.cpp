#include <vector>
#include <queue>
#include <memory>
#include <map>
#include <set>
#include <string>
#include <algorithm>

template <class T, class U>
bool contains(const T& container, U item) {
    return (container.count(item) > 0);
}

// Sets s1 = union(s1, s2)
template <class SetType>
void set_union(SetType& s1, const SetType& s2) {
    std::copy(s2.begin(), s2.end(), std::inserter(s1, s1.end()));
}

// Node in the trie
template <class Char>
struct Node : std::enable_shared_from_this<Node<Char>> {
    typedef std::shared_ptr<Node<Char>> NodePtr;
    // Attributes
    std::weak_ptr<Node<Char>> parent;
    Char edge;
    std::map<Char, NodePtr> children;
    bool end = false;
    std::weak_ptr<Node<Char>> suffix_link;
    std::weak_ptr<Node<Char>> output_link;
    NodePtr go_to_or_add(Char edge) {
        if (!contains(children, edge)) {
            NodePtr new_child = std::make_shared<Node<Char>>();
            new_child->edge = edge;
            new_child->parent = this->shared_from_this();
            children[edge] = new_child;
        }
        return children[edge];
    }
    // Whether this node is a root node
    bool is_root() {
        return parent.expired();
    }
};

template <class Char>
using NodeT = std::shared_ptr<Node<Char>>;

template <class Char>
using NodeW = std::weak_ptr<Node<Char>>;

// Trie for Aho-Corasick algorithm
template <class Char>
struct Trie {
    Trie() : root(std::make_shared<Node<Char>>()) {}
    // Adds a word to the trie.
    void add(const Char* word) {
        std::weak_ptr<Node<Char>> v = root;
        auto p = word;
        while (*p) {
            v = v.lock()->go_to_or_add(*p);
            p++;
        }
        v.lock()->end = true;
    }
    // Call this after finish adding words.
    void finish();

    NodeT<Char> root;
};

// Calls action() on root and its neighbours in breadth-first search manner
template <class T, class NeighbourFunc, class ActionFunc>
void breadth_first_search(const T& root, const NeighbourFunc& neighbours, const ActionFunc& action) {
    std::queue<T> q = std::queue<T>();
    q.push(root);
    while (!q.empty()) {
        T x = q.front();
        q.pop();
        action(x);
        for (const auto& v : neighbours(x)) {
            q.push(v);
        }
    }
}

template <class Char>
void add_suffix_link(Node<Char>& wa) {
    if (wa.is_root()) return;
    if (wa.parent.lock()->is_root()) {
        wa.suffix_link = wa.parent;
        return;
    }
    // wa represents a string
    Char a = wa.edge;
    NodeT<Char> w = wa.parent.lock();
    NodeT<Char> x = w->suffix_link.lock(); 
    while (!x->is_root() && !contains(x->children, a)) {
        x = x->suffix_link.lock();
    }
    if (x->is_root()) {
        wa.suffix_link = x;
    } else {
        // xa exists
        wa.suffix_link = x->children[a];
    }
}

template <class Char>
struct ActionFunc {
    void operator()(NodeT<Char> v) const {
        // Add suffix link to Node v
        if (v->is_root()) return; // -------------------------------------------
        add_suffix_link(*v);
        NodeT<Char> u = v->suffix_link.lock();
        // Fill the output links
        if (u->end) {
            v->output_link = u;
        } else {
            v->output_link = u->output_link;
        }
    }
};

template <class Char>
struct NeighbourFunc {
    typedef NodeT<Char> NodeType;
    std::vector<NodeType> operator()(NodeT<Char> v) const {
        std::vector<NodeType> results;
        // Get the children of this node
        std::transform(
            v->children.begin(), v->children.end(),
            std::back_inserter(results),
            [](const auto& kv){return kv.second;});
        return results;
    }
};

template <class Char>
void Trie<Char>::finish() {
    NeighbourFunc<Char> neighbours;
    ActionFunc<Char> action;
    breadth_first_search(root, neighbours, action);
}

template <class Char>
void get_string(std::string& s, NodeT<Char> node) {
    auto x = node;
    s.clear();
    while (!x->is_root()) {
        s = x->edge + s;
        x = x->parent.lock();
    }
}

template <class Char>
NodeT<Char> next_node(const NodeT<Char> v, Char c) {
    NodeT<Char> node = v;
    while (true) {
        if (!node) return nullptr;
        if (contains(node->children, c)) {
            node = node->children[c];
            break;
        } else if (node->is_root()) {
            break;
        } else {
            // Node doesn't have child c
            node = node->suffix_link.lock();
        }
    }
    return node;
}

template <class Char, class Output>
std::set<NodeT<Char>> do_output(NodeT<Char> node, int i, Output callback) {
    typedef NodeT<Char> NodeT;
    NodeT node2 = node;
    std::set<NodeT> output_nodes;
    // Go through each output ending at the same position
    int start;
    std::string str;
    while (node2 && !node2->is_root()) {
        output_nodes.insert(node2);
        get_string(str, node2);
        start = i - str.size() + 1;
        callback(start, str);
        node2 = node2->output_link.lock();
    }
    return output_nodes;
}

template <class Char, class Output>
class AhoCorasick {
public:
    typedef std::set<NodeT<Char>> NodeSet;
    AhoCorasick(const Output& output) : output(output) {
        reset();
    }
    void add(const Char* str) {
        trie.add(str);
    }
    void finish() {
        trie.finish();
    }
    void next(Char edge) {
        NodeSet new_states;
        new_states.insert(trie.root);
        for (const auto& node : states) {
            auto nn = next_node(node, edge);
            if (!nn) continue;
            new_states.insert(nn);
            if (nn->end) {
                auto output_nodes = do_output(nn, i, output);
                set_union(new_states, output_nodes);
            }
        }
        states = new_states;
        i++;
    }
    void reset() {
        states = {trie.root};
        i = 0;
    }
private:
    Trie<Char> trie;
    Output output;
    NodeSet states;
    int i = 0;
};

#include <iostream>

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

    char input[] = "11234HelloHelloWorld1234";
    for (const auto& x : input) {
    	ac.next(x);
    }
    ac.reset();
}

