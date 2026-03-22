// ─────────────────────────────────────────────
// ADAPTER PATTERN — Third-party API Integration
// miniOrange use case | Nitin Gupta | Interview Prep
// ─────────────────────────────────────────────


// ── 1. Target Interface (what YOUR system expects) ─────────────────────────
interface ApiResponse {
  msg: string;
  status: number;
  data: Record<string, unknown>;
}


// ── 2. Adaptee — Moralis (incompatible response format) ───────────────────
interface MoralisResponse {
  message: string;       // different key name
  httpStatus: number;    // different key name
  result: unknown;       // different key name
}

class MoralisService {
  fetchApiData(): MoralisResponse {
    // Simulating Moralis API response
    return {
      message: "Data fetched successfully!",
      httpStatus: 200,
      result: { tokenId: "123", owner: "0xABC" },
    };
  }
}


// ── 3. Another Adaptee — AlchemyAPI (different format again) ──────────────
interface AlchemyResponse {
  statusCode: number;    // yet another key name
  body: string;
  payload: unknown;
}

class AlchemyService {
  fetchData(): AlchemyResponse {
    return {
      statusCode: 200,
      body: "Alchemy data fetched!",
      payload: { contract: "0xDEF", balance: "5.2 ETH" },
    };
  }
}


// ── 4. Target interface the Adapters must implement ────────────────────────
interface BlockchainApiAdapter {
  getApiResponse(): ApiResponse;
}


// ── 5. MoralisAdapter — translates Moralis → standard format ──────────────
class MoralisAdapter implements BlockchainApiAdapter {
  private service: MoralisService;

  // DIP: dependency injected, not created inside
  constructor(service: MoralisService) {
    this.service = service;
  }

  getApiResponse(): ApiResponse {
    const raw = this.service.fetchApiData(); // call adaptee

    // Translate incompatible format → standard format
    return {
      msg:    raw.message,
      status: raw.httpStatus,
      data:   raw.result as Record<string, unknown>,
    };
  }
}


// ── 6. AlchemyAdapter — translates Alchemy → same standard format ──────────
class AlchemyAdapter implements BlockchainApiAdapter {
  private service: AlchemyService;

  constructor(service: AlchemyService) {
    this.service = service;
  }

  getApiResponse(): ApiResponse {
    const raw = this.service.fetchData(); // call adaptee

    // Translate incompatible format → standard format
    return {
      msg:    raw.body,
      status: raw.statusCode,
      data:   raw.payload as Record<string, unknown>,
    };
  }
}


// ── 7. Client — knows NOTHING about Moralis or Alchemy ────────────────────
class TokenGatingService {
  private adapter: BlockchainApiAdapter;

  // DIP: depends on interface, not concrete adapter
  constructor(adapter: BlockchainApiAdapter) {
    this.adapter = adapter;
  }

  checkAccess(): void {
    const response = this.adapter.getApiResponse(); // standard format always

    if (response.status === 200) {
      console.log("Access granted:", response.msg);
      console.log("Data:", response.data);
    } else {
      console.log("Access denied");
    }
  }
}


// ── 8. Usage ───────────────────────────────────────────────────────────────

// Using Moralis — client has no idea
const moralisAdapter = new MoralisAdapter(new MoralisService());
const service1 = new TokenGatingService(moralisAdapter);
service1.checkAccess();
// → Access granted: Data fetched successfully!
// → Data: { tokenId: '123', owner: '0xABC' }

// Swap to Alchemy — zero client code changes
const alchemyAdapter = new AlchemyAdapter(new AlchemyService());
const service2 = new TokenGatingService(alchemyAdapter);
service2.checkAccess();
// → Access granted: Alchemy data fetched!
// → Data: { contract: '0xDEF', balance: '5.2 ETH' }