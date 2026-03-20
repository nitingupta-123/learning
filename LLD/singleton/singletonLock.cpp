#include<iostream>
using namespace std;
#include <mutex>

class Singleton{
    private:
    static Singleton* instance;
    // Approach 1: static member mutex (define in .cpp) - risk of static init order fiasco

    Singleton(){
        cout<<"Singleton constructor"<<endl;
    }

    public:
    static Singleton* getInstance(){
        
        if(instance == nullptr){
            // Approach 2 (better): function-local static - safe init, no static order fiasco
            static std::mutex mutex;
            std::lock_guard<std::mutex> lock(mutex);
            if(instance == nullptr){
                instance = new Singleton();
            }
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

Singleton* Singleton::instance = nullptr;

int main(){
    Singleton* s = Singleton::getInstance();
    s->doSomething();
    Singleton* s2 = Singleton::getInstance();
    s2->doSomething();
    return 0;
}