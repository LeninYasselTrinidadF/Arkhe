#include "search_utils.hpp"
#include <queue>

std::shared_ptr<MathNode> find_node_by_code(
    const std::shared_ptr<MathNode>& root,
    const std::string& target)
{
    if (!root) return nullptr;
    std::queue<std::shared_ptr<MathNode>> q;
    q.push(root);
    while (!q.empty()) {
        auto n = q.front(); q.pop();
        if (n->code == target) return n;
        for (auto& c : n->children) q.push(c);
    }
    return nullptr;
}

std::shared_ptr<MathNode> find_node_by_module(
    const std::shared_ptr<MathNode>& root,
    const std::string& module)
{
    if (!root || module.empty()) return nullptr;

    auto exact = find_node_by_code(root, module);
    if (exact) return exact;

    std::queue<std::shared_ptr<MathNode>> q;
    q.push(root);
    std::shared_ptr<MathNode> best;
    int best_len = 0;
    while (!q.empty()) {
        auto n = q.front(); q.pop();
        if (!n->code.empty()
            && module.find(n->code) != std::string::npos
            && (int)n->code.size() > best_len)
        {
            best     = n;
            best_len = (int)n->code.size();
        }
        for (auto& c : n->children) q.push(c);
    }
    return best;
}
