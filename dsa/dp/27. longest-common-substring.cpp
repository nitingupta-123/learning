/*

https://www.youtube.com/watch?v=_wP9mWNPL5w&list=PLgUwDviBIf0qUlt5H_kiKYaNSqJ81PMMY&index=29

https://www.geeksforgeeks.org/problems/longest-common-substring1452/1

*/

class Solution {
  public:
    // space optimization
    int longestCommonSubstr(string& s1, string& s2) {
        // your code here
        int n = s1.size();
        int m = s2.size();
        vector<int>curr(m+1,0),prev(m+1,0);
        int ans = 0;
        
	    for(int i=0;i<=n;i++){
		    for(int j=0;j<=m;j++){
			    if(i==0 || j==0){
				    prev[j] =0;
			    }
			    else if(s1[i-1]==s2[j-1]){
				    curr[j] = 1+ prev[j-1];
				    ans = max(ans,curr[j]);
			    }
			    else{
				    curr[j] = 0;
			    }
		    }
		    prev = curr;
	    }
	        
	    return ans;
	    
    }
    /*
    int longestCommonSubstr(string& s1, string& s2) {
        // your code here
        int n = s1.size();
        int m = s2.size();
        vector<vector<int>>dp(n+1,vector<int>(m+1,0));
        
        int ans = 0;
        
	    for(int i=0;i<=n;i++){
		    for(int j=0;j<=m;j++){
			    if(i==0 || j==0){
				    dp[i][j] =0;
			    }
			    else if(s1[i-1]==s2[j-1]){
				    dp[i][j] = 1+ dp[i-1][j-1];
				    ans = max(ans,dp[i][j]);
			    }
			    else{
				dp[i][j] = 0;
			    }
		    }
	    }
	        
	    return ans;
	    
    }*/
};