#ifndef _AHOCORASICK
#define _AHOCORASICK

#include <map>
#include <set>
#include <queue>
#include <functional>
#include <memory>
#include <cassert>

// Type of the callback that will be called whenever the automaton finds a hit
typedef std::function<void(int word_index, unsigned long start, unsigned long end)> Callback;

// Node in the trie
template <class Char>
struct Node {
    typedef std::shared_ptr<Node<Char>> NodePtr;
    typedef std::weak_ptr<Node<Char>> NodeWPtr;

    NodeWPtr parent;
    Char edge;
    std::map<Char,NodePtr> children;
    int word_index = -1; // ID of the word that ends at this node (or -1 if no words end here)
    int end = -1; // Length of word that ends at this node (or -1 if not the end)
    NodeWPtr suffix_link;
    NodeWPtr output_link;

    bool has_edge(Char edge);
    NodeWPtr get(Char edge);
    bool is_root() const;
};

template <class Char>
using NodePtr = std::shared_ptr<Node<Char>>;

template <class Char>
using WeakPtr = std::weak_ptr<Node<Char>>;

template <class Char>
bool Node<Char>::has_edge(Char edge) {
    return children.count(edge) > 0;
}

template <class Char>
WeakPtr<Char> Node<Char>::get(Char edge) {
    return children[edge];
}

template <class Char>
bool Node<Char>::is_root() const {
    return parent.expired();
}

// Trie for adding the words to search for
template <class Char>
class Trie {
public:
    Trie() : root(std::make_shared<Node<Char>>()) {}
    //~Trie() { delete root; }
    void add(const Char* word);
    void finish();
    WeakPtr<Char> get_root() const {return root;}
    bool is_finished() const {return finished;}
private:
    NodePtr<Char> root;
    int num_words = 0; // number of words stored in the trie
    bool finished = false;
};

// Does breadth-first search starting from the root
// and performing action on every visited node
template <class Char>
static void breadth_first_search(NodePtr<Char> root, void (*action)(WeakPtr<Char>)) {
    std::queue<WeakPtr<Char>> q;
    q.push(root);
    while (!q.empty()) {
        WeakPtr<Char> x = q.front();
        q.pop();
        action(x);
        for (const auto& kv : x.lock()->children) {
            q.push(kv.second);
        }
    }
}

// Start from node v and follow edge. If child node doesn't exist, create it.
template <class Char>
static WeakPtr<Char> go_to_or_add(WeakPtr<Char> v, Char edge) {
    if (!v.lock()->has_edge(edge)) {
        NodePtr<Char> new_child = std::make_shared<Node<Char>>();
        new_child->edge = edge;
        new_child->parent = v;
        v.lock()->children[edge] = new_child;
    }
    return v.lock()->get(edge);
}

// Adds word to the trie starting at the root
template <class Char>
static void add_word(NodePtr<Char> root, const Char* word, int word_index) {
    WeakPtr<Char> v = root;
    Char* p = (Char*) word;
    int len = 0;
    while (*p) {
        v = go_to_or_add(v, *p);
        p++;
        len++;
    }
    v.lock()->word_index = word_index;
    v.lock()->end = len;
}

// Add suffix link to the node after all the words have been added to the trie.
template <class Char>
static void add_suffix_link(WeakPtr<Char> wa) {
    if (wa.lock()->is_root()) return;
    if (wa.lock()->parent.lock()->is_root()) {
        wa.lock()->suffix_link = wa.lock()->parent;
        return;
    }
    // wa represents a string
    Char a = wa.lock()->edge;
    WeakPtr<Char> w = wa.lock()->parent;
    WeakPtr<Char> x = w.lock()->suffix_link; 
    while (!x.lock()->is_root() && !x.lock()->has_edge(a)) {
        x = x.lock()->suffix_link;
    }
    if (x.lock()->is_root()) {
        wa.lock()->suffix_link = x;
    } else {
        // xa exists
        wa.lock()->suffix_link = x.lock()->get(a);
    }
}

template <class Char>
static void action(WeakPtr<Char> v) {
    // Add suffix link to Node<Char> v
    if (v.lock()->is_root()) return;
    add_suffix_link(v);
    WeakPtr<Char> u = v.lock()->suffix_link;
    // Fill the output links
    if (u.lock()->end) {
        v.lock()->output_link = u;
    } else {
        v.lock()->output_link = u.lock()->output_link;
    }
}

// Adds suffix and output links on the nodes after adding the words
template <class Char>
static void finish_nodes(NodePtr<Char> root) {
    breadth_first_search(root, action);
}

// Adds a word to the trie
template <class Char>
void Trie<Char>::add(const Char* word) {
    assert(!finished);
    add_word(root, word, num_words);
    num_words++;
}

// Call this after finished adding all the words
template <class Char>
void Trie<Char>::finish() {
    if (!finished) {
        finish_nodes(root);
        finished = 1;
    }
}

template <class Char>
static WeakPtr<Char> next_node(WeakPtr<Char> v, Char c) {
    WeakPtr<Char> node = v;
    while (true) {
        if (node.expired()) return node;
        if (node.lock()->has_edge(c)) {
            node = node.lock()->get(c);
            break;
        } else if (node.lock()->is_root()) {
            break;
        } else {
            // Node doesn't have child c
            node = node.lock()->suffix_link;
        }
    }
    return node;
}

template <class Char>
struct WeakCompare {
    bool operator()(const WeakPtr<Char>& a, const WeakPtr<Char>& b) const {
        return a.lock().get() < b.lock().get();
    }
};

template <class Char>
using WeakSet = std::set<WeakPtr<Char>, WeakCompare<Char>>;

template <class Char>
static void do_output(WeakPtr<Char> node, WeakSet<Char> output_nodes, unsigned long i, const Callback& callback) {
    WeakPtr<Char> node2 = node;
    // Go through each output ending at the same position
    unsigned long start;
    while (!node2.expired() && !node2.lock()->is_root()) {
        output_nodes.insert(node2);
        unsigned long end = i + 1;
        start = end - node2.lock()->end;
        callback(node2.lock()->word_index, start, end);
        node2 = node2.lock()->output_link;
    }
}

// Aho-Corasick automaton defined on a trie
template <class Char>
class AhoCorasick {
public:
    AhoCorasick(const Trie<Char>& trie, const Callback& callback) :
        trie(std::make_shared<Trie<Char>>(trie)), 
        states({trie.get_root()}), // Make sure there's a root
        output_callback(callback) {}
    // Feed a character into the automaton
    void next(Char edge);
    void reset(); // Reset back into original state
private:
    typedef WeakSet<Char> NodeSet;
    std::shared_ptr<Trie<Char>> trie;
    Callback output_callback;
    NodeSet states;
    unsigned long i = 0; // position in the string
};

// Follow the edge and set the automaton to its new states
template <class Char>
void AhoCorasick<Char>::next(Char edge) {
    assert(trie->is_finished());
    NodeSet new_states = {trie->get_root()};
    for (const auto& node : states) {
        WeakPtr<Char> nn = next_node(node, edge);
        if (!nn.expired()) {
            new_states.insert(nn);
            if (nn.lock()->word_index != -1) {
                do_output(nn, new_states, i, output_callback);
            }
        }
    }
    states = new_states;
    // Increment location counter
    i++;
}

// Puts the automaton in its starting state
template <class Char>
void AhoCorasick<Char>::reset() {
    states = {trie->get_root()};
    i = 0;
}

#endif