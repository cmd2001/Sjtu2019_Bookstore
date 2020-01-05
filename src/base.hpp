//
// Created by Amagi on 1/3/20.
//

#ifndef BOOKSTORE_BASE_HPP
#define BOOKSTORE_BASE_HPP

#include <iostream>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <vector>
#include <set>
#include <iomanip>
#include <sstream>
#include <stack>
#ifndef debug
#define debug cerr
#endif
#ifndef hash_t
typedef unsigned long long hash_t;
#endif
using namespace std;

class Input {
private:
    bool type;
    ifstream fin;
public:
    Input(): type(0) {}
    void hook(const string name) {
        type = 1;
        fin.open(name.c_str());
    }
    bool getLine(string &ret) {
        if(fin.eof()) return 0;
        if(!type) getline(cin, ret);
        else getline(fin, ret);
        return 1;
    }
};

enum Property {
    ISBN, NAME, AUTHOR, KEYWORD, PRICE
};

struct Book {
    string data[5];
    int rem;
    friend ostream& operator << (ostream &os, const Book &x) {
        for(int i = 0; i < 5; i++) os << x.data[i] << " ";
        os << x.rem;
        return os;
    }
    friend bool operator < (const Book &a, const Book &b) {
        return *a.data < *b.data;
    }
};

// only storage book id in database, access book data by its id.
// the judge server should contain a SSD.

class BookNotFound {}; // error class: book not found.

class FileChecker {
public:
    bool operator () (const string &x) {
        ifstream file(x.c_str());
        return file.good();
    }
};

class BookPool { // direct access book file.
private:
    string pathPrf, pathSuf;
public:
    BookPool() {
        pathPrf = "books_", pathSuf = ".book";
    }
    void addBook(const Book &x) {
        const string path = pathPrf + x.data[ISBN] + pathSuf;
        ofstream fout(path.c_str());
        for(int i = 0; i < 5; i++) fout << x.data[i] << endl;
        fout << x.rem << endl;
    }
    void removeBook(const string &isbn) {
        const string path = pathPrf + isbn + pathSuf;
        remove(path.c_str());
    }
    Book getBook(const string isbn) {
        const string path = pathPrf + isbn + pathSuf;
        FileChecker checkFile;
        if(!checkFile(path)) throw BookNotFound();
        Book ret;
        ifstream fin(path.c_str());
        for(int i = 0; i < 5; i++) getline(fin, ret.data[i]);
        fin >> ret.rem;
        return ret;
    }
};

class StringHasher {
public:
    hash_t operator () (const string &x) {
        static constexpr hash_t hash_Base = 31;
        hash_t ret = 0;
        for(unsigned i = 0; i < x.length(); i++) ret = ret * hash_Base + (hash_t) x[i];
        return ret;
    }
    string toString(const string &s) {
        return toString(this->operator()(s));
    }
    string toString(hash_t x) {
        static constexpr hash_t hashMod = 32768;
        string ret = "";
        x &= (hashMod - 1);
        while(x) ret = ret + (char)((x % 10) + 'a'), x /= 10;
        if(!ret.length()) ret = "0";
        reverse(ret.begin(), ret.end());
        return ret;
    }
};

// throw error when selecting a book that not exist.
class DatabaseCore { // storage and find by hash.
private:
    Property cur;
    string pathPrf, pathSuf;
    set<hash_t> used; // storage used hash place, for showALL.
public:
    void init(const Property _cur, const string _pathPrf)  {
        cur = _cur, pathPrf = _pathPrf, pathSuf = ".database";
        if(!cur) {
            const string path = pathPrf + "usedHash" + pathSuf;
            FileChecker checkFile;
            if(!checkFile(path)) return;

            ifstream fin(path);
            hash_t x;
            while(fin >> x) used.insert(x);
        }
    }
    ~DatabaseCore() {
        if(!cur) {
            const string path = pathPrf + "usedHash" + pathSuf;

            remove(path.c_str());
            ofstream fout(path);
            for(auto t: used) fout << t << endl;
        }
    }
    vector<Book> searchBook(const string &tar, BookPool &pool) { // return full information of target book.
        StringHasher hashStr;
        FileChecker checkFile;
        string path = pathPrf + hashStr.toString(tar) + pathSuf;
        // debug << "path = " << path << endl;
        vector<Book> ret;

        if(!checkFile(path)) return ret;

        // debug << "in sb tar = " << tar << endl;

        ifstream fin(path.c_str());
        string isbn;
        Book t;
        while(fin >> isbn) {
            try {
                t = pool.getBook(isbn);
            } catch(BookNotFound) {
                debug << "Book Not Found when requesting bookPool, this shouldn't happen." << endl;
                debug << "Please check your dataBase update policy." << endl;
                continue;
            }
            if(cur != KEYWORD) {
                if(t.data[cur] == tar) ret.push_back(t);
            } else {
                const unsigned pos = t.data[cur].find(tar);
                if(pos == t.data[cur].npos) continue;
                if(pos && t.data[cur][pos - 1] != '|') continue;
                if(pos + tar.length() < t.data[cur].length() && t.data[cur][pos + tar.length()] != '|') continue;
                ret.push_back(t);
            }
        }
        return ret;
    }
    void removeBook(const string &tar, const string &tarISBN) {
        StringHasher hashStr;
        FileChecker checkFile;
        string path = pathPrf + hashStr.toString(tar) + pathSuf;

        if(!checkFile(path)) {
            debug << "Nothing to Remove, this may because you are selecting a book that not exist." << endl;
            return;
        }

        ifstream fin(path.c_str());
        vector<string> nw;
        string isbn;
        Book t;
        while(fin >> isbn) {
            if(isbn != tarISBN) nw.push_back(isbn);
        }

        remove(path.c_str());
        ofstream fout(path.c_str());
        for(auto t: nw) fout << t << endl;

        if(!cur && !nw.size()) used.erase(hashStr(tar)); // if cur == 0, then maintain used hash list for show all command.
    }
    void addBook(const Book &x) {
        StringHasher hashStr;
        FileChecker checkFile;
        string path = pathPrf + hashStr.toString(x.data[cur]) + pathSuf;

        ofstream fout(path.c_str(), ios::app);
        fout << x.data[ISBN] << endl;

        if(!cur) used.insert(hashStr(x.data[cur]));
    }
    void addBook(const Book &x, const string &tag) {
        StringHasher hashStr;
        FileChecker checkFile;
        string path = pathPrf + hashStr.toString(tag) + pathSuf;

        ofstream fout(path.c_str(), ios::app);
        fout << x.data[ISBN] << endl;

        if(!cur) used.insert(hashStr(x.data[cur]));
    }

    vector<Book> showAll(BookPool &bp) {
        vector<Book> ret;
        StringHasher hashStr;

        for(hash_t i: used) {
            string path = pathPrf + hashStr.toString(i) + pathSuf;

            FileChecker checkFile;
            if(!checkFile(path)) {
                debug << "file not found in showALL, check your maintain policy." << endl;
                continue;
            }

            ifstream fin(path.c_str());
            string s;
            Book t;
            while(fin >> s) {
                try {
                    t = bp.getBook(s);
                } catch(BookNotFound) {
                    debug << "Book Not Found when requesting bookPool, this shouldn't happen" << endl;
                    debug << "Please check your dataBase update policy" << endl;
                    continue;
                }
                ret.push_back(t);
            }
        }
        return ret;
    }

};

struct User {
    string id, password, name;
    int type;
    friend istream& operator >> (istream &in, User &x) {
        in >> x.id >> x.password >> x.name >> x.type;
        return in;
    }
    friend ostream& operator << (ostream &out, const User &x) {
        out << x.id << " " << x.password << " " << x.name << " " << x.type;
        return out;
    }
};

class UserNotFound {};

class UserStack {
private:
    string pathPrf, pathSuf;
public:
    UserStack(): pathPrf("users_"), pathSuf(".user") {}
    User findUser(const string &id) {
        StringHasher hashStr;
        const string path = pathPrf + hashStr.toString(id) + pathSuf;

        FileChecker checkFile;
        if(!checkFile(path)) throw UserNotFound();

        ifstream fin(path.c_str());
        User t;
        while(fin >> t) if(t.id == id) return t;

        throw UserNotFound();
    }
    void addUser(const User &u) {
        StringHasher hashStr;
        const string path = pathPrf + hashStr.toString(u.id) + pathSuf;
        ofstream fout(path.c_str(), ios::app);
        fout << u << endl;
    }
    void deleteUser(const string &id) {
        StringHasher hashStr;
        const string path = pathPrf + hashStr.toString(id) + pathSuf;

        FileChecker checkFile;
        if(!checkFile(path)) throw UserNotFound();

        vector<User> nw;
        ifstream fin(path.c_str());
        User tp;
        bool flag = 1;
        while(fin >> tp) {
            if(tp.id != id) nw.push_back(tp);
            else flag = 0;
        }
        if(flag) throw UserNotFound();

        remove(path.c_str());
        ofstream fout(path.c_str());
        for(auto t: nw) fout << t << endl;
    }
};

class FinanceLog {
private:
    double incomeSum, outcomeSum;
    fstream file;
    string fileName;
    void createFile(const string &x) {
        ofstream fout(x.c_str());
        fout.close();
    }
public:
    FinanceLog():fileName("finance.bin") {
        FileChecker checkFile;
        if(!checkFile(fileName)) createFile(fileName);
        file.open(fileName.c_str(), ios::ate | ios::binary | ios::in | ios::out);
        int location = file.tellp();
        if(!location) { // new file
            file.seekp(0);
            incomeSum = outcomeSum = 0;
            file.write(reinterpret_cast<char*> (&incomeSum), 8);
            file.write(reinterpret_cast<char*> (&outcomeSum), 8);
        } else {
            file.seekp(location - 16);
            file.read(reinterpret_cast<char*> (&incomeSum), 8);
            file.read(reinterpret_cast<char*> (&outcomeSum), 8);
        }
    }
    void addIncome(double x) {
        debug << "add income " << x << endl;
        int location = file.tellp();
        file.seekp(location - 16);
        // debug << "seeked p = " << file.tellp() << endl;
        incomeSum += x;
        file.write(reinterpret_cast<char*> (&x), 8);
        file.write(reinterpret_cast<char*> (&incomeSum), 8);
        file.write(reinterpret_cast<char*> (&outcomeSum), 8);
        // debug << "added tp = " << file.tellp() << endl;
    }
    void addOutcome(double x) {
        debug << "add outcome " << x << endl;
        int location = file.tellp();
        file.seekp(location - 16);
        // debug << "seeked p = " << file.tellp() << endl;
        outcomeSum += x, x = -x;
        file.write(reinterpret_cast<char*> (&x), 8);
        file.write(reinterpret_cast<char*> (&incomeSum), 8);
        file.write(reinterpret_cast<char*> (&outcomeSum), 8);
        // debug << "added tp = " << file.tellp() << endl;
    }
    void showAll() {
        cout << setprecision(2) << fixed << "+ " << incomeSum << " - " << outcomeSum << endl;
    }
    void showTime(int t) {
        int location = file.tellp();
        debug << "location = " << location << "t = " << t << endl;
        if(location < (t + 2) * 8) {
            cout << "Invalid" << endl;
            return;
        }
        file.seekg(location - (t + 2) * 8);
        double is = 0, os = 0, temp;
        for(int i = 1; i <= t; i++) {
            file.read(reinterpret_cast<char*> (&temp), 8);
            if(temp < 0) os -= temp;
            else is += temp;
        }
        cout << setprecision(2) << fixed << "+ " << is << " - " << os << endl;
        file.seekp(location); // move pointer back.
    }
};

class BookStore {
private:
    BookPool bp;
    DatabaseCore db[4];
    UserStack us;
    FinanceLog fl;

    User curUser;
    Book curBook;
    bool nullBook, curExist;

    stack<User> suStack;


    void init() {
        const string path = "inited.txt";

        FileChecker fileChecker;
        if(fileChecker(path)) return;

        ofstream fout(path.c_str());
        fout << "1" << endl;

        us.addUser((User){"root", "sjtu", "root", 7});
    }

    void invalid() {
        cout << "Invalid" << endl;
    }
    void su(const string &id, const string &password) {
        // debug << "id = " << id << "pswd = " << password << endl;
        User tar;
        try {
            tar = us.findUser(id);
        } catch (UserNotFound) {
            return invalid();
        }
        if(password == "") {
            if(curUser.type == 7) suStack.push(tar), curUser = tar;
            else return invalid();
        } else {
            if(tar.password == password) suStack.push(tar), curUser = tar;
            // else if(curUser.id > tar.id) curUser = tar; //?!! todo
            else return invalid();
        }
    }
    void logOut() {
        if(!curUser.type) return invalid();
        curUser = suStack.top(), suStack.pop();
    }
    void addUser(const string &id, const string &password, const string &name, int type) {
        // debug << "in adduser id = " << id << "type = " << type << "cur = " << curUser.type << endl;
        if(type != -1 && curUser.type < 3) return invalid();
        if(type >= curUser.type) return invalid();
        if(type == -1) type = 1;
        User same;
        bool flag = 1;
        try {
            same = us.findUser(id);
        } catch (UserNotFound) {
            flag = 0;
            us.addUser((User){id, password, name, type});
        }
        if(flag) return invalid();
    }
    void deleteUser(const string &id) {
        if(curUser.type < 7) return invalid();
        try {
            us.deleteUser(id);
        } catch (UserNotFound) {
            return invalid();
        }
    }
    void passwdUser(const string &id, const string &old, string nw) {
        if(curUser.type < 1) return invalid();
        User tar;
        try {
            tar = us.findUser(id);
        } catch (UserNotFound) {
            return invalid();
        }
        if(curUser.type == 7) {
            if(nw == "") nw = old;
        } else {
            if(old != tar.password) return invalid();
        }
        tar.password = nw;
        us.deleteUser(id);
        us.addUser(tar);
    }
    vector<string> splitStr(const string &x) {
        vector<string> ret;
        unsigned i = 0, cur = 0;
        for(i = 0, cur = 0; i < x.length(); i++) if(x[i] == '|') {
            // debug << "cur = " << cur << "i = " << i << endl;
            string t = x.substr(cur, i - cur);
            ret.push_back(t), cur = i + 1;
        }
        ret.push_back(x.substr(cur, x.length() - cur + 1));
        return ret;
    }
    void selectBook(const string &isbn) { // if !curExist, there is no need to delete before modify.
        if(curUser.type < 3) return invalid();
        nullBook = 0, curExist = 1;
        try {
            curBook = bp.getBook(isbn);
        } catch (BookNotFound) {
            // debug << "in select book not found" << endl;
            curExist = 0;
            curBook = (Book){isbn, "", "", "", "", 0};
        }
    }
    void modifyBook(Book tar) {
        if(nullBook) return invalid();
        if(curUser.type < 3) return invalid();
        // debug << "in modify tar = " << tar << endl;
        if(tar.data[ISBN] != "") {
            bool flag = 1;
            try {
                bp.getBook(tar.data[ISBN]);
            } catch(BookNotFound) {
                flag = 0;
            }
            if(flag) return invalid();
        }
        if(curExist) {
            tar.rem = curBook.rem;
            for(int i = 0; i < 3; i++) db[i].removeBook(curBook.data[i], curBook.data[ISBN]);
            vector<string> keywords = splitStr(curBook.data[KEYWORD]);
            for(auto t: keywords) db[KEYWORD].removeBook(t, curBook.data[ISBN]);
            bp.removeBook(curBook.data[ISBN]);
        }
        curExist = 1;
        for(int i = 0; i < 5; i++) if(tar.data[i] != "") curBook.data[i] = tar.data[i];
        for(int i = 0; i < 3; i++) db[i].addBook(curBook);
        // debug << "new keyword = " << curBook.data[KEYWORD] << endl;
        // debug << "splitting by |" << endl;
        vector<string> keywords = splitStr(curBook.data[KEYWORD]);
        // debug << "split finished" << endl;
        for(auto t: keywords) db[KEYWORD].addBook(curBook, t);
        bp.addBook(curBook);
    }
    void importBook(const int v, const string cost) {
        if(nullBook) return invalid();
        if(curUser.type < 3) return invalid();
        bp.removeBook(curBook.data[ISBN]);
        curBook.rem += v;
        bp.addBook(curBook);

        fl.addOutcome(toDouble(cost));
    }
    double toDouble(const string &str) {
        double ret = 0, mul = 0.1;
        unsigned i = 0;
        for(; i < str.length() && str[i] != '.' ; i++) ret = ret * 10 + str[i] - '0';
        for(i++; i < str.length(); i++, mul *= 0.1) ret += (str[i] - '0') * mul;
        return ret;
    }
    void buyBook(const string &isbn, const int v) {
        if(curUser.type < 1) return invalid();
        Book t;
        try {
            t = bp.getBook(isbn);
        } catch (BookNotFound) {
            return invalid();
        }
        if(t.rem < v) return invalid();
        bp.removeBook(t.data[ISBN]);
        t.rem -= v, bp.addBook(t);

        fl.addIncome(toDouble(t.data[PRICE]) * v);
    }
    void showBook(const int argType, const string &arg) {
        // debug << "type = " << curUser.type << endl;
        if(curUser.type < 1) return invalid();
        if(argType == -1 && curUser.type < 1) return invalid(); // !? todo
        // debug << "in sb arg = " << argType << endl;
        // debug << "calling db.sb" << endl;
        vector<Book> ans = argType != -1 ? db[argType].searchBook(arg, bp) : db[ISBN].showAll(bp);
        sort(ans.begin(), ans.end());
        for(auto t: ans) {
            for(int i = 0; i < 4; i++) cout << t.data[i] << '\t';
            cout << setprecision(2) << fixed << toDouble(t.data[4]) << '\t';
            cout << t.rem << "本" << endl; // todo: encoding.
        }
        // if(!ans.size()) cout << endl;
    }
    void showFinance(int arg) {
        if(curUser.type != 7) return invalid();
        if(!~arg) fl.showAll();
        else fl.showTime(arg);
    }

    class getTargetFailed {};
    Book getTarget(const string &s) {
        Book ret = (Book){"", "", "", "", "", 0};
        string str;
        unsigned i = 0, j = 0;
        while(i < s.length() && s[i] == ' ') i++;
        while(i < s.length() && s[i] != ' ') i++;
        // debug << "initial i = " << i << endl;
        for(i++; i < s.length(); i = j + 1) { // !? what if the same argument appears twice ? todo
            str = s.substr(i, s.length() - i);
            // debug << "first str = " << str << endl;
            if(str.substr(0, 6) == "-ISBN=") {
                for(j = i; j < s.length() && s[j] != ' '; j++) ;

                string s2 = str.substr(6, j - i - 6);
                if(ret.data[ISBN] != "") throw getTargetFailed();
                ret.data[ISBN] = s2;
            } else if(str.substr(0, 7) == "-name=\"") {
                for(j = i; j < s.length() && s[j] != '\"'; j++) ;
                j++;
                for(; j < s.length() && s[j] != '\"'; j++) ;
                if(j >= s.length()) throw getTargetFailed();

                string s2 = str.substr(7, j - i - 7);
                if(ret.data[NAME] != "") throw getTargetFailed();
                ret.data[NAME] = s2;
                j++;
            } else if(str.substr(0, 9) == "-author=\"") {
                for(j = i; j < s.length() && s[j] != '\"'; j++) ;
                j++;
                for(; j < s.length() && s[j] != '\"'; j++) ;
                if(j >= s.length()) throw getTargetFailed();

                string s2 = str.substr(9, j - i - 9);
                if(ret.data[AUTHOR] != "") throw getTargetFailed();
                ret.data[AUTHOR] = s2;
                j++;
            } else if(str.substr(0, 10) == "-keyword=\"") {
                for(j = i; j < s.length() && s[j] != '\"'; j++) ;
                j++;
                for(; j < s.length() && s[j] != '\"'; j++) ;
                if(j >= s.length()) throw getTargetFailed();

                string s2 = str.substr(10, j - i - 10);
                if(ret.data[KEYWORD] != "") throw getTargetFailed();
                ret.data[KEYWORD] = s2;
                j++;
            } else if(str.substr(0, 7) == "-price=") {
                for(j = i; j < s.length() && s[j] != ' '; j++) ;

                string s2 = str.substr(7, j - i - 7);
                if(ret.data[PRICE] != "") throw getTargetFailed();
                ret.data[PRICE] = s2;
            } else throw getTargetFailed();
        }
        return ret;
    }
    void reduce(string &s) {
        while(s.length() && isspace(s[s.length() - 1])) s.resize(s.length() - 1);
    }
    void work() {
        Input in;
        FileChecker checkFile;
        if(checkFile("command.txt")) in.hook("command.txt");
        curUser.type = 0;
        suStack.push(curUser);
        curUser.type = 7;

        string line;
        while(in.getLine(line)) {
            debug << line << endl;
            reduce(line);
            stringstream ss;
            ss << line;
            string com;
            ss >> com;
            // ?! todo:: load
#define checkSS() if(!ss.eof()) {invalid(); continue;}
            if(com == "su") {
                string uid, pswd;
                ss >> uid >> pswd;
                if(uid == "") {invalid(); continue;}
                checkSS();
                su(uid, pswd);
            } else if(com == "logout") {
                checkSS();
                logOut();
            } else if(com == "useradd") {
                string id, pswd, name;
                int type = -1;
                ss >> id >> pswd >> type >> name;
                if(id == "" || pswd == "" || type == -1 || name == "") {invalid(); continue;}
                checkSS();
                addUser(id, pswd, name, type);
            } else if(com == "register") {
                string id, pswd, name;
                ss >> id >> pswd >> name;
                if(id == "" || pswd == "" || name == "" || !ss.eof()) {invalid(); continue;}
                addUser(id, pswd, name, -1);
            } else if(com == "delete") {
                string id;
                ss >> id;
                if(id == "") continue;
                checkSS();
                deleteUser(id);
            } else if(com == "passwd") {
                string id, old, nw = "";
                ss >> id >> old >> nw;
                if(id == "" || old == "") {invalid(); continue;}
                checkSS();
                passwdUser(id, old, nw);
            } else if(com == "select") {
                string isbn;
                ss >> isbn;
                if(isbn == "") {invalid(); continue;}
                checkSS();
                selectBook(isbn);
            } else if(com == "modify") { // todo: getTarget invalid.
                Book nw;
                try {
                    nw = getTarget(line);
                } catch (getTargetFailed) {
                    invalid(); continue;
                }
                debug << "nw = " << nw << endl;
                modifyBook(nw);
            } else if(com == "import") {
                int quantity = -1;
                string cost;
                ss >> quantity >> cost;
                if(quantity == -1 || cost == "") {invalid(); continue;}
                checkSS();
                importBook(quantity, cost);
            } else if(com == "show") {
                string test;
                ss >> test;
                if(test == "finance") {
                    int time = -1;
                    ss >> time;
                    checkSS();
                    showFinance(time);
                } else {
                    Book tar;
                    try {
                        tar = getTarget(line);
                    } catch (getTargetFailed) {
                        invalid(); continue;
                    }
                    // debug << "target = " << tar << endl;
                    int tag = 0;
                    for(int i = 0; i < 4; i++) if(tar.data[i] != "")  tag |= (1 << i);
                    if(tag != (tag & -tag)) {invalid(); continue;} // multiple argument.
                    bool flag = 0;
                    for(int i = 0; i < 4; i++) if(tar.data[i] != "") {
                        showBook(i, tar.data[i]);
                        flag = 1;
                    }
                    if(!flag) showBook(-1, "");
                }
            } else if(com == "buy") {
                string isbn;
                int v = -1;
                ss >> isbn >> v;
                if(isbn == "" || v == -1) {invalid(); continue;}
                checkSS();
                buyBook(isbn, v);
            } else if(com == "exit") {
                checkSS();
                return;
            }
            else invalid();
            debug << "==================================================" << endl;
        }
    }
public:
    BookStore() {
        db[0].init(ISBN, "db_isbn_");
        db[1].init(NAME, "db_name_");
        db[2].init(AUTHOR, "db_author_");
        db[3].init(KEYWORD, "db_keyword_");
        nullBook = 1;
        init();
        work();
    }
};


#endif //BOOKSTORE_BASE_HPP
