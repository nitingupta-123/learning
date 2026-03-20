#include<iostream>
#include<vector>
#include<algorithm>
#include<string>
using namespace std;

class IObservable;

class IObserver{
    public:
    virtual void update(IObservable *ch) = 0;
    virtual ~IObserver(){

    };  // required for correct delete via base pointer
};

class IObservable{
    public:
    virtual void addSubsciber(IObserver *obsrever) = 0;
    virtual void removeSubscriber(IObserver *obsrever) = 0;
    virtual void notifyAll(IObservable *ch) = 0;
    virtual void getVideo() = 0;  // so observers can call it via base pointer
    virtual ~IObservable(){

    };  // required for correct delete via base pointer
};





class Channel: public IObservable{
    private:
    vector<IObserver*>subscribers;
    string channelName;
    public:
    Channel(string channelName){
        this->channelName = channelName;
    }

    public:
    ~Channel(){
        for(auto it = subscribers.begin(); it != subscribers.end(); ++it){
            delete *it;
        }
        subscribers.clear();
    }

    public:
    void addSubsciber (IObserver *subscriber) override{
        if(find(subscribers.begin(),subscribers.end(),subscriber)==subscribers.end()){
            subscribers.push_back(subscriber);
        }
    }


    void removeSubscriber(IObserver *subscriber){
        for(auto it = subscribers.begin(); it != subscribers.end(); ++it){
            if(*it == subscriber){
                subscribers.erase(it);
                break;
            }
        }
    }

    void uploadVideo(Channel &ch){
        cout<<"Video uploaded"<<endl;
        notifyAll(this);
    }

    void notifyAll(IObservable *ch){
        for(auto it=subscribers.begin(); it!=subscribers.end();it++){
            (*it)->update(ch);
        }
    }

    void getVideo() override {
        cout<<"Playing the video"<<endl;
    }

};

class Subscriber: public IObserver{

    private:
    string name;
    string channelName;

    public:

    Subscriber(string name, string channelName){
        this->name = name;
        this->channelName = channelName;
    }

    void update(IObservable *ch){
        ch->getVideo();
    }

};


int main(){

    IObserver *sub1 = new Subscriber("Nitin","mySirG");
    IObserver *sub2 = new Subscriber("Jatin","Namaste Js");

    Channel *ch = new Channel("mySirG");
    ch->addSubsciber(sub1);
    ch->addSubsciber(sub2);

    ch->uploadVideo(*ch);

    delete ch;
    return 0;
}