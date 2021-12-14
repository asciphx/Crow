using namespace std;
struct User {
  bool is; int age; double weight;
  string name; unique_ptr<bool> state;
};
SYMBOL_(User, AS_(is), AS_(age), AS_(weight), AS_(name), AS_(state))
struct List { User user; vector<User> userList; };
SYMBOL_(List, AS_(user), AS_(userList))