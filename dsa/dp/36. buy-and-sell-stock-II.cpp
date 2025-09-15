class Solution {
public:
    // Memoization
    int fn(int i, vector<int>& prices, int canBuy,vector<vector<int>>&dp) {


        int n = prices.size();

        if(i==n){
            return 0;
        }

        if(dp[i][canBuy]!=-1){
            return dp[i][canBuy];
        }

        int profit =0;
        if(canBuy){//1
            profit =  max(fn(i+1,prices,1,dp), -prices[i]+fn(i+1,prices,0,dp));
        }else{//0
            profit =  max(fn(i+1,prices,0,dp), prices[i]+fn(i+1,prices,1,dp));
        }

        return dp[i][canBuy] = profit;
    }

/*    
    int maxProfit(vector<int>& prices) {
        int n = prices.size();
        vector<vector<int>>dp(n+1,vector<int>(2,-1));
        return fn(0, prices, 1,dp);
    }
*/

    // Tabulation
    int maxProfit(vector<int>& prices) {
        int n = prices.size();
        // vector<vector<int>>dp(n+1,vector<int>(2,-1));
        vector<int>fwd(2,0);
        vector<int>curr(2,0);
        for(int i=n;i>=0;i--){
            for(int j=0;j<2;j++){
                if(i==n){
                    fwd[j] = 0;
                }
                else{
                    int profit =0;
                    if(j){//1
                        profit =  max(fwd[1], -prices[i] + fwd[0] );
                    }else{//0
                        profit =  max(fwd[0], prices[i]+ fwd[1]);
                    }

                    curr[j] = profit;

                }
            }

            fwd = curr;
        }

        return fwd[1];
    }
    

    // int maxProfit(vector<int>& prices) {
    //     int n = prices.size();
    //     vector<vector<int>>dp(n+1,vector<int>(n+1,0));


    // }

/*

    SC && TC => O(n^2)

    int fn(int i, vector<int>& prices, int lastStock) {

        int n = prices.size();

        if (i >= n) return 0;

        int buy = 0;
        if (lastStock == -1) {
            buy = fn(i + 1, prices, i);
        }

        int sell = 0;
        if (lastStock >= 0) {
            sell = (prices[i] - prices[lastStock]) + fn(i + 1, prices, -1);
        }

        int sellAndBuy = 0;
        if (lastStock >= 0) {
            sellAndBuy = (prices[i] - prices[lastStock]) + fn(i + 1, prices, i);
        }

        int omit = fn(i + 1, prices, lastStock);

        return max({sell, buy, sellAndBuy, omit});
    }

    int maxProfit(vector<int>& prices) {
        int n = prices.size();
        vector<vector<int>>dp(n+1,vector<int>(n+1,-1));
        return fn(0, prices, -1);
    }

*/
};
