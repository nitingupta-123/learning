class Solution {

  public:
    int minOperations(string &s1, string &s2) {
        // Your code goes here
        

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
        
        return n +m -2*prev[m];
    }
};