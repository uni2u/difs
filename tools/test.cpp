// list::front
#include <iostream>
#include <list>
#include <boost/core/noncopyable.hpp>
#include <string>

using namespace std;

class TestClass : boost::noncopyable
{
public:
  string default_name = "John";
void sayHello() {
  std::cout<<"I say Hello to " << default_name << std::endl;
}

};


class MyClass final : public TestClass
{

  public:
  void setDefaultNames(string name) {
    default_name = name;
  }
  // MyClass() {
  //     default_name = string("default");
  // }
  // MyClass(string name) {
  //     default_name = name;
  // }

};

int main ()
{
  std::list<int> mylist;

    mylist.push_back(1);
    mylist.push_back(2);
    mylist.push_back(3);
    mylist.push_back(4);
    mylist.push_back(5);
    mylist.push_back(6);
    mylist.push_back(7);
    mylist.push_back(8);
    mylist.push_back(9);
    mylist.push_back(0);

  // now front equals 77, and back 22

  mylist.front();

  //std::cout << "mylist.front() is now " << mylist.front() << '\n';
  std::cout << "mylist.front() is now " << mylist.front() << '\n';
  std::cout << "mylist.back() is now " << mylist.back() << '\n';

  MyClass myclass;
  myclass.setDefaultNames(string("justin"));
  myclass.sayHello();

  MyClass *yourclass = &myclass;

  yourclass->default_name = string("brian");
  yourclass->sayHello();

  // TestClass* testclass = new TestClass();
  // TestClass testclass1;

  // testclass1 = testclass;

  // cout<< "testclass1:" << testclass1.default_name << endl;

  return 0;
}