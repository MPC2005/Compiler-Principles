#include "tree.h"

#include <fstream>

ASTNodePtr makeASTNode(const std::string& kind) {
    return std::make_shared<ASTNode>(kind);
}

void addASTAttribute(const ASTNodePtr& node, const std::string& name,
                     const std::string& value) {
    if (node) {
        node->attributes.push_back(std::make_pair(name, value));
    }
}

static void printASTNode(std::ostream& out, const ASTNodePtr& node,
                         const std::string& prefix, bool last, int& serial,
                         bool root) {
    if (!node) return;
    if (root) {
        out << '[' << serial++ << "] " << node->kind;
    } else {
        out << prefix << (last ? "`-- " : "|-- ")
            << '[' << serial++ << "] " << node->kind;
    }
    for (const std::pair<std::string, std::string>& attribute : node->attributes) {
        out << ' ' << attribute.first << '=' << attribute.second;
    }
    out << '\n';
    const std::string childPrefix = root ? "    " : prefix + (last ? "    " : "|   ");
    for (std::size_t i = 0; i < node->children.size(); ++i) {
        printASTNode(out, node->children[i], childPrefix,
                     i + 1 == node->children.size(), serial, false);
    }
}

void writeASTTree(const ASTNodePtr& root, const std::string& path) {
    std::ofstream out(path.c_str(), std::ios::binary);
    if (out) {
        int serial = 0;
        printASTNode(out, root, "", true, serial, true);
    }
}


