#include<iostream>
using namespace std;

// interface or abstract class
class FlyBehaviour{
    public:
    virtual void fly() = 0;
    FlyBehaviour(){
        cout << "FlyBehaviour constructor" << endl;
    }
    virtual ~FlyBehaviour(){
        cout << "FlyBehaviour destructor" << endl;
    }
};

class FlyWithWings: public FlyBehaviour{
    public:
    void fly() override{
        cout << "Flying with wings" << endl;
    }
};

class FlyNoWay: public FlyBehaviour{
    public:
    void fly() override{
        cout << "Flying no way" << endl;
    }
};

// interface or abstract class
class QuackBehaviour{
    public:
    virtual void quack() = 0;
    QuackBehaviour(){
        cout << "QuackBehaviour constructor" << endl;
    }
    virtual ~QuackBehaviour(){
        cout << "QuackBehaviour destructor" << endl;
    }
};

class Quack: public QuackBehaviour{
    public:
    void quack() override{
        cout << "Quacking" << endl;
    }
};

// concrete classes
class Squeak: public QuackBehaviour{
    public:
    void quack() override{
        cout << "Squeaking" << endl;
    }
};


// concrete classes
class MuteQuack: public QuackBehaviour{
    public:
    void quack() override{
        cout << "Mute quacking" << endl;
    }
};


class Duck{
    protected:

    FlyBehaviour* flyBehaviour;
    QuackBehaviour* quackBehaviour;
    string duckName;
    public:

    virtual ~Duck(){
        cout << "Duck destructor" << endl;
        delete flyBehaviour;
        delete quackBehaviour;
    }

    string getDuckName(){
        return duckName;
    }

    void setDuckName(string duckName){
        this->duckName = duckName;
    }

    void setFlyBehaviour(FlyBehaviour* flyBehaviour){
        this->flyBehaviour = flyBehaviour;
    }
    void setQuackBehaviour(QuackBehaviour* quackBehaviour){
        this->quackBehaviour = quackBehaviour;
    }

    void performFly(){
        cout << duckName << " fly action: ";
        flyBehaviour->fly();
        cout << endl;
    }

    void performQuack(){
        cout << duckName << " quack action: ";
        quackBehaviour->quack();
        cout << endl;
    }

    void swim(){
        cout << "Swimming" << endl;
    }

    void display(){
        cout << "Displaying" << endl;
    }
};

class MallardDuck: public Duck{
    public:
    MallardDuck(string duckName){
        this->duckName = duckName;
        flyBehaviour = new FlyWithWings();
        quackBehaviour = new Quack();
    }
    ~MallardDuck(){
        cout << "MallardDuck destructor" << endl;
    }
};

class RubberDuck: public Duck{
    public:
    RubberDuck(string duckName){
        this->duckName = duckName;
        flyBehaviour = new FlyNoWay();
        quackBehaviour = new Squeak();
    }
    ~RubberDuck(){
        cout << "RubberDuck destructor" << endl;
    }
};

class DecoyDuck: public Duck{
    public:
    DecoyDuck(string duckName){
        this->duckName = duckName;
        flyBehaviour = new FlyNoWay();
        quackBehaviour = new MuteQuack();
    }
    ~DecoyDuck(){
        cout << "DecoyDuck destructor" << endl;
    }
};

int main(){
    Duck* mallardDuck = new MallardDuck("Mallard Duck");
    Duck* rubberDuck = new RubberDuck("Rubber Duck");
    Duck* decoyDuck = new DecoyDuck("Decoy Duck");

    mallardDuck->performFly();
    mallardDuck->performQuack();

    rubberDuck->performFly();
    rubberDuck->performQuack();

    decoyDuck->performFly();
    decoyDuck->performQuack();


    rubberDuck->setFlyBehaviour(new FlyWithWings());
    rubberDuck->performFly();

    delete mallardDuck;
    delete rubberDuck;
    delete decoyDuck;
    return 0;
}