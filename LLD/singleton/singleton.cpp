/*
singleton pattern and eager initialization

singleton pattern is a design pattern that ensures a class has only one instance and provides a global point of access to it.
eager initialization is a technique of initializing the singleton instance at the time of class loading.

advantages:
1. ensures a single instance of the class
2. provides a global point of access to it
3. ensures thread safety
4. ensures lazy initialization? what is this? 
lazy initialization is a technique of initializing the singleton instance when it is needed.


*/

#include<iostream>
using namespace std;

class Singleton{
    private:
    static Singleton* instance;
    Singleton(){
        cout<<"Singleton constructor"<<endl;
    }
    public:
    static Singleton* getInstance(){
        if(instance == nullptr){
            instance = new Singleton();
        }
        return instance;
    }
    ~Singleton(){
        cout<<"Singleton destructor"<<endl;
    }

    void doSomething(){
        cout<<"Doing something"<<endl;
    }

};
Singleton* Singleton::instance = new Singleton();


int main(){
    Singleton* s = Singleton::getInstance();
    s->doSomething();
    return 0;
}