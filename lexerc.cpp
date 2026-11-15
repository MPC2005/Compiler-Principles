#include <iostream>
#include <string>
#include <vector>

using namespace std;

vector<string> terminal1 = {
    "int", "const", "void",
    "if", "else", "while", "return",
    "break", "continue"
};

bool isKeyword(const string &s) {
    for (auto &k : terminal1) {
        if (s == k) return true;
    }
    return false;
}

bool isIdentStart(char c) {
    return c == '_' ||
           (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z');
}

bool isIdentChar(char c) {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

bool isSingleOp(char c) {
    string ops = "+-*/%!=><(){};,";
    return ops.find(c) != string::npos;
}

string tokenKwOp(const string &s) {
    return "'" + s + "':\"" + s + "\"";
}

string tokenIdent(const string &s) {
    return "Ident:\"" + s + "\"";
}

string tokenInt(const string &s) {
    return "IntConst:\"" + s + "\"";
}

string quoteCharForError(const string &ch) {
    // 期望中单引号字符输出为 "'"，避免出现 ''' 形式
    if (ch == "'") return "\"'\"";
    return "'" + ch + "'";
}

string formatError(int line, const string &ch) {
    return to_string(line) + " illegal character " + quoteCharForError(ch);
}

int utf8Len(unsigned char b) {
    if ((b & 0x80) == 0) return 1;
    if ((b & 0xE0) == 0xC0) return 2;
    if ((b & 0xF0) == 0xE0) return 3;
    if ((b & 0xF8) == 0xF0) return 4;
    return 1;
}

int main() {
    string str;
    char ch;
    while (cin.get(ch)) {
        str += ch;
    }

    int n = (int)str.size();
    int line_no = 0; // 按期望，错误行号从 0 开始
    vector<string> tokens;
    vector<string> errors;

    for (int i = 0; i < n; ) {
        char c = str[i];

        if (c == '\n') {
            ++line_no;
            ++i;
            continue;
        }

        if (c == ' ' || c == '\t' || c == '\r') {
            ++i;
            continue;
        }

        // 单行注释
        if (c == '/' && i + 1 < n && str[i + 1] == '/') {
            i += 2;
            while (i < n && str[i] != '\n') ++i;
            continue;
        }

        // 多行注释
        if (c == '/' && i + 1 < n && str[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(str[i] == '*' && str[i + 1] == '/')) {
                if (str[i] == '\n') ++line_no;
                ++i;
            }
            if (i + 1 < n) i += 2;
            else i = n;
            continue;
        }

        // 标识符/关键字
        if (isIdentStart(c)) {
            int j = i + 1;
            while (j < n && isIdentChar(str[j])) ++j;
            string word = str.substr(i, j - i);
            if (isKeyword(word)) {
                tokens.push_back(tokenKwOp(word));
            } else {
                tokens.push_back(tokenIdent(word));
            }
            i = j;
            continue;
        }

        // 数字字面量，与原代码类似：连续字母数字下划线作为整体
        if (c >= '0' && c <= '9') {
            int j = i;
            while (j < n && isIdentChar(str[j])) ++j;
            string word = str.substr(i, j - i);
            tokens.push_back(tokenInt(word));
            i = j;
            continue;
        }

        // 双字符合法运算符
        if (i + 1 < n) {
            string two = str.substr(i, 2);
            if (two == "==" || two == "!=" ||
                two == ">=" || two == "<=" ||
                two == "&&" || two == "||") {
                tokens.push_back(tokenKwOp(two));
                i += 2;
                continue;
            }
        }

        // 单字符合法运算符/标点
        if (isSingleOp(c)) {
            tokens.push_back(tokenKwOp(string(1, c)));
            ++i;
            continue;
        }

        // 非 ASCII 字符，按 UTF-8 字符整体报错，例如中文
        if ((unsigned char)c >= 0x80) {
            int len = utf8Len((unsigned char)c);
            if (i + len > n) len = n - i;
            string chstr = str.substr(i, len);
            errors.push_back(formatError(line_no, chstr));
            i += len;
            continue;
        }

        // 其他非法 ASCII 字符，例如 $ # ? : ~ ^ & | \ ' 等
        errors.push_back(formatError(line_no, string(1, c)));
        ++i;
    }

    // 先输出错误信息
    if (!errors.empty()) {
        cout << "reject\n";
        for (auto &e : errors) {
            cout << e << "\n";
        }
        cout << endl;
    }

    // 再输出 token 流，编号从 0 开始
    for (int k = 0; k < (int)tokens.size(); ++k) {
        cout << k << ":" << tokens[k] << "\n";
    }

    return 0;
}