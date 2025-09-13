// User function template for C++

/*

https://www.youtube.com/watch?v=xElxAuBcvsU&list=PLgUwDviBIf0qUlt5H_kiKYaNSqJ81PMMY&index=33

https://www.geeksforgeeks.org/problems/shortest-common-supersequence0322/1
*/

class Solution {
  public:
    // Function to find length of shortest common supersequence of two strings.
    int shortestCommonSupersequence(string &s1, string &s2) {
        // code here
        
        int n = s1.size();
        int m = s2.size();
        
        vector<int>curr(m+1,0),prev(m+1,0);
        
        for(int i=0;i<=n;i++){
            for(int j=0;j<=m;j++){
                
                if(i==0 || j==0){
                    prev[j] =0;
                }
                else if(s1[i-1]==s2[j-1]){
                    curr[j] = 1+ prev[j-1];
                }
                else{
                    curr[j] = max(prev[j],curr[j-1]);
                }
                
            }
            
            prev = curr;
        }
        
        return n +m -prev[m];
    }
};