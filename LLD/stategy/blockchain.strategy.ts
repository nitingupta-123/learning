// ─────────────────────────────────────────────
// STRATEGY PATTERN — Blockchain Signature Verification
// miniOrange use case | Nitin Gupta | Interview Prep
// ─────────────────────────────────────────────


// ── 1. Strategy Interface ──────────────────────────────────────────────────
interface SignatureAlgorithm {
    sign(walletAddress: string, nonce: string): string;
  }
  
  
  // ── 2. Wallet Interface ────────────────────────────────────────────────────
  interface Wallet {
    getWalletAddress(): string;
    setWalletAddress(address: string): void;
  }
  
  
  // ── 3. Concrete Wallets ────────────────────────────────────────────────────
  class Metamask implements Wallet {
    private walletAddress: string;
  
    constructor(walletAddress: string) {
      this.walletAddress = walletAddress;
    }
  
    getWalletAddress(): string {
      return this.walletAddress;
    }
  
    setWalletAddress(address: string): void {
      this.walletAddress = address;
    }
  }
  
  class PeraWallet implements Wallet {
    private walletAddress: string = "";
  
    getWalletAddress(): string {
      return this.walletAddress;
    }
  
    setWalletAddress(address: string): void {
      this.walletAddress = address;
    }
  }
  
  
  // ── 4. Concrete Strategies ─────────────────────────────────────────────────
  class EthereumSigner implements SignatureAlgorithm {
    sign(walletAddress: string, nonce: string): string {
      return `eth_signed:${walletAddress}:${nonce}`;
    }
  }
  
  class PolygonSigner implements SignatureAlgorithm {
    sign(walletAddress: string, nonce: string): string {
      return `polygon_signed:${walletAddress}:${nonce}`;
    }
  }
  
  class SolanaSigner implements SignatureAlgorithm {
    sign(walletAddress: string, nonce: string): string {
      return `solana_signed:${walletAddress}:${nonce}`;
    }
  }
  
  
  // ── 5. Signer Registry (Map — NOT if-else) ─────────────────────────────────
  // OCP: adding a new chain = one new line here, zero changes elsewhere
  const signerRegistry: Record<string, () => SignatureAlgorithm> = {
    ethereum: () => new EthereumSigner(),
    polygon:  () => new PolygonSigner(),
    solana:   () => new SolanaSigner(),
  };
  
  
  // ── 6. SignerFactory ───────────────────────────────────────────────────────
  class SignerFactory {
    static getStrategy(chain: string): SignatureAlgorithm {
      const factory = signerRegistry[chain];
      if (!factory) throw new Error(`Unsupported chain: ${chain}`);
      return factory();
    }
  
    // OCP: register new chains at runtime without touching existing code
    static register(chain: string, factory: () => SignatureAlgorithm): void {
      signerRegistry[chain] = factory;
    }
  }
  
  
  // ── 7. Context Class ───────────────────────────────────────────────────────
  class BlockchainService {
    private strategy: SignatureAlgorithm;
    private wallet: Wallet;
    private blockchainName: string;
  
    constructor(chain: string, wallet: Wallet) {
      this.blockchainName = chain;
      this.wallet = wallet;
      this.strategy = SignerFactory.getStrategy(chain); // DIP: depends on interface
    }
  
    // Core of Strategy Pattern — swap algorithm at runtime
    setStrategy(chain: string): void {
      this.blockchainName = chain;
      this.strategy = SignerFactory.getStrategy(chain);
    }
  
    verifyAccess(nonce: string): string {
      const address = this.wallet.getWalletAddress();
      return this.strategy.sign(address, nonce);
    }
  
    getChain(): string {
      return this.blockchainName;
    }
  }
  
  
  // ── 8. Usage / Main ────────────────────────────────────────────────────────
  const walletAddress = "0xABC123DEF456";
  const nonce = "secure_nonce_xyz";
  
  const wallet = new Metamask(walletAddress);
  const service = new BlockchainService("ethereum", wallet);
  
  // Ethereum signing
  const sig1 = service.verifyAccess(nonce);
  console.log("Ethereum:", sig1);
  // → eth_signed:0xABC123DEF456:secure_nonce_xyz
  
  // Swap to Polygon at runtime — no class changes, no if-else
  service.setStrategy("polygon");
  const sig2 = service.verifyAccess(nonce);
  console.log("Polygon:", sig2);
  // → polygon_signed:0xABC123DEF456:secure_nonce_xyz
  
  // OCP in action: add new chain — zero existing code modified
  SignerFactory.register("avalanche", () => ({
    sign: (addr, n) => `avalanche_signed:${addr}:${n}`
  }));
  service.setStrategy("avalanche");
  const sig3 = service.verifyAccess(nonce);
  console.log("Avalanche:", sig3);
  // → avalanche_signed:0xABC123DEF456:secure_nonce_xyz