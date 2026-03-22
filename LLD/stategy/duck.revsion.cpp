#include<iostream>
using namespace std;


class FlyBehaviour{
    public:
    virtual void fly() = 0;
    
};

class  NoFly: public FlyBehaviour{
    void fly(){
        cout<<"Duck cannot Fly"<<endl;
    }
};


class  FlyWithWings: public FlyBehaviour{
    void fly(){
        cout<<"Fly with Wings"<<endl;
    }
};



class QuackBehaviour{
    public:
        virtual void quack() = 0;
};

class Quack: QuackBehaviour{
    void quack(){
        cout<<"Quacking Sound"<<endl;
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

    public:
    FlyBehaviour* fb;
    QuackBehaviour* qb;

    void swim(){
        cout<<"I can swim"<<endl;
    }

    void setFlyBehviour(FlyBehaviour *fb){
        this->fb = fb;
    }

    void setQuackBehviour(QuackBehaviour *qb){
        this->qb = qb;
    }

};

class RubberDuck: public Duck{

    RubberDuck(){
        qb = new Squeak();
        fb = new NoFly();
    }


};

class RubberDuck: public Duck{

};









