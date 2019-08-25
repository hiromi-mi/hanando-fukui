template <typename Elem> class Vector {
   public:
   Elem **data;
   long capacity;
   long len;
   //Elem get(long id);
};

// does not work yet!
/*
template <typename Elem> long Array<Elem>::GetSize( )
    { return this->len; }
    */

int intfunc() {
   Vector<int> a;
   a.len = 0;
   a.capacity = 16;
   a.data = malloc(sizeof(int) * a.capacity);
}

int charfunc() {
   Vector<char> a;
   a.len = 0;
   a.capacity = 16;
   a.data = malloc(sizeof(char) * a.capacity);
}

int main() {
   intfunc();
   charfunc();
   return 0;
}
