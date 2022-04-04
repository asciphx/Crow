using namespace std;
class User;
class List;
Class(User)
bool is; int age; double weight;
string name; bool state;
FUCKJSON(User, is, age, weight, name, state)

Class(List)
User* user = nullptr;
vector<User> userList;
~List() { user = nullptr; }
FUCKJSON(List, user, userList)