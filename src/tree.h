#ifndef TOYC_TREE_H
#define TOYC_TREE_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

struct ASTNode {
    std::string kind;
    std::vector<std::pair<std::string, std::string> > attributes;
    std::vector<std::shared_ptr<ASTNode> > children;

    explicit ASTNode(const std::string& nodeKind) : kind(nodeKind) {}
};

typedef std::shared_ptr<ASTNode> ASTNodePtr;

ASTNodePtr makeASTNode(const std::string& kind);
void addASTAttribute(const ASTNodePtr& node, const std::string& name,
                     const std::string& value);
void writeASTTree(const ASTNodePtr& root, const std::string& path);

#endif

