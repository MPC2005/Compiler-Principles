#include <iostream>
#include <string>
#include <vector>

using namespace std;

vector <string> terminal1 = {"int", "const", "void", 
                            "if", "else", "while", "return", 
                            "break", "continue"};

vector <string> terminal2 = {"(", ")", "{", "}", ";", ",",
                            "=", "+", "-", "*", "/", "%", "!",
                            ">", "<", "==", "!=", ">=", "<=", 
                            "&&", "||", "!"};

int tot, state;
char ch;
string str, cur;

int main() {
    while (cin.get(ch))  {
        str += ch;
    }
    for (int i = 0; i < str.length(); i++) {
        ch = str[i];
        if (ch == '_' || isalpha(ch) || isdigit(ch)) {
            if (state == 2) {
                for (auto &s : terminal2) {
                    if (cur == s) {
                        cout << tot++ << ":" << "'" << cur << "':" << "\"" << cur << "\"" << endl;
                        break;
                    }
                }
                cur.clear();
            }
            state = 1, cur += ch;
        }
        else if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || 
                 ch == '%' || ch == '!' || ch == '=' || ch == '>' || 
                 ch == '<' || ch == '&' || ch == '|' || ch == '(' || 
                 ch == ')' || ch == '{' || ch == '}' || ch == ';' || ch == ',') {

            if (state != 0) {
                for(auto &s : terminal1) {
                    if (cur == s) {
                        cout << tot++ << ":" << "'" << cur << "':" << "\"" << cur << "\"" << endl;
                        state = 0;
                        break;
                    }
                }
                // if (ch == '&' || ch == '|') {
                //     if (i < str.length() - 1 && str[i + 1] == ch) {
                //         cur += ch;
                //         i++;
                //     }
                // }
                // else if (ch == '=' || ch == '>' || ch == '<' || ch == '!') {
                //     if (i < str.length() - 1 && str[i + 1] == '=') {
                //         cur += '=';
                //         i++;
                //     }
                // }
                // else if (ch == '/') {
                //     if (i < str.length() - 1 && str[i + 1] == '/') {
                //         while (i < str.length() && str[i] != '\n') {
                //             i++;
                //         }
                //         state = 0;
                //         continue;
                //     }
                //     else if (i < str.length() - 1 && str[i + 1] == '*') {
                //         i += 2;
                //         while (i < str.length() - 1 && !(str[i] == '*' && str[i + 1] == '/')) {
                //             i++;
                //         }
                //         i += 2;
                //         state = 0;
                //         continue;
                //     }
                // }
                for (auto &s : terminal2) {
                    if (cur == s) {
                        cout << tot++ << ":" << "'" << cur << "':" << "\"" << cur << "\"" << endl;
                        state = 0;
                        break;
                    }
                }
                if (state != 0 && (cur[0] == '_' || isalpha(cur[0]))) {
                    cout << tot++ << ":" << "Ident:" << "\"" << cur << "\"" << endl;
                }
                else if (isdigit(cur[0])) {
                    cout << tot++ << ":" << "IntConst:" << cur << endl;
                }
                cur.clear();
            }
            if (ch == '/') {
                if (i < str.length() - 1 && str[i + 1] == '/') {
                    while (i < str.length() && str[i] != '\n') {
                        i++;
                    }
                    state = 0;
                    continue;
                }
                else if (i < str.length() - 1 && str[i + 1] == '*') {
                    i += 2;
                    while (i < str.length() - 1 && !(str[i] == '*' && str[i + 1] == '/')) {
                        i++;
                    }
                    i += 2;
                    state = 0;
                    continue;
                }
                else {
                    state = 2, cur += ch;
                }
            }
            else {
                state = 2, cur += ch;
                if (ch == '&' || ch == '|') {
                    if (i < str.length() - 1 && str[i + 1] == ch) {
                        cur += ch, i++;
                    }
                }
                else if (ch == '=' || ch == '>' || ch == '<' || ch == '!') {
                    if (i < str.length() - 1 && str[i + 1] == '=') {
                        cur += '=', i++;
                    }
                }
            }
        }
        else if (ch == ' ' || ch == '\n' || ch == '\t') {
            if (state != 0) {
                for(auto &s : terminal1) {
                    if (cur == s) {
                        cout << tot++ << ":" << "'" << cur << "':" << "\"" << cur << "\"" << endl;
                        state = 0;
                        break;
                    }
                }
                for (auto &s : terminal2) {
                    if (cur == s) {
                        cout << tot++ << ":" << "'" << cur << "':" << "\"" << cur << "\"" << endl;
                        state = 0;
                        break;
                    }
                }
                if (state != 0 && (cur[0] == '_' || isalpha(cur[0]))) {
                    cout << tot++ << ":" << "Ident:" << "\"" << cur << "\"" << endl;
                }
                else if (isdigit(cur[0])) {
                    cout << tot++ << ":" << "IntConst:" << cur << endl;
                }
            }
            state = 0, cur.clear();
        }
    }
    return 0;
}
