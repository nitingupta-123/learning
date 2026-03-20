#include<iostream>
#include<map>
using namespace std;

class ICommand{

    public:
    virtual void execute() =0;
    virtual void undo() = 0;
};


class Light{

    public:

    void on(){
        cout<<"on"<<endl;
    }

    void off(){
        cout<<"off"<<endl;
    }
};

class LightCommand: public ICommand{

    private:

    Light *t;

    public:

    LightCommand(Light *t){
        this->t = t;
    }

    void execute(){
        t->on();
    }

    void undo(){
        t->off();
    }
};


class Remote{

    private:
        map<string,ICommand*>cmd;
        int numBtns;
    
    public:
    
    Remote(int numBtns){
        this->numBtns = numBtns;
    }

    void setCommand(map<string,ICommand*>cmd){
        if(cmd.size()>numBtns){
            cout<<"invalid operation"<<endl;
            return;
        }
        this->cmd = cmd;
    }

    void pressButton(string btnFunction){
        
        cmd[btnFunction]->execute();
    }



};


int main(){

    
    ICommand* lightCmd = new LightCommand(new Light());
    Remote* r = new Remote(1);

    map<string,ICommand*> remoteFuncMap;
    remoteFuncMap["on"] = lightCmd;

    r->setCommand(remoteFuncMap);


    r->pressButton("on");

}
