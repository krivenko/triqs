#include <string>
#include <iostream>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <triqs/utility/variant.hpp>
#include <triqs/utility/variant_serialize.hpp>

using namespace triqs::utility;
using namespace std;

struct my_struct {
  string name;
  int value;
  int id;

  my_struct(string const& name = "", double value = 0) : name(name), value(value), id(next_id) {
    cout << "I'm constructor of my_struct (instance " << id << ")" << endl;
    next_id++;
  }
  my_struct(my_struct const& other) : name(other.name), value(other.value), id(next_id) {
    cout << "I'm copy-constructor of my_struct (instance " << id << ")" << endl;
    next_id++;
  }
  ~my_struct() {
    cout << "I'm destructor of my_struct (instance " << id << ")" << endl;
  }

  my_struct & operator=(my_struct const& other) {
    cout << "I'm operator=() of my_struct (instance " << id << ")" << endl;
    name = other.name;
    value = other.value;
    return *this;
  }

  bool operator==(my_struct const& other) const {
    return name == other.name && value == other.value;
  }

  bool operator<(my_struct const& other) const {
    return value < other.value;
  }

  friend ostream & operator<<(ostream & os, my_struct const& ms) {
    return os << "{" << ms.name << " => " << ms.value << "}";
  }

  // Boost.Serialization
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & name & value;
  }

  static int next_id;
};
int my_struct::next_id = 0;

struct visitor {
  void operator()(int i) { cout << "int " << i << " is stored" << endl; }
  void operator()(string s) { cout << "string '" << s << "' is stored" << endl; }
  void operator()(my_struct const& ms) { cout << "my_struct " << ms << " is stored" << endl; }
};

int main() {

  using my_variant = variant<int,string,my_struct>;

  my_variant v_int(2);
  my_variant v_string("text");
  my_variant v_my_struct1(my_struct{"g",9});
  //my_variant v_error(make_pair(1,1));

  my_variant v_my_struct2(v_my_struct1);
  my_variant v_my_struct3(my_struct{"x",2});
  v_my_struct1 = v_my_struct3;
  v_my_struct1 = v_string;

  cout << (v_my_struct1 == v_string) << endl;

  cout << "v_int = " << v_int << endl;
  cout << "v_string = " << v_string << endl;
  cout << "v_my_struct1 = " << v_my_struct1 << endl;
  cout << "v_my_struct2 = " << v_my_struct2 << endl;
  cout << "v_my_struct3 = " << v_my_struct3 << endl;

  cout << "v_int: "; apply_visitor(visitor(), v_int);
  cout << "v_string: "; apply_visitor(visitor(), v_string);
  cout << "v_my_struct1: "; apply_visitor(visitor(), v_my_struct1);
  cout << "v_my_struct2: "; apply_visitor(visitor(), v_my_struct2);
  cout << "v_my_struct3: "; apply_visitor(visitor(), v_my_struct3);

  cout << "Test serialization" << endl;
  stringstream ss;
  boost::archive::text_oarchive oa(ss);

  oa << v_int << v_string << v_my_struct1 << v_my_struct2 << v_my_struct3;

  boost::archive::text_iarchive ia(ss);
  my_variant restored_variant(0);
  for(auto i : {0,1,2,3,4}) {
    ia >> restored_variant;
    cout << restored_variant << endl;
  }

  return 0;
}
