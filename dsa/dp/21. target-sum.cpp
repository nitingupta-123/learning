//Back-end complete function Template for C++

/*
https://www.youtube.com/watch?v=b3GD8263-PQ&list=PLgUwDviBIf0qUlt5H_kiKYaNSqJ81PMMY&index=23

https://www.geeksforgeeks.org/problems/target-sum-1626326450/1


*/
class Solution {
  public:
  
 /* 
    s1-(sum-s1) = target
    
2s1 - sum = target


s1 = (target+ sum)/2

*/
const int MOD = 1e9 + 7;

long long fn(int i, vector<int>& A, int target, int offset, vector<vector<int>>& dp) {
    if (i == 0) {
        int cnt = 0;
        if (A[0] == target) cnt++;
        if (-A[0] == target) cnt++;
        // If A[0] == 0 and target == 0, this counts as two ways: +0 and -0
        return cnt;
    }

    if (target + offset < 0 || target + offset >= dp[0].size()) {
        return 0; // Out of bounds
    }

    if (dp[i][target + offset] != -1) {
        return dp[i][target + offset];
    }

    long long plusWays = fn(i - 1, A, target - A[i], offset, dp);
    long long minusWays = fn(i - 1, A, target + A[i], offset, dp);

    return dp[i][target + offset] = (plusWays + minusWays) % MOD;
}

long long findTargetSumWays(int n, vector<int>& A, int target) {
    int offset = 2000;
    int totalSum = accumulate(A.begin(), A.end(), 0);

    int maxRange = 4000;

    vector<vector<int>> dp(n, vector<int>(maxRange, -1));
    return fn(n - 1, A, target, offset, dp);
}

    
    
    
    /*
    long long fn(int i, vector<int>& A, int target,int offset,vector<vector<int>>&dp){
        
        
        if(i==0){
            if(A[i]==target){
                return 1;
            }
            
            if(target==0){
                return 1;
            }
            
            return 0;
        }
        
        if(dp[i][offset+target]!=-1){
            return dp[i][offset + target];
        }
        
        int pick = 0;
        
        if(target>=A[i]){
            pick = fn(i-1,A,target-A[i],offset,dp);
        }
        
        int notPick = fn(i-1,A,target,offset,dp);
        
        return dp[i][offset + target]= (pick + notPick)*1LL;
    }
    

    long long findTargetSumWays(int n, vector<int>& A, int target) {
        // Your code here
        // int n = A.size();
        int sum = accumulate(A.begin(), A.end(), 0);

        // If target is not reachable
        if (abs(target) > sum) return 0;

        int offset = sum;
        vector<vector<int>> dp(n, vector<int>(2 * sum + 1, -1));
        
        return fn(n-1,A,(target+sum)/2,offset,dp);
    }
    */
    
};