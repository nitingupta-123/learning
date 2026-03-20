#include<iostream>
using namespace std;

string caller = "";

class ICharacter{
    public:
    virtual string getAbilities() = 0;
    virtual ~ICharacter(){
        cout <<caller<< " Inside ICharacter destructor." << endl;
        caller = " ";
    }
};

class Mario: public ICharacter{
    public:
    string getAbilities() override{
        return "Mario can jump and run";
    }
};


class ICharacterDecorator: public ICharacter{
    public:
    ICharacter* character;
    ICharacterDecorator(ICharacter* character){
        this->character = character;
    }
    virtual ~ICharacterDecorator(){
        cout <<caller<< " Inside ICharacterDecorator destructor." << endl;
        delete character;
        cout << "Calling ICharacter destructor from ICharacterDecorator destructor." << endl;
        caller = "Came from ICharacterDecorator destructor";
    }
    virtual string getAbilities() = 0;
};


class HeightPowerUpDecorator: public ICharacterDecorator{

    public:

    HeightPowerUpDecorator(ICharacter* character): ICharacterDecorator(character){}
   
    string getAbilities() override{
        return character->getAbilities() + " " + "with height power up";
    }

    virtual ~HeightPowerUpDecorator(){
        cout << "Inside HeightPowerUpDecorator destructor." << endl;
        caller = "Came from HeightPowerUpDecorator destructor";
    }

};



class GunPowerUpDecorator: public ICharacterDecorator  {

    public:

    GunPowerUpDecorator(ICharacter* character): ICharacterDecorator(character){}

    string getAbilities() override{
        return character->getAbilities() + " " + "with Gun power up";
    }

    virtual ~GunPowerUpDecorator(){
        cout << "Inside GunPowerUpDecorator destructor." << endl;
        caller = "Came from GunPowerUpDecorator destructor";

    }

};


int main(){


    ICharacter* mario = new HeightPowerUpDecorator(new GunPowerUpDecorator(new Mario()));
    cout<< mario->getAbilities()<<endl;

    delete mario;
    return 0;

}
