class Solution {
  public:
  
    /*
    
    int fn(int i,int j,string &s1, string &s2,vector<vector<int>>&dp){
        
        
        if(i==0){
           for(int k=j;k>=0;k--){
               if(s1[0]==s2[k]){
                   return 1;
               }
           }
           return 0;
        }
        
        if(j==0){
            for(int k=i;k>=0;k--){
               if(s1[k]==s2[0]){
                   return 1;
               }
           }
           return 0;
        }
        
        if(dp[i][j]!=-1){
            return dp[i][j];
        }
        
        
        if(s1[i]==s2[j]){
            return dp[i][j] = 1 + fn(i-1,j-1,s1,s2,dp);
        }
        
        return dp[i][j]=max(fn(i-1,j,s1,s2,dp),fn(i,j-1,s1,s2,dp));
        
    }

    
    int lcs(string &s1, string &s2) {
        // code here
        int n = s1.size();
        int m = s2.size();
        
        vector<vector<int>>dp(n,vector<int>(m,-1));
        
        return fn(n-1,m-1,s1,s2,dp);
    }
    
    
    
        int fn(int i,int j,string &s1, string &s2,vector<vector<int>>&dp){
        
        
        if(i<0 || j<0) return 0;
        
        if(dp[i][j]!=-1){
            return dp[i][j];
        }
        
        
        if(s1[i]==s2[j]){
            return dp[i][j] = 1 + fn(i-1,j-1,s1,s2,dp);
        }
        
        return dp[i][j]=max(fn(i-1,j,s1,s2,dp),fn(i,j-1,s1,s2,dp));
        
    }

    
    int lcs(string &s1, string &s2) {
        // code here
        int n = s1.size();
        int m = s2.size();
        
        vector<vector<int>>dp(n,vector<int>(m,-1));
        
        return fn(n-1,m-1,s1,s2,dp);
    }
    */
    
    
int lcs(string &s1, string &s2) {
    int n = s1.size();
    int m = s2.size();
    
    vector<vector<int>> dp(n, vector<int>(m, 0));
    
    // Initialize first row
    for (int j = 0; j < m; j++) {
        if (s1[0] == s2[j]) dp[0][j] = 1;
        /* 
            we have record previous value as well
            when we say j E (0..m-1) and i=0
            
            si => a
            sj => asdfgh
            
            what should be dp[0][j]
            it will be => [1,1,1,1,1]
            
            let take si  and sj( take any len of sj, len>0) 
            
            the lcs will be always be one and thats what 
            [1,1,1,1,1] represents for dp[0][j]
    
        */
        else if (j > 0) dp[0][j] = dp[0][j - 1];
    }

    // Initialize first column
    for (int i = 0; i < n; i++) {
        if (s1[i] == s2[0]) dp[i][0] = 1;
        else if (i > 0) dp[i][0] = dp[i - 1][0];
    }

    // Fill the rest of the dp table
    for (int i = 1; i < n; i++) {
        for (int j = 1; j < m; j++) {
            if (s1[i] == s2[j]) {
                dp[i][j] = 1 + dp[i - 1][j - 1];
            } else {
                dp[i][j] = max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }
    
    return dp[n - 1][m - 1];
}

    
};
