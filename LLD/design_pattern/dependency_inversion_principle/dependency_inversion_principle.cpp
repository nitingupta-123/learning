#include<iostream>
using namespace std;


class PersistanceDb{

    public:
    virtual void saveToDb() = 0;

    virtual ~PersistanceDb(){

    }
};

class MongoDb: public PersistanceDb{
    public:
    void saveToDb() override{
        cout<<"saving to mongo db..."<<endl;
    }
};


class SqlDb: public PersistanceDb{
    public:
    void saveToDb() override{
        cout<<"saving to sql db..."<<endl;
    }
};


class Service{
    PersistanceDb *db = nullptr;

    public:
    Service(PersistanceDb *db) : db(db) {}

    void setInstance(PersistanceDb *db){
        this->db = db;
    }

    void saveToDB(){
        if(db) db->saveToDb();
    }
};


int main(){
    PersistanceDb* sql = new SqlDb();
    PersistanceDb* mongo = new MongoDb();

    Service* s1 = new Service(sql);
    s1->saveToDB();

    s1->setInstance(mongo);
    s1->saveToDB();

    delete sql;
    delete mongo;
    delete s1;

}




