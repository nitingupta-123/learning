class Solution {
public:


    int MOD = 1e9+7;


    int numDistinct(string t, string s) {

        int n = s.size();
        int m = t.size();
        // vector<vector<int>>dp(n,vector<int>(m,0));

        vector<int>curr(m,0),prev(m,0);


        prev[0] = s[0] == t[0];
        for(int j=1;j<m;j++){
            if(s[0]==t[j]){
                prev[j]=prev[j-1] + 1;
            }else{
                prev[j]+= prev[j-1];
            }
            
        }
        for(int i=1;i<n;i++){
            for(int j=1;j<m;j++){
                
                if(s[i]==t[j]){
                    curr[j] = ( (1LL * prev[j-1]) + curr[j-1] ) % MOD;
                }
                else{
                    curr[j] = curr[j-1];
                }
                
            }

            prev = curr;
        }

        
        return prev[m-1];
    }

    int fn(int i,int j, string &t, string &s,vector<vector<int>>&dp){

        if(i<0){
            return 1;
        }

        if(j<0){
            return 0;
        }

        if(dp[i][j]!=-1){
            return dp[i][j];
        }
        
        
        if(t[i]==s[j]){

            return dp[i][j]=fn(i-1,j-1,t,s,dp) + fn(i,j-1,t,s,dp);

        }else{
            return dp[i][j]=fn(i,j-1,t,s,dp);
        }


    }

    
};