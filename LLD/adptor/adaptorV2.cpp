// ─────────────────────────────────────────────
// ADAPTER PATTERN — Third-party API Integration
// miniOrange use case | Nitin Gupta | Interview Prep
// ─────────────────────────────────────────────

#include <iostream>
#include <string>
#include <map>
using namespace std;


// ── 1. Standard format YOUR system expects ────────────────────────────────
struct ApiResponse {
    string msg;
    int    status;
    string data;
};


// ── 2. Adaptee — Moralis (incompatible format) ────────────────────────────
struct MoralisResponse {
    string message;     // different key
    int    httpStatus;  // different key
    string result;      // different key
};

class MoralisService {
public:
    MoralisResponse fetchApiData() {
        return { "Data fetched from Moralis!", 200, "tokenId:123,owner:0xABC" };
    }
};


// ── 3. Adaptee — Alchemy (yet another format) ─────────────────────────────
struct AlchemyResponse {
    int    statusCode;  // different key
    string body;        // different key
    string payload;     // different key
};

class AlchemyService {
public:
    AlchemyResponse fetchData() {
        return { 200, "Alchemy data fetched!", "contract:0xDEF,balance:5.2ETH" };
    }
};


// ── 4. Target interface all adapters must implement ───────────────────────
class BlockchainApiAdapter {
public:
    virtual ApiResponse getApiResponse() = 0;
    virtual ~BlockchainApiAdapter() {}
};


// ── 5. MoralisAdapter — translates Moralis → standard format ──────────────
class MoralisAdapter : public BlockchainApiAdapter {
private:
    MoralisService& service;  // DIP: injected, not created inside

public:
    MoralisAdapter(MoralisService& svc) : service(svc) {}

    ApiResponse getApiResponse() override {
        MoralisResponse raw = service.fetchApiData();  // call adaptee

        // Translate incompatible → standard
        ApiResponse resp;
        resp.msg    = raw.message;
        resp.status = raw.httpStatus;
        resp.data   = raw.result;
        return resp;
    }
};


// ── 6. AlchemyAdapter — translates Alchemy → same standard format ─────────
class AlchemyAdapter : public BlockchainApiAdapter {
private:
    AlchemyService& service;  // DIP: injected

public:
    AlchemyAdapter(AlchemyService& svc) : service(svc) {}

    ApiResponse getApiResponse() override {
        AlchemyResponse raw = service.fetchData();  // call adaptee

        // Translate incompatible → standard
        ApiResponse resp;
        resp.msg    = raw.body;
        resp.status = raw.statusCode;
        resp.data   = raw.payload;
        return resp;
    }
};


// ── 7. Client — knows NOTHING about Moralis or Alchemy ────────────────────
class TokenGatingService {
private:
    BlockchainApiAdapter& adapter;  // DIP: depends on interface only

public:
    TokenGatingService(BlockchainApiAdapter& adapter) : adapter(adapter) {}

    void checkAccess() {
        ApiResponse response = adapter.getApiResponse();  // standard always

        if (response.status == 200) {
            cout << "Access granted: " << response.msg  << endl;
            cout << "Data: "           << response.data << endl;
        } else {
            cout << "Access denied" << endl;
        }
    }
};


// ── 8. Main ───────────────────────────────────────────────────────────────
int main() {

    // Using Moralis — client has no idea
    MoralisService moralisService;
    MoralisAdapter moralisAdapter(moralisService);
    TokenGatingService service1(moralisAdapter);
    service1.checkAccess();
    // → Access granted: Data fetched from Moralis!
    // → Data: tokenId:123,owner:0xABC

    cout << "---" << endl;

    // Swap to Alchemy — zero client code changes
    AlchemyService alchemyService;
    AlchemyAdapter alchemyAdapter(alchemyService);
    TokenGatingService service2(alchemyAdapter);
    service2.checkAccess();
    // → Access granted: Alchemy data fetched!
    // → Data: contract:0xDEF,balance:5.2ETH

    return 0;
}